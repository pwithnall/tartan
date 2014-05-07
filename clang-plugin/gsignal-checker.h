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

#ifndef TARTAN_GSIGNAL_CHECKER_H
#define TARTAN_GSIGNAL_CHECKER_H

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>

#include "gir-manager.h"

using namespace clang;

class GSignalVisitor : public RecursiveASTVisitor<GSignalVisitor> {
public:
	explicit GSignalVisitor (CompilerInstance& compiler,
	                         std::shared_ptr<const GirManager> gir_manager) :
		_compiler (compiler), _context (compiler.getASTContext ()),
		_gir_manager (gir_manager) {}

private:
	CompilerInstance& _compiler;
	const ASTContext& _context;
	std::shared_ptr<const GirManager> _gir_manager;

public:
	bool VisitCallExpr (CallExpr* call);
};

class GSignalConsumer : public ASTConsumer {
public:
	GSignalConsumer (CompilerInstance& compiler,
	                 std::shared_ptr<const GirManager> gir_manager) :
		_visitor (compiler, gir_manager) {}

private:
	GSignalVisitor _visitor;

public:
	virtual void HandleTranslationUnit (ASTContext& context);
};

#endif /* !TARTAN_GSIGNAL_CHECKER_H */
