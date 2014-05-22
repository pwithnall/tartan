/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Tartan
 * Copyright © 2014 Philip Withnall
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
 *     Philip Withnall <philip@tecnocode.co.uk>
 */

#ifndef TARTAN_GVARIANT_CHECKER_H
#define TARTAN_GVARIANT_CHECKER_H

#include <unordered_set>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>

#include "checker.h"

namespace tartan {

using namespace clang;

class GVariantVisitor : public RecursiveASTVisitor<GVariantVisitor> {
public:
	explicit GVariantVisitor (CompilerInstance& compiler) :
		_compiler (compiler), _context (compiler.getASTContext ()) {}

private:
	QualType _gvariant_pointer_type;
	CompilerInstance& _compiler;
	const ASTContext& _context;

public:
	bool VisitCallExpr (CallExpr* call);
};

class GVariantConsumer : public tartan::Checker {
public:
	GVariantConsumer (CompilerInstance& compiler,
	                  std::shared_ptr<const GirManager> gir_manager,
	                  std::shared_ptr<const std::unordered_set<std::string>> disabled_plugins) :
		Checker (compiler, gir_manager, disabled_plugins),
		_visitor (compiler) {}

private:
	GVariantVisitor _visitor;

public:
	virtual void HandleTranslationUnit (ASTContext& context);
	const std::string get_name () const { return "gvariant"; }
};

} /* namespace tartan */

#endif /* !TARTAN_GVARIANT_CHECKER_H */
