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

#ifndef TARTAN_NULLABILITY_CHECKER_H
#define TARTAN_NULLABILITY_CHECKER_H

#include <unordered_set>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>

#include "gir-manager.h"

namespace tartan {

using namespace clang;

class NullabilityVisitor : public RecursiveASTVisitor<NullabilityVisitor> {
public:
	explicit NullabilityVisitor (CompilerInstance& compiler,
	                             std::shared_ptr<const GirManager> gir_manager) :
		_compiler (compiler), _context (compiler.getASTContext ()),
		_gir_manager (gir_manager) {}

private:
	CompilerInstance& _compiler;
	const ASTContext& _context;
	std::shared_ptr<const GirManager> _gir_manager;

public:
	bool TraverseFunctionDecl (FunctionDecl* func);
};

class NullabilityConsumer : public ASTConsumer {
public:
	NullabilityConsumer (CompilerInstance& compiler,
	                     std::shared_ptr<const GirManager> gir_manager,
	                     std::shared_ptr<const std::unordered_set<std::string>> disabled_plugins) :
		_visitor (compiler, gir_manager),
		_disabled_plugins (disabled_plugins) {}

private:
	NullabilityVisitor _visitor;
	std::shared_ptr<const std::unordered_set<std::string>> _disabled_plugins;

public:
	virtual void HandleTranslationUnit (ASTContext& context);
};

} /* namespace tartan */

#endif /* !TARTAN_NULLABILITY_CHECKER_H */
