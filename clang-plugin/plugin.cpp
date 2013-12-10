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

	GirManager _gir_manager;

public:
	GnomeAction ()
	{
		this->_gir_consumer =
			std::unique_ptr<GirAttributesConsumer> (
				new GirAttributesConsumer (this->_gir_manager));
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
	_gi_repository_helper (const CompilerInstance &CI,
	                       std::vector<std::string>::const_iterator it,
	                       std::vector<std::string>::const_iterator end)
	{
		if (it + 1 >= end) {
			DiagnosticsEngine &d = CI.getDiagnostics ();
			unsigned int id = d.getCustomDiagID (
				DiagnosticsEngine::Error,
				"--gi-repository requires a "
				"[namespace]-[version] argument (e.g. "
				"--gi-repository GLib-2.0).");
			d.Report (id);

			return false;
		}

		std::advance (it, 1);  /* skip --gi-repository */
		std::string gi_namespace_and_version = *it;

		/* Try and split up the namespace and version.
		 * e.g. ‘GLib-2.0’ becomes ‘GLib’ and ‘2.0’. */
		std::string::size_type p = gi_namespace_and_version.find ("-");
		if (p == std::string::npos) {
			DiagnosticsEngine &d = CI.getDiagnostics ();
			unsigned int id = d.getCustomDiagID (
				DiagnosticsEngine::Error,
				"--gi-repository requires a "
				"[namespace]-[version] argument (e.g. "
				"--gi-repository GLib-2.0).");
			d.Report (id);

			return false;
		}

		std::string gi_namespace =
			gi_namespace_and_version.substr (0, p);
		std::string gi_version =
			gi_namespace_and_version.substr (p + 1);

		/* Load the repository. */
		GError *error = NULL;

		this->_gir_manager.load_namespace (gi_namespace, gi_version,
		                                   &error);
		if (error != NULL) {
			DiagnosticsEngine &d = CI.getDiagnostics ();
			unsigned int id = d.getCustomDiagID (
				DiagnosticsEngine::Error,
				"Error loading GI repository ‘" + gi_namespace +
				"’ (version " + gi_version + "): " +
				error->message);
			d.Report (id);

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

		for (std::vector<std::string>::const_iterator it = args.begin();
		     it != args.end (); ++it) {
			std::string arg = *it;

			if (arg == "--gi-repository") {
				if (!this->_gi_repository_helper (CI, it,
				                                  args.end ()))
					return false;
				n_repos++;
			} else if (arg == "--help") {
				this->PrintHelp (llvm::errs ());
			}
		}

		if (n_repos == 0) {
			DiagnosticsEngine &d = CI.getDiagnostics ();
			unsigned int id = d.getCustomDiagID (
				DiagnosticsEngine::Error,
				"At least one --gi-repository "
				"[namespace]-[version] argument is required.");
			d.Report (id);

			return false;
		}

		return true;
	}

	/* Print plugin-specific help. */
	void
	PrintHelp (llvm::raw_ostream& out)
	{
		/* TODO: i18n */
		out << "A plugin to enable extra static analysis checks and "
		       "warnings for C code which uses GLib, by making use of "
		       "GIR metadata and other GLib coding conventions.\n"
		       "\n"
		       "Arguments:\n"
		       "    --gi-repository [namespace]-[version]\n"
		       "        Load the GIR metadata for the given version of "
		               "the given namespace.\n"
		       "        Both namespace and "
		               "version are required parameters.\n"
		       "\n"
		       "Usage:\n"
		       "    clang -cc1 -load /path/to/libclang-gnome.so "
		           "-add-plugin gnome \\\n"
		       "        -plugin-arg-gnome --gi-repository\n"
		       "        -plugin-arg-gnome GLib-2.0\n"
		       "        -plugin-arg-gnome --gi-repository\n"
		       "        -plugin-arg-gnome GnomeDesktop-3.0\n";
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
