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
#include <clang/Frontend/MultiplexConsumer.h>
#include <llvm/Support/raw_ostream.h>

#include "gir-attributes.h"
#include "gassert-attributes.h"

using namespace clang;

namespace {

/**
 * Plugin core.
 */
class GnomeAction : public PluginASTAction {
private:
	std::unique_ptr<GirAttributesConsumer> _gir_consumer;
	std::unique_ptr<GAssertAttributesConsumer> _gassert_consumer;

public:
	GnomeAction ()
	{
		this->_gir_consumer =
			std::unique_ptr<GirAttributesConsumer> (
				new GirAttributesConsumer ());
		this->_gassert_consumer =
			std::unique_ptr<GAssertAttributesConsumer> (
				new GAssertAttributesConsumer ());
	}

protected:
	/* Note: This is called before ParseArgs, and must transfer ownership
	 * of the ASTConsumer. */
	ASTConsumer *
	CreateASTConsumer (CompilerInstance &CI, llvm::StringRef)
	{
		std::vector<ASTConsumer*> consumers;
		consumers.push_back (this->_gir_consumer.release ());
		consumers.push_back (this->_gassert_consumer.release ());

		return new MultiplexConsumer (consumers);
	}

private:
	/* Command line parser helper for loading GI namespaces. */
	bool
	_gi_repository_helper (std::vector<std::string>::const_iterator it,
	                       std::vector<std::string>::const_iterator end)
	{
		if (it + 2 >= end) {
			llvm::errs () << "Error: --gi-repository takes two " <<
			                 "arguments: --gi-repository " <<
			                 "[namespace] [version].\n";
			return false;
		}

		std::advance (it, 1);  /* skip --gi-repository */
		std::string gi_namespace = *it;
		std::advance (it, 1);  /* skip namespace */
		std::string gi_version = *it;

		/* Load the repository. */
		GError *error = NULL;

		this->_gir_consumer->load_namespace (gi_namespace, gi_version,
		                                     &error);
		if (error != NULL) {
			llvm::errs () << "Error loading GI repository ‘" <<
			                 gi_namespace << "’ (version " <<
			                 gi_version << "): " <<
			                 error->message << "\n";
			return false;
		}

		return true;
	}

protected:
	/* Parse command line arguments for the plugin. Note: This is called
	 * after CreateASTConsumer. */
	bool
	ParseArgs (const CompilerInstance &CI,
	           const std::vector<std::string>& args)
	{
		unsigned int n_repos = 0;

		/* TODO: Gross hack for testing. */
		const char* fake_args[] = { "--gi-repository", "GLib", "2.0",
		                            "--gi-repository", "Gio", "2.0",
		                            "--gi-repository", "GObject", "2.0",
		                            "--gi-repository", "GModule",
		                            "2.0" };
		std::vector<std::string> v (fake_args, fake_args + 12);
		std::vector<std::string>::const_iterator i = v.begin ();
		this->_gi_repository_helper (i, v.end ());
		this->_gi_repository_helper (i, v.end ());
		return true;

		for (std::vector<std::string>::const_iterator it = args.begin();
		     it != args.end (); ++it) {
			std::string arg = *it;

			if (arg == "--gi-repository") {
				if (!this->_gi_repository_helper (it,
				                                  args.end ()))
					return false;
				n_repos++;
			} else if (arg == "help") {
				this->PrintHelp (llvm::errs ());
			}
		}

		if (n_repos == 0) {
			llvm::errs () << "Error: --gi-repository is required.\n";
			return false;
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
static FrontendPluginRegistry::Add<GnomeAction>
X("gnome", "add attributes and warnings using GNOME-specific metadata");

} /* namespace */
