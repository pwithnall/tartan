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

#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

#include "gir-attributes.h"

using namespace clang;

namespace {

/**
 * Plugin core.
 */
class GirAttributesAction : public PluginASTAction {
private:
	std::string _gi_namespace;
	std::string _gi_version;

protected:
	ASTConsumer *
	CreateASTConsumer (CompilerInstance &CI, llvm::StringRef)
	{
		GError *error = NULL;

		/* TODO: Sort of need to be able to specify multiple namespaces. */
		GirAttributesConsumer *consumer =
			new GirAttributesConsumer (this->_gi_namespace,
			                           this->_gi_version);

		consumer->prepare (&error);
		if (error != NULL) {
			llvm::errs () << "Error loading GIR typelib ‘" <<
			                 this->_gi_namespace << "’ (version " <<
			                 this->_gi_version << "): " <<
			                 error->message;
			return NULL;
		}

		return consumer;
	}

	/* Parse command line arguments for the plugin.
	 * TODO: Called before CreateASTConsumer? */
	bool
	ParseArgs (const CompilerInstance &CI,
	           const std::vector<std::string>& args)
	{
		/* TODO: Gross hack for testing. */
		this->_gi_namespace = "GLib";
		this->_gi_version = "2.0";
		return true;

		for (unsigned int i = 0, e = args.size (); i < e; i++) {
			if (args[i] == "-gi-namespace") {
				if (i + 1 >= e) {
					/* TODO: Error */
				}

				this->_gi_namespace = args[i + 1];
			} else if (args[i] == "-gi-version") {
				if (i + 1 >= e) {
					/* TODO: Error */
				}

				this->_gi_version = args[i + 1];
			} else if (args[i] == "help") {
				this->PrintHelp (llvm::errs ());
			}
		}

		if (this->_gi_namespace.empty ()) {
			llvm::errs () << "Error: -gi-namespace is required.";
			/* TODO */
		}

		return true;
	}

	/* Print plugin-specific help. */
	void
	PrintHelp (llvm::raw_ostream& out)
	{
		/* TODO: i18n */
		out << "A plugin to add C code attributes from GIR annotation "
		       "data. This plugin currently accepts no arguments.\n";
	}

	bool
	shouldEraseOutputFiles ()
	{
		/* TODO: Make this conditional on an error occurring. */
		return false;
	}
};


/* Register the plugin with LLVM. */
static FrontendPluginRegistry::Add<GirAttributesAction>
X("gir-attributes", "add attributes from GIR data");

} /* namespace */
