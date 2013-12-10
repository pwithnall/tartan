/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * gnome-clang
 * Copyright © 2013 Collabora Ltd.
 *
 * gnome-clang is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gnome-clang is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gnome-clang.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Philip Withnall <philip.withnall@collabora.co.uk>
 */

#include <girepository.h>
#include <gitypes.h>

#include <clang/AST/Attr.h>

#include "debug.h"
#include "gir-attributes.h"

/* Determine whether a type should be const, given its (transfer) annotation and
 * base type. */
static bool
_type_should_be_const (GITransfer transfer, GITypeTag type_tag)
{
	return (transfer == GI_TRANSFER_NOTHING &&
	        (type_tag == GI_TYPE_TAG_UTF8 ||
	         type_tag == GI_TYPE_TAG_FILENAME ||
	         type_tag == GI_TYPE_TAG_ARRAY ||
	         type_tag == GI_TYPE_TAG_GLIST ||
	         type_tag == GI_TYPE_TAG_GSLIST ||
	         type_tag == GI_TYPE_TAG_GHASH ||
	         type_tag == GI_TYPE_TAG_ERROR));
}

/* Make the return type of a FunctionType const. This will go one level of
 * typing below the return type, so it won’t constify the top-level pointer
 * return. e.g.:
 *     char* → const char *          (pointer to const char)
 * and not:
 *     char* → char * const          (const pointer to char)
 *     char* → const char * const    (const pointer to const char) */
static void
_constify_function_return_type (FunctionDecl& func)
{
	/* We have to construct a new type because the existing FunctionType
	 * is immutable. */
	const FunctionType* f_type = func.getType ()->getAs<FunctionType> ();
	ASTContext& context = func.getASTContext ();
	const QualType old_result_type = f_type->getResultType ();

	const PointerType* old_result_pointer_type = dyn_cast<PointerType> (old_result_type);
	if (old_result_pointer_type == NULL)
		return;

	QualType new_result_pointee_type =
		old_result_pointer_type->getPointeeType ().withConst ();
	QualType new_result_type = context.getPointerType (new_result_pointee_type);

	for (FunctionDecl* func_decl = func.getMostRecentDecl ();
	     func_decl != NULL; func_decl = func_decl->getPreviousDecl ()) {
		const FunctionNoProtoType* f_n_type =
			dyn_cast<FunctionNoProtoType> (f_type);
		QualType t;

		if (f_n_type != NULL) {
			t = context.getFunctionNoProtoType (new_result_type,
			                                    f_n_type->getExtInfo ());
		} else {
			const FunctionProtoType *f_p_type =
				cast<FunctionProtoType> (f_type);
			t = context.getFunctionType (new_result_type,
			                             f_p_type->getArgTypes (),
			                             f_p_type->getExtProtoInfo ());
		}

		DEBUG ("Constifying type " <<
		       func_decl->getType ().getAsString () << " → " <<
		       t.getAsString ());
		func_decl->setType (t);
	}
}

void
GirAttributesConsumer::_handle_function_decl (FunctionDecl& func)
{
	/* Ignore static functions immediately; they shouldn’t have any
	 * GIR data, and searching for it massively slows down
	 * compilation. */
	StorageClass sc = func.getStorageClass ();
	if (sc != SC_None && sc != SC_Extern)
		return;

	/* Try to find typelib information about the function. */
	const std::string func_name = func.getNameAsString ();  /* TODO: expensive? */
	GIBaseInfo *info = this->_gir_manager.get ()->find_function_info (func_name);

	if (info == NULL)
		return;

	/* Extract information from the GIBaseInfo and add AST attributes
	 * accordingly. */
	switch (g_base_info_get_type (info)) {
	case GI_INFO_TYPE_FUNCTION: {
		GICallableInfo *callable_info = (GICallableInfo *) info;

		std::vector<unsigned int> non_null_args;
		unsigned int j, k;

		NonNullAttr* nonnull_attr = func.getAttr<NonNullAttr> ();
		if (nonnull_attr != NULL) {
			/* Extend and replace the existing attribute. */
			DEBUG ("Extending existing attribute.");
			non_null_args.insert (non_null_args.begin (),
			                      nonnull_attr->args_begin (),
			                      nonnull_attr->args_end ());
		}

		for (j = 0, k = g_callable_info_get_n_args (callable_info);
		     j < k; j++) {
			GIArgInfo arg;
			GITypeInfo type_info;
			GITransfer transfer;
			GITypeTag type_tag;

			g_callable_info_load_arg (callable_info, j, &arg);
			g_arg_info_load_type (&arg, &type_info);
			transfer = g_arg_info_get_ownership_transfer (&arg);
			type_tag = g_type_info_get_tag (&type_info);

			if (!g_arg_info_may_be_null (&arg) &&
			    g_type_info_is_pointer (&type_info)) {
				DEBUG ("Got nonnull arg " << j << " from GIR.");
				non_null_args.push_back (j);
			}

			if (_type_should_be_const (transfer, type_tag)) {
				ParmVarDecl *parm = func.getParamDecl (j);
				QualType t = parm->getType ();

				if (!t.isConstant (parm->getASTContext ()))
					parm->setType (t.withConst ());
			}
		}

		if (non_null_args.size () > 0) {
			nonnull_attr = ::new (func.getASTContext ())
				NonNullAttr (func.getSourceRange (),
				             func.getASTContext (),
				             non_null_args.data (),
				             non_null_args.size ());
			func.addAttr (nonnull_attr);
		}

		/* Process the function’s return type. */
		/* FIXME: Support returns_nonnull when Clang supports it.
		 * http://llvm.org/bugs/show_bug.cgi?id=4832 */
		GITypeInfo return_type_info;
		GITransfer return_transfer;
		GITypeTag return_type_tag;

		g_callable_info_load_return_type (info, &return_type_info);
		return_transfer = g_callable_info_get_caller_owns (info);
		return_type_tag = g_type_info_get_tag (&return_type_info);

		if (return_transfer != GI_TRANSFER_NOTHING) {
			WarnUnusedAttr* warn_unused_attr =
				::new (func.getASTContext ())
				WarnUnusedAttr (func.getSourceRange (),
				                func.getASTContext ());
			func.addAttr (warn_unused_attr);
		} else if (_type_should_be_const (return_transfer,
		                                  return_type_tag)) {
			_constify_function_return_type (func);
		}

		/* Mark the function as deprecated if it wasn’t already. The
		 * typelib file doesn’t contain a deprecation message, version,
		 * or replacement function so we can’t make use of them. */
		if (g_base_info_is_deprecated (info) &&
		    !func.hasAttr<DeprecatedAttr> ()) {
			DeprecatedAttr* deprecated_attr =
				::new (func.getASTContext ())
				DeprecatedAttr (func.getSourceRange (),
				                func.getASTContext (),
				                "Deprecated using the gtk-doc "
				                "attribute.");
			func.addAttr (deprecated_attr);
		}

		/* Mark the function as allocating memory if it’s a
		 * constructor. */
		if (g_function_info_get_flags (info) &
		    GI_FUNCTION_IS_CONSTRUCTOR &&
		    !func.hasAttr<MallocAttr> ()) {
			MallocAttr* malloc_attr =
				::new (func.getASTContext ())
				MallocAttr (func.getSourceRange (),
				            func.getASTContext ());
			func.addAttr (malloc_attr);
		}

		break;
	}
	case GI_INFO_TYPE_CALLBACK:
	case GI_INFO_TYPE_STRUCT:
	case GI_INFO_TYPE_BOXED:
	case GI_INFO_TYPE_ENUM:
	case GI_INFO_TYPE_FLAGS:
	case GI_INFO_TYPE_OBJECT:
	case GI_INFO_TYPE_INTERFACE:
	case GI_INFO_TYPE_CONSTANT:
	case GI_INFO_TYPE_INVALID_0:
	case GI_INFO_TYPE_UNION:
	case GI_INFO_TYPE_VALUE:
	case GI_INFO_TYPE_SIGNAL:
	case GI_INFO_TYPE_VFUNC:
	case GI_INFO_TYPE_PROPERTY:
	case GI_INFO_TYPE_FIELD:
	case GI_INFO_TYPE_ARG:
	case GI_INFO_TYPE_TYPE:
	case GI_INFO_TYPE_UNRESOLVED:
	case GI_INFO_TYPE_INVALID:
	default:
		llvm::errs () << "Error: Unhandled GI type " <<
		                 g_base_info_get_type (info) <<
		                 " in introspection info for function ‘" <<
		                 func_name << "’.\n";
	}

	g_base_info_unref (info);
}

bool
GirAttributesConsumer::HandleTopLevelDecl (DeclGroupRef decl_group)
{
	DeclGroupRef::iterator i, e;

	for (i = decl_group.begin (), e = decl_group.end (); i != e; i++) {
		Decl *decl = *i;
		FunctionDecl *func = dyn_cast<FunctionDecl> (decl);

		/* We’re only interested in function declarations. */
		if (func == NULL)
			continue;

		this->_handle_function_decl (*func);
	}

	return true;
}
