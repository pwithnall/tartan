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

#ifndef TARTAN_GIR_ATTRIBUTES_H
#define TARTAN_GIR_ATTRIBUTES_H

#include <unordered_set>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>

#include <girepository.h>

#include "checker.h"
#include "gir-manager.h"

namespace tartan {

using namespace clang;

class GirAttributesConsumer : public clang::ASTConsumer {

public:
	explicit GirAttributesConsumer (
		std::shared_ptr<const GirManager> gir_manager) :
		_gir_manager (gir_manager) {}

private:
	std::shared_ptr<const GirManager> _gir_manager;

	void _handle_function_decl (FunctionDecl& func);
public:
	virtual bool HandleTopLevelDecl (DeclGroupRef decl_group);
};


class GirAttributesChecker : public tartan::ASTChecker {

public:
	explicit GirAttributesChecker (
		CompilerInstance& compiler,
		std::shared_ptr<const GirManager> gir_manager,
		std::shared_ptr<const std::unordered_set<std::string>> disabled_plugins) :
		ASTChecker (compiler, gir_manager, disabled_plugins) {}

private:
	void _handle_function_decl (FunctionDecl& func);
public:
	virtual bool HandleTopLevelDecl (DeclGroupRef decl_group);
	const std::string get_name () const { return "gir-attributes"; }
};

} /* namespace tartan */

#endif /* !TARTAN_GIR_ATTRIBUTES_H */
