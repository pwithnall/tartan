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

#include <unordered_map>

#include <clang/AST/Attr.h>
#include <clang/Lex/Lexer.h>

#include "type-manager.h"
#include "debug.h"

namespace tartan {

/* Find a #QualType for the typedeffed type with the given @name. This is a very
 * slow call (it requires iterating through all defined types in the given
 * @context), so its results should be cached where possible (FIXME: do this).
 *
 * If type lookup fails, a null type is returned. */
const QualType
TypeManager::find_type_by_name (const std::string name)
{
	/* Look up the type in the cache first. */
	std::unordered_map<std::string, QualType>::const_iterator cached =
		this->_type_cache.find (name);
	if (cached != this->_type_cache.end ()) {
		return (*cached).second;
	}

	for (ASTContext::const_type_iterator it = this->_context.types_begin (),
	     ie = this->_context.types_end (); it != ie; ++it) {
		const Type *t = *it;
		const TypedefType *tt = t->getAs<TypedefType> ();

		if (tt != NULL) {
			const TypedefNameDecl *decl = tt->getDecl ();

			if (decl->getName () == name) {
				DEBUG ("Found type ‘" << name << "’ with "
				       "desugared type ‘" <<
				       tt->desugar ().getAsString () << "’.");

				QualType qt (tt, 0);

				/* Insert it into the cache. */
				std::string _name (name);
				this->_type_cache.emplace (_name, qt);

				return qt;
			}
		}
	}

	DEBUG ("Failed to find type ‘" << name << "’.");

	return QualType ();
}

/* Version of _find_type_by_name() which makes it a pointer type. */
const QualType
TypeManager::find_pointer_type_by_name (const std::string name)
{
	QualType qt = this->find_type_by_name (name);
	if (!qt.isNull ()) {
		return this->_context.getPointerType (qt);
	}

	return QualType ();
}

} /* namespace tartan */
