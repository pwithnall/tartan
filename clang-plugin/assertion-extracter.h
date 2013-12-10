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

#include <unordered_set>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

using namespace clang;

namespace AssertionExtracter {
	Expr* is_assertion_stmt (Stmt& stmt, const ASTContext& context);

	std::unordered_set<const ValueDecl*> assertion_is_nonnull_check (
		Expr& assertion_expr, const ASTContext& context);
}
