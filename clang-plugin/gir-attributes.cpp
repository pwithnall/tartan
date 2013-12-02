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

#include <clang/AST/Attr.h>

#include <girepository.h>
#include <gitypes.h>

#include "gir-attributes.h"

#ifdef ENABLE_DEBUG
#define DEBUG(X) llvm::errs () << X << "\n"
#else
#define DEBUG(X)
#endif

GirAttributesConsumer::GirAttributesConsumer (std::string& gi_namespace,
                                              std::string& gi_version)
{
	this->_gi_namespace = gi_namespace;
	this->_gi_version = gi_version;
}

GirAttributesConsumer::~GirAttributesConsumer ()
{
	if (this->_typelib != NULL)
		g_object_unref (this->_typelib);
	if (this->_repo != NULL)
		g_object_unref (this->_repo);
}

void
GirAttributesConsumer::prepare (GError **error)
{
	/* Load the GIR typelib. */
	this->_repo = g_irepository_get_default ();
	this->_typelib = g_irepository_require (this->_repo,
	                                        this->_gi_namespace.c_str (),
	                                        this->_gi_version.c_str (),
	                                        (GIRepositoryLoadFlags) 0,
	                                        error);

	if (this->_typelib == NULL) {
		g_object_unref (this->_repo);
		this->_repo = NULL;
	}

	/* Get the C prefix from the repository and convert it to lower case. */
	const char *c_prefix =
		g_irepository_get_c_prefix (this->_repo,
		                            this->_gi_namespace.c_str ());
	this->_gi_c_prefix = std::string (c_prefix);
	std::transform (this->_gi_c_prefix.begin (), this->_gi_c_prefix.end (),
	                this->_gi_c_prefix.begin (), ::tolower);
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

	/* The func_name includes the namespace, which needs stripping.
	 * e.g. g_irepository_find_by_name → find_by_name. */
	std::string func_name = func.getNameAsString ();
	std::string func_name_stripped;

	if (func_name.compare (0, this->_gi_c_prefix.size (),
	                       this->_gi_c_prefix) == 0) {
		size_t prefix_len =
			this->_gi_c_prefix.size () + 1 /* underscore */;
		func_name_stripped = func_name.substr (prefix_len);
	} else {
		DEBUG ("Ignoring function " << func_name);
		return;
	}

	/* Try to find typelib information about the function. */
	GIBaseInfo *info;
	info = g_irepository_find_by_name (this->_repo,
	                                   this->_gi_namespace.c_str (),
	                                   func_name_stripped.c_str ());

	DEBUG ("Looking for info for function " << func_name_stripped);

	if (info == NULL)
		return;

	/* Successfully found an entry in the typelib. */
	DEBUG ("Found info!");

	switch (g_base_info_get_type (info)) {
	case GI_INFO_TYPE_FUNCTION: {
		GICallableInfo *callable_info = (GICallableInfo *) info;

		/* TODO: Return types. */
		/* TODO: warning if attr doesn't match gir */

		NonNullAttr *attr;
		std::vector<unsigned int> non_null_args;
		unsigned int j, k;

		attr = func.getAttr<NonNullAttr> ();
		if (attr != NULL) {
			/* Extend and replace the existing attribute. */
			llvm::errs () << "extending existing attr\n";
			non_null_args.insert (non_null_args.begin (),
			                      attr->args_begin (),
			                      attr->args_end ());
		}

		for (j = 0, k = g_callable_info_get_n_args (callable_info);
		     j < k; j++) {
			GIArgInfo arg;
			GITypeInfo type_info;

			g_callable_info_load_arg (callable_info, j, &arg);
			g_arg_info_load_type (&arg, &type_info);

			if (!g_arg_info_may_be_null (&arg) &&
			    g_type_info_is_pointer (&type_info)) {
				llvm::errs () << "got arg " << j << " from GIR\n";
				non_null_args.push_back (j);
			}
		}

		if (non_null_args.size () > 0) {
			llvm::errs () << "NONNULL:\n";
			for (unsigned int z = 0; z < non_null_args.size (); z++)
				llvm::errs () << "\t" << non_null_args[z] << "\n";

			attr = ::new (func.getASTContext ())
				NonNullAttr (func.getSourceRange (),
				             func.getASTContext (),
				             non_null_args.data (),
				             non_null_args.size ());
			func.addAttr (attr);
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
		/* TODO */
		llvm::errs () << "Error: unhandled type " << g_base_info_get_type (info) << "\n";
	}

	g_base_info_unref (info);
}

/* Prepare must have been called successfully before this may be called. */
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
