/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Tartan
 * Copyright © 2013 Collabora Ltd.
 *
 * Tartan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tartan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tartan.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Philip Withnall <philip.withnall@collabora.co.uk>
 */

#include "config.h"

#include <cstring>

#include <girepository.h>
#include <gitypes.h>

#include <clang/AST/Attr.h>

#include "debug.h"
#include "gir-attributes.h"

namespace tartan {

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

/* Determine whether an argument is definitely required to be non-NULL given
 * its (nullable) and (optional) annotations, direction annotation and type.
 *
 * If it’s an array type, it may be NULL if its associated length parameter is
 * 0. Since we can’t currently analyse array bounds, assume that all C array
 * parameters may be NULL. (Other array types are structs, so may not be
 * NULL.) */
static bool
_arg_is_nonnull (GIArgInfo arg, GITypeInfo type_info)
{
	return ((g_type_info_is_pointer (&type_info) ||
	         g_arg_info_get_direction (&arg) == GI_DIRECTION_OUT) &&
	        !g_arg_info_may_be_null (&arg) &&
	        !g_arg_info_is_optional (&arg) &&
	        !(g_type_info_get_tag (&type_info) == GI_TYPE_TAG_ARRAY &&
	          g_type_info_get_array_type (&type_info) == GI_ARRAY_TYPE_C));
}

/* Determine whether a return type is constant. Typically, this will be used
 * for constant pointer types, in which case pointer_type will be non-NULL. */
static bool
_function_return_type_is_const (FunctionDecl& func)
{
#ifdef HAVE_LLVM_3_5
	QualType type = func.getReturnType ();
#else /* if !HAVE_LLVM_3_5 */
	QualType type = func.getResultType ();
#endif /* !HAVE_LLVM_3_5 */

	QualType canonical_type = type.getCanonicalType ();

	const PointerType* pointer_type = dyn_cast<PointerType> (canonical_type);
	if (pointer_type == NULL)
		return canonical_type.isConstQualified ();

	QualType pointee_type =
		pointer_type->getPointeeType ().getCanonicalType ();

	return (pointee_type.isConstQualified () ||
	        canonical_type.isConstQualified ());
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
#ifdef HAVE_LLVM_3_5
	const QualType old_result_type = f_type->getReturnType ();
#else /* if !HAVE_LLVM_3_5 */
	const QualType old_result_type = f_type->getResultType ();
#endif /* !HAVE_LLVM_3_5 */

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
			ArrayRef<QualType> param_types;

#ifdef HAVE_LLVM_3_5
			param_types = f_p_type->getParamTypes ();
#else /* !HAVE_LLVM_3_5 */
			param_types = f_p_type->getArgTypes ();
#endif /* !HAVE_LLVM_3_5 */

			t = context.getFunctionType (new_result_type,
			                             param_types,
			                             f_p_type->getExtProtoInfo ());
		}

		DEBUG ("Constifying type " <<
		       func_decl->getType ().getAsString () << " → " <<
		       t.getAsString ());
		func_decl->setType (t);
	}
}

/* Determine whether the given function should be excluded from having extra
 * nonnull attributes added due to being GLib-internal. Such functions are
 * exported as symbols from libglib-2.0.so, but are deliberately not in the GIR
 * file — and hence we can’t pick up annotations on them. */
static bool
_ignore_glib_internal_func (const std::string func_name)
{
	static const char *internal_funcs[] = {
		"g_assertion_message",
		"g_assertion_message_cmpnum",
		"g_assertion_message_cmpstr",
		"g_assertion_message_error",
		"g_assertion_message_expr",
		"g_test_trap_assertions",
		"g_return_if_fail_warning",
		"g_warn_message",
	};

	for (unsigned int i = 0; i < G_N_ELEMENTS (internal_funcs); i++) {
		if (strcmp (func_name.c_str (), internal_funcs[i]) == 0) {
			return true;
		}
	}

	return false;
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

		/* GError formal parameters aren’t included in the number of
		 * callable arguments. */
		unsigned int k = g_callable_info_get_n_args (callable_info);
		unsigned int err_params =
			(g_function_info_get_flags (callable_info) &
			 GI_FUNCTION_THROWS) ? 1 : 0;
		unsigned int obj_params =
			(g_base_info_get_container (info) != NULL &&
			 g_function_info_get_flags (callable_info) &
			 GI_FUNCTION_IS_METHOD) ? 1 : 0;

		/* Sanity check. */
		if (obj_params + k + err_params != func.getNumParams ()) {
			WARN ("Number of GIR callable parameters (" <<
			      obj_params + k + err_params << ") "
			      "differs from number of C formal parameters (" <<
			      func.getNumParams () << "). Ignoring function " <<
			      func_name << "().");
			break;
		}

		std::vector<unsigned int> non_null_args;
		unsigned int j;

		NonNullAttr* nonnull_attr = func.getAttr<NonNullAttr> ();
		if (nonnull_attr != NULL) {
			/* Extend and replace the existing attribute. */
			DEBUG ("Extending existing attribute.");
			non_null_args.insert (non_null_args.begin (),
			                      nonnull_attr->args_begin (),
			                      nonnull_attr->args_end ());
		}

		for (j = 0; j < k; j++) {
			GIArgInfo arg;
			GITypeInfo type_info;
			GITransfer transfer;
			GITypeTag type_tag;

			g_callable_info_load_arg (callable_info, j, &arg);
			g_arg_info_load_type (&arg, &type_info);
			transfer = g_arg_info_get_ownership_transfer (&arg);
			type_tag = g_type_info_get_tag (&type_info);

			DEBUG_CODE (int array_type =
				(g_type_info_get_tag (&type_info) ==
				 GI_TYPE_TAG_ARRAY) ?
					g_type_info_get_array_type (&type_info) :
					-1);
			DEBUG ("GirAttributes: " << func_name << "(" << j <<
			       ")\n"
			       "\tTransfer: " << transfer << "\n"
			       "\tDirection: " <<
			       g_arg_info_get_direction (&arg) << "\n"
			       "\tNullable: " <<
			       g_arg_info_may_be_null (&arg) << "\n"
			       "\tOptional: " <<
			       g_arg_info_is_optional (&arg) << "\n"
			       "\tIs pointer: " <<
			       g_type_info_is_pointer (&type_info) << "\n"
			       "\tType tag: " <<
			       g_type_tag_to_string (
			           g_type_info_get_tag (&type_info)) << "\n"
			       "\tArray type: " <<
			       array_type << "\n"
			       "\tArray length: " <<
			       g_type_info_get_array_length (&type_info) << "\n"
			       "\tArray fixed size: " <<
			       g_type_info_get_array_fixed_size (&type_info));

			if (_arg_is_nonnull (arg, type_info)) {
				DEBUG ("Got nonnull arg " << obj_params + j <<
				       " from GIR.");
				non_null_args.push_back (obj_params + j);
			}

			if (_type_should_be_const (transfer, type_tag)) {
				ParmVarDecl *parm = func.getParamDecl (obj_params + j);
				QualType t = parm->getType ();

				if (!t.isConstant (parm->getASTContext ()))
					parm->setType (t.withConst ());
			}
		}

		if (non_null_args.size () > 0 &&
		    !_ignore_glib_internal_func (func_name)) {
#ifdef HAVE_LLVM_3_5
			nonnull_attr = ::new (func.getASTContext ())
				NonNullAttr (func.getSourceRange (),
				             func.getASTContext (),
				             non_null_args.data (),
				             non_null_args.size (), 0);
#else /* if !HAVE_LLVM_3_5 */
			nonnull_attr = ::new (func.getASTContext ())
				NonNullAttr (func.getSourceRange (),
				             func.getASTContext (),
				             non_null_args.data (),
				             non_null_args.size ());
#endif /* !HAVE_LLVM_3_5 */
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
#ifdef HAVE_LLVM_3_5
			WarnUnusedAttr* warn_unused_attr =
				::new (func.getASTContext ())
				WarnUnusedAttr (func.getSourceRange (),
				                func.getASTContext (), 0);
#else /* if !HAVE_LLVM_3_5 */
			WarnUnusedAttr* warn_unused_attr =
				::new (func.getASTContext ())
				WarnUnusedAttr (func.getSourceRange (),
				                func.getASTContext ());
#endif /* !HAVE_LLVM_3_5 */
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
#ifdef HAVE_LLVM_3_8
			DeprecatedAttr* deprecated_attr =
				::new (func.getASTContext ())
				DeprecatedAttr (func.getSourceRange (),
				                func.getASTContext (),
				                0);
#elif HAVE_LLVM_3_5
			DeprecatedAttr* deprecated_attr =
				::new (func.getASTContext ())
				DeprecatedAttr (func.getSourceRange (),
				                func.getASTContext (),
				                "Deprecated using the gtk-doc "
				                "attribute.", 0);
#else /* if !HAVE_LLVM_3_5 */
			DeprecatedAttr* deprecated_attr =
				::new (func.getASTContext ())
				DeprecatedAttr (func.getSourceRange (),
				                func.getASTContext (),
				                "Deprecated using the gtk-doc "
				                "attribute.");
#endif /* !HAVE_LLVM_3_5 */
			func.addAttr (deprecated_attr);
		}

		/* Mark the function as allocating memory if it’s a
		 * constructor. */
#if defined(HAVE_LLVM_3_7)
		if (g_function_info_get_flags (info) &
		    GI_FUNCTION_IS_CONSTRUCTOR &&
		    !func.hasAttr<RestrictAttr> ()) {
			RestrictAttr* malloc_attr =
				::new (func.getASTContext ())
				RestrictAttr (func.getSourceRange (),
				              func.getASTContext (), 0);
			func.addAttr (malloc_attr);
		}
#elif defined(HAVE_LLVM_3_6)
		if (g_function_info_get_flags (info) &
		    GI_FUNCTION_IS_CONSTRUCTOR &&
		    !func.hasAttr<MallocAttr> ()) {
			MallocAttr* malloc_attr =
				::new (func.getASTContext ())
				MallocAttr (func.getSourceRange (),
				            func.getASTContext (), 0);
			func.addAttr (malloc_attr);
		}
#else
		if (g_function_info_get_flags (info) &
		    GI_FUNCTION_IS_CONSTRUCTOR &&
		    !func.hasAttr<MallocAttr> ()) {
			MallocAttr* malloc_attr =
				::new (func.getASTContext ())
				MallocAttr (func.getSourceRange (),
				            func.getASTContext ());
			func.addAttr (malloc_attr);
		}
#endif

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
		WARN ("Error: Unhandled GI type " <<
		      g_base_info_get_type (info) << " in introspection info "
		      "for function ‘" << func_name << "’.");
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


void
GirAttributesChecker::_handle_function_decl (FunctionDecl& func)
{
	/* TODO: Factor this out and share it with the implementation above. */
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

	/* Extract information from the GIBaseInfo and check AST attributes
	 * accordingly. */
	switch (g_base_info_get_type (info)) {
	case GI_INFO_TYPE_FUNCTION: {
		GICallableInfo *callable_info = (GICallableInfo *) info;

		/* GError formal parameters aren’t included in the number of
		 * callable arguments. */
		unsigned int k = g_callable_info_get_n_args (callable_info);
		unsigned int err_params =
			(g_function_info_get_flags (callable_info) &
			 GI_FUNCTION_THROWS) ? 1 : 0;
		unsigned int obj_params =
			(g_base_info_get_container (info) != NULL &&
			 g_function_info_get_flags (callable_info) &
			 GI_FUNCTION_IS_METHOD) ? 1 : 0;

		/* Sanity check. */
		if (obj_params + k + err_params != func.getNumParams ()) {
			WARN ("Number of GIR callable parameters (" <<
			      obj_params + k + err_params << ") "
			      "differs from number of C formal parameters (" <<
			      func.getNumParams () << "). Ignoring function " <<
			      func_name << "().");
			break;
		}

		/* Process the function’s return type. */
		GITypeInfo return_type_info;
		GITransfer return_transfer;
		GITypeTag return_type_tag;

		g_callable_info_load_return_type (info, &return_type_info);
		return_transfer = g_callable_info_get_caller_owns (info);
		return_type_tag = g_type_info_get_tag (&return_type_info);

		/* If the return type is const-qualified but no (transfer none)
		 * annotation exists, emit a warning.
		 *
		 * Similarly, if a (transfer none) annotation exists but the
		 * return type is not const-qualified, emit a warning. */
		if (_function_return_type_is_const (func) &&
		    return_transfer != GI_TRANSFER_NOTHING) {
			Debug::emit_error (
				"Missing (transfer none) annotation on the "
				"return value of function %0() (already has a "
				"const modifier).",
				this->_compiler,
				func.getLocStart ())
			<< func.getNameAsString ();
		} else if (return_transfer == GI_TRANSFER_NOTHING &&
		           _type_should_be_const (return_transfer,
		                                  return_type_tag) &&
		           !_function_return_type_is_const (func)) {
			Debug::emit_error (
				"Missing const modifier on the return value of "
				"function %0() (already has a (transfer none) "
				"annotation).",
				this->_compiler,
				func.getLocStart ())
			<< func.getNameAsString ();
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
		WARN ("Error: Unhandled GI type " <<
		      g_base_info_get_type (info) << " in introspection info "
		      "for function ‘" << func_name << "’.");
	}

	g_base_info_unref (info);
}

bool
GirAttributesChecker::HandleTopLevelDecl (DeclGroupRef decl_group)
{
	DeclGroupRef::iterator i, e;

	/* Run away if the plugin is disabled. */
	if (!this->is_enabled ()) {
		return true;
	}

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

} /* namespace tartan */
