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

#include <girepository.h>
#include <gitypes.h>

#include <clang/AST/Attr.h>

#include "debug.h"
#include "gir-manager.h"

GirManager::GirManager ()
{
	this->_repo = g_irepository_get_default ();
}

void
GirManager::load_namespace (const std::string& gi_namespace,
                            const std::string& gi_version,
                            GError** error)
{
	/* Load the GIR typelib. */
	GITypelib* typelib = g_irepository_require (this->_repo,
	                                            gi_namespace.c_str (),
	                                            gi_version.c_str (),
	                                            (GIRepositoryLoadFlags) 0,
	                                            error);

	if (typelib == NULL)
		return;

	/* Get the C prefix from the repository and convert it to lower case. */
	const char *c_prefix =
		g_irepository_get_c_prefix (this->_repo,
		                            gi_namespace.c_str ());
	if (c_prefix == NULL) {
		c_prefix = "";
	}

	Nspace r;
	r.nspace = gi_namespace;
	r.version = gi_version;
	r.c_prefix = std::string (c_prefix);
	r.c_prefix_lower = std::string (c_prefix);
	r.typelib = typelib;

	std::transform (r.c_prefix_lower.begin (), r.c_prefix_lower.end (),
	                r.c_prefix_lower.begin (), ::tolower);

	this->_typelibs.push_back (r);
}

/* Returns a reference to the #GIFunctionInfo, or %NULL. */
static GIFunctionInfo *
_find_function_in_function (GIFunctionInfo *info, const std::string &func_name)
{
	if (g_function_info_get_symbol (info) == func_name) {
		return g_base_info_ref (info);
	}

	return NULL;
}

/* Returns a reference to the #GIFunctionInfo, or %NULL. */
static GIFunctionInfo *
_find_function_in_struct (GIStructInfo *info, const std::string &func_name)
{
	GIFunctionInfo *f_info = NULL;
	gint n_methods = g_struct_info_get_n_methods (info);

	for (gint i = 0; i < n_methods && f_info == NULL; i++) {
		GIFunctionInfo *_info = g_struct_info_get_method (info, i);
		f_info = _find_function_in_function (_info, func_name);
		g_base_info_unref (_info);
	}

	return f_info;
}

/* Returns a reference to the #GIFunctionInfo, or %NULL. */
static GIFunctionInfo *
_find_function_in_enum (GIEnumInfo *info, const std::string &func_name)
{
	GIFunctionInfo *f_info = NULL;
	gint n_methods = g_enum_info_get_n_methods (info);

	for (gint i = 0; i < n_methods && f_info == NULL; i++) {
		GIFunctionInfo *_info = g_enum_info_get_method (info, i);
		f_info = _find_function_in_function (_info, func_name);
		g_base_info_unref (_info);
	}

	return f_info;
}

/* Returns a reference to the #GIFunctionInfo, or %NULL. */
static GIFunctionInfo *
_find_function_in_object (GIObjectInfo *info, const std::string &func_name)
{
	GIFunctionInfo *f_info = NULL;
	gint n_methods = g_object_info_get_n_methods (info);

	for (gint i = 0; i < n_methods && f_info == NULL; i++) {
		GIFunctionInfo *_info = g_object_info_get_method (info, i);
		f_info = _find_function_in_function (_info, func_name);
		g_base_info_unref (_info);
	}

	return f_info;
}

/* Returns a reference to the #GIFunctionInfo, or %NULL. */
static GIFunctionInfo *
_find_function_in_interface (GIInterfaceInfo *info, const std::string &func_name)
{
	GIFunctionInfo *f_info = NULL;
	gint n_methods = g_interface_info_get_n_methods (info);

	for (gint i = 0; i < n_methods && f_info == NULL; i++) {
		GIFunctionInfo *_info = g_interface_info_get_method (info, i);
		f_info = _find_function_in_function (_info, func_name);
		g_base_info_unref (_info);
	}

	return f_info;
}

/* Returns a reference to the #GIFunctionInfo, or %NULL. */
static GIFunctionInfo *
_find_function_in_union (GIUnionInfo *info, const std::string &func_name)
{
	GIFunctionInfo *f_info = NULL;
	gint n_methods = g_union_info_get_n_methods (info);

	for (gint i = 0; i < n_methods && f_info == NULL; i++) {
		GIFunctionInfo *_info = g_union_info_get_method (info, i);
		f_info = _find_function_in_function (_info, func_name);
		g_base_info_unref (_info);
	}

	return f_info;
}

/* Try to find typelib information about the function.
 * Note: This returns a reference which needs freeing using
 * g_base_info_unref(). */
GIBaseInfo*
GirManager::find_function_info (const std::string& func_name) const
{
	GIBaseInfo *info = NULL;

	for (std::vector<Nspace>::const_iterator it = this->_typelibs.begin (),
	     ie = this->_typelibs.end (); it != ie && info == NULL; ++it) {
		const Nspace r = *it;

		/* Check the function matches this namespace.
		 * e.g. g_irepository_find_by_name →
		 *      (g_irepository_, find_by_name). */
		if (!r.c_prefix_lower.empty () &&
		    (func_name.size () <= r.c_prefix_lower.size () ||
		     func_name.compare (0, r.c_prefix_lower.size (),
		                        r.c_prefix_lower) != 0 ||
		     func_name[r.c_prefix_lower.size ()] != '_')) {
			continue;
		}

		/* Iterate through every info in the namespace, trying to match
		 * the entire @func_name against the info, or one of the methods
		 * it contains. */
		guint n_infos = g_irepository_get_n_infos (this->_repo,
		                                           r.nspace.c_str ());

		for (guint i = 0; i < n_infos && info == NULL; i++) {
			GIBaseInfo *_info;

			_info = g_irepository_get_info (this->_repo,
			                                r.nspace.c_str (), i);

			switch (g_base_info_get_type (_info)) {
			case GI_INFO_TYPE_FUNCTION:
				info = _find_function_in_function (_info,
				                                   func_name);
				break;
			case GI_INFO_TYPE_STRUCT:
				info = _find_function_in_struct (_info,
				                                 func_name);
				break;
			case GI_INFO_TYPE_ENUM:
				info = _find_function_in_enum (_info,
				                               func_name);
				break;
			case GI_INFO_TYPE_OBJECT:
				info = _find_function_in_object (_info,
				                                 func_name);
				break;
			case GI_INFO_TYPE_INTERFACE:
				info = _find_function_in_interface (_info,
				                                    func_name);
				break;
			case GI_INFO_TYPE_UNION:
				info = _find_function_in_union (_info,
				                                func_name);
				break;
			case GI_INFO_TYPE_INVALID:
			case GI_INFO_TYPE_CALLBACK:
			case GI_INFO_TYPE_BOXED:
			case GI_INFO_TYPE_FLAGS:
			case GI_INFO_TYPE_CONSTANT:
			case GI_INFO_TYPE_INVALID_0:
			case GI_INFO_TYPE_VALUE:
			case GI_INFO_TYPE_SIGNAL:
			case GI_INFO_TYPE_VFUNC:
			case GI_INFO_TYPE_PROPERTY:
			case GI_INFO_TYPE_FIELD:
			case GI_INFO_TYPE_ARG:
			case GI_INFO_TYPE_TYPE:
			case GI_INFO_TYPE_UNRESOLVED:
			default:
				/* Doesn’t have methods — ignore. */
				break;
			}

			g_base_info_unref (_info);
		}
	}

	/* Double-check that this isn’t a shadowed function, since the parameter
	 * information from shadowed functions doesn’t match up with what Clang
	 * has parsed. */
	assert (info == NULL ||
	        (g_base_info_get_type (info) == GI_INFO_TYPE_FUNCTION &&
	         func_name == g_function_info_get_symbol (info)));

	return info;
}

/* Try to find typelib information about the type. The type could be a GObject
 * or a GInterface.
 *
 * Note: This returns a reference which needs freeing using
 * g_base_info_unref(). The GIBaseInfo* is guaranteed to be a valid
 * GIObjectInfo*. */
GIBaseInfo*
GirManager::find_object_info (const std::string& type_name) const
{
	GIBaseInfo *info = NULL;
	std::string type_name_stripped;

	for (std::vector<Nspace>::const_iterator it = this->_typelibs.begin (),
	     ie = this->_typelibs.end (); it != ie; ++it) {
		const Nspace r = *it;

		/* The type_name includes the namespace, which needs stripping.
		 * e.g. GObject → Object. */
		if (!r.c_prefix.empty () &&
		    type_name.size () > r.c_prefix.size () &&
		    type_name.compare (0, r.c_prefix.size (),
		                       r.c_prefix) == 0) {
			size_t prefix_len = r.c_prefix.size ();
			type_name_stripped = type_name.substr (prefix_len);
		} else if (r.c_prefix.empty ()) {
			type_name_stripped = type_name;
		} else {
			continue;
		}

		info = g_irepository_find_by_name (this->_repo,
		                                   r.nspace.c_str (),
		                                   type_name_stripped.c_str ());

		if (info != NULL) {
			/* Successfully found an entry in the typelib. */
			break;
		}
	}

	/* Check it is actually a GObject. */
	if (info != NULL &&
	    g_base_info_get_type (info) != GI_INFO_TYPE_OBJECT &&
	    g_base_info_get_type (info) != GI_INFO_TYPE_INTERFACE) {
		DEBUG ("Ignoring type " << type_name << " as its GI info "
		       "indicates it’s not a GObject.");

		g_base_info_unref (info);
		info = NULL;
	}

	return info;
}

/* Return the full C name of a type. For example, this is ‘GObject’ for a
 * GObject. The prefix in this case is ‘G’ and the symbol name is ‘Object’. */
std::string
GirManager::get_c_name_for_type (GIBaseInfo *base_info) const
{
	std::string symbol_name (g_base_info_get_name (base_info));
	const gchar *c_prefix = g_irepository_get_c_prefix (this->_repo,
	                                                    g_base_info_get_namespace (base_info));

	if (c_prefix == NULL) {
		return symbol_name;
	} else {
		return std::string (c_prefix) + symbol_name;
	}
}
