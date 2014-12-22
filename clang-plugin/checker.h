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

#ifndef TARTAN_CHECKER_H
#define TARTAN_CHECKER_H

#include <unordered_set>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>

#include <girepository.h>

#include "gir-manager.h"

namespace tartan {

using namespace clang;

extern std::shared_ptr<GirManager> global_gir_manager;

class Checker {
public:
	virtual const std::string get_name () const = 0;
};

class ASTChecker : public tartan::Checker,
                   public clang::ASTConsumer {

public:
	explicit ASTChecker (
		CompilerInstance& compiler,
		std::shared_ptr<const GirManager> gir_manager,
		std::shared_ptr<const std::unordered_set<std::string>> disabled_plugins) :
		_compiler (compiler), _gir_manager (gir_manager),
		_disabled_plugins (disabled_plugins) {}

protected:
	CompilerInstance& _compiler;
	std::shared_ptr<const GirManager> _gir_manager;
	std::shared_ptr<const std::unordered_set<std::string>> _disabled_plugins;

public:
	bool is_enabled () const;
};

} /* namespace tartan */

#endif /* !TARTAN_CHECKER_H */
