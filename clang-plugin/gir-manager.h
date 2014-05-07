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

#ifndef TARTAN_GIR_MANAGER_H
#define TARTAN_GIR_MANAGER_H

#include <girepository.h>

class GirManager {
private:
	struct Nspace {
		/* All non-NULL. */
		std::string nspace;
		std::string version;
		std::string c_prefix;

		GITypelib* typelib;  /* unowned */
	};

	GIRepository* _repo;  /* unowned */
	std::vector<Nspace> _typelibs;

public:
	GirManager ();

	void load_namespace (const std::string& gi_namespace,
	                     const std::string& gi_version,
	                     GError** error);

	GIBaseInfo* find_function_info (const std::string& func_name) const;
};

#endif /* !TARTAN_GIR_MANAGER_H */
