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

	Nspace r;
	r.nspace = gi_namespace;
	r.version = gi_version;
	r.c_prefix = std::string (c_prefix);
	r.typelib = typelib;

	std::transform (r.c_prefix.begin (), r.c_prefix.end (),
	                r.c_prefix.begin (), ::tolower);

	this->_typelibs.push_back (r);
}

/* Try to find typelib information about the function. */
GIBaseInfo*
GirManager::find_function_info (const std::string& func_name) const
{
	GIBaseInfo *info = NULL;
	std::string func_name_stripped;

	for (std::vector<Nspace>::const_iterator it = this->_typelibs.begin (),
	     ie = this->_typelibs.end (); it != ie; ++it) {
		const Nspace r = *it;

		DEBUG ("Looking for function " << func_name <<
		       " in repository " << r.nspace << " (version " <<
		       r.version << ", C prefix ‘" << r.c_prefix << "’).");

		/* The func_name includes the namespace, which needs stripping.
		 * e.g. g_irepository_find_by_name → find_by_name. */
		if (func_name.compare (0, r.c_prefix.size (),
		                       r.c_prefix) == 0) {
			size_t prefix_len =
				r.c_prefix.size () + 1 /* underscore */;
			func_name_stripped = func_name.substr (prefix_len);
		} else {
			DEBUG ("\tDoesn’t match C prefix ‘" << r.c_prefix <<
			       "’.");
			continue;
		}

		info = g_irepository_find_by_name (this->_repo,
		                                   r.nspace.c_str (),
		                                   func_name_stripped.c_str ());

		if (info != NULL) {
			/* Successfully found an entry in the typelib. */
			DEBUG ("Found info!");
			break;
		}
	}

	return info;
}
