/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Tartan
 * Copyright © 2014 Collabora Ltd.
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

#ifndef TARTAN_TYPE_MANAGER_H
#define TARTAN_TYPE_MANAGER_H

#include <unordered_map>

#include <clang/AST/ASTContext.h>

namespace tartan {

using namespace clang;

class TypeManager {
public:
	explicit TypeManager (const ASTContext &context) :
		_context (context) {};

	const QualType find_type_by_name (const std::string name);
	const QualType find_pointer_type_by_name (const std::string name);

private:
	const ASTContext &_context;

	std::unordered_map<std::string, QualType> _type_cache;
};

} /* namespace tartan */

#endif /* !TARTAN_TYPE_MANAGER_H */
