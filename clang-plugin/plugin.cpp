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

#include "config.h"

#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/StaticAnalyzer/Core/CheckerRegistry.h>
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <llvm/Support/raw_ostream.h>

#include "debug.h"
#include "gir-attributes.h"
#include "gassert-attributes.h"
#include "gerror-checker.h"
#include "gsignal-checker.h"
#include "gvariant-checker.h"
#include "nullability-checker.h"

using namespace clang;

namespace tartan {

/* Global GIR manager shared between AST and path-sensitive checkers. */
std::shared_ptr<GirManager> global_gir_manager =
	std::make_shared<GirManager> ();

/**
 * Plugin core.
 */
class TartanAction : public PluginASTAction {
private:
	/* Enabling/Disabling checkers is implemented as a blacklist: all
	 * checkers are enabled by default, unless a --disable-checker argument
	 * specifically disables them (by listing their name in this set). */
	std::shared_ptr<std::unordered_set<std::string>> _disabled_checkers =
		std::make_shared<std::unordered_set<std::string>> ();

	/* Whether to limit output to only diagnostics. */
	enum {
		VERBOSITY_QUIET,
		VERBOSITY_NORMAL,
		VERBOSITY_VERBOSE,
	}_verbosity = VERBOSITY_NORMAL;

protected:
	/* Note: This is called before ParseArgs, and must transfer ownership
	 * of the ASTConsumer. The TartanAction object is destroyed immediately
	 * after this function call returns, so must be careful not to retain
	 * state which is needed by the consumers. */
#ifdef HAVE_LLVM_3_6
	std::unique_ptr<ASTConsumer>
	CreateASTConsumer (CompilerInstance &compiler, llvm::StringRef in_file)
	{
		std::vector<std::unique_ptr<ASTConsumer>> consumers;

		/* Annotaters. */
		consumers.push_back (std::unique_ptr<ASTConsumer> (
			new GirAttributesConsumer (global_gir_manager)));
		consumers.push_back (std::unique_ptr<ASTConsumer> (
			new GAssertAttributesConsumer ()));

		/* Checkers. */
		consumers.push_back (std::unique_ptr<ASTConsumer> (
			new NullabilityConsumer (compiler,
			                         global_gir_manager,
			                         this->_disabled_checkers)));
		consumers.push_back (std::unique_ptr<ASTConsumer> (
			new GVariantConsumer (compiler,
			                      global_gir_manager,
			                      this->_disabled_checkers)));
		consumers.push_back (std::unique_ptr<ASTConsumer> (
			new GSignalConsumer (compiler,
			                     global_gir_manager,
			                     this->_disabled_checkers)));
		consumers.push_back (std::unique_ptr<ASTConsumer> (
			new GirAttributesChecker (compiler,
			                          global_gir_manager,
			                          this->_disabled_checkers)));

		return llvm::make_unique<MultiplexConsumer> (std::move (consumers));
	}
#else /* if !HAVE_LLVM_3_6 */
	ASTConsumer *
	CreateASTConsumer (CompilerInstance &compiler, llvm::StringRef in_file)
	{
		std::vector<ASTConsumer*> consumers;

		/* Annotaters. */
		consumers.push_back (
			new GirAttributesConsumer (global_gir_manager));
		consumers.push_back (
			new GAssertAttributesConsumer ());

		/* Checkers. */
		consumers.push_back (
			new NullabilityConsumer (compiler,
			                         global_gir_manager,
			                         this->_disabled_checkers));
		consumers.push_back (
			new GVariantConsumer (compiler,
			                      global_gir_manager,
			                      this->_disabled_checkers));
		consumers.push_back (
			new GSignalConsumer (compiler,
			                     global_gir_manager,
			                     this->_disabled_checkers));
		consumers.push_back (
			new GirAttributesChecker (compiler,
			                          global_gir_manager,
			                          this->_disabled_checkers));

		return new MultiplexConsumer (consumers);
	}
#endif /* !HAVE_LLVM_3_6 */

private:
	bool
	_load_typelib (const CompilerInstance &CI,
	               const std::string& gi_namespace_and_version)
	{
		std::string::size_type p = gi_namespace_and_version.find ("-");

		if (p == std::string::npos) {
			/* Ignore it — probably a non-typelib file. */
			return false;
		}

		std::string gi_namespace =
			gi_namespace_and_version.substr (0, p);
		std::string gi_version =
			gi_namespace_and_version.substr (p + 1);

		DEBUG ("Loading typelib " + gi_namespace + " " + gi_version);

		/* Load the repository. */
		GError *error = NULL;

		global_gir_manager.get ()->load_namespace (gi_namespace,
		                                           gi_version,
		                                           &error);
		if (error != NULL &&
		    !g_error_matches (error, G_IREPOSITORY_ERROR,
		                      G_IREPOSITORY_ERROR_NAMESPACE_VERSION_CONFLICT)) {
			DiagnosticsEngine &d = CI.getDiagnostics ();
			DiagnosticIDs &ids = *d.getDiagnosticIDs ();
			unsigned int id = ids.getCustomDiagID (
				(DiagnosticIDs::Level) DiagnosticsEngine::Warning,
				"Fail to load GI repository ‘" + gi_namespace +
				"’ (version " + gi_version + "): " +
				error->message);
			d.Report (id);

			g_error_free (error);

			return false;
		}

		g_clear_error (&error);

		return true;
	}

	/* Load all the GI typelibs we can find. This shouldn’t take long, and
	 * saves the user having to specify which typelibs to use (or us having
	 * to try and work out which ones the user’s code uses by looking at
	 * #included files). */
	bool
	_load_gi_repositories (const CompilerInstance &CI)
	{
		GSList/*<unowned string>*/ *typelib_paths, *l;

		typelib_paths = g_irepository_get_search_path ();

		for (l = typelib_paths; l != NULL; l = l->next) {
			GDir *dir;
			const gchar *typelib_path, *typelib_filename;
			GError *error = NULL;

			typelib_path = (const gchar *) l->data;
			dir = g_dir_open (typelib_path, 0, &error);

			if (error != NULL) {
				/* Warn about the bogus include path and
				 * continue. */
				DiagnosticsEngine &d = CI.getDiagnostics ();

				unsigned int id = d.getCustomDiagID (
					DiagnosticsEngine::Warning,
					"Error opening typelib path ‘%0’: %1");
				d.Report (id)
					<< typelib_path
					<< error->message;

				continue;
			}

			while ((typelib_filename = g_dir_read_name (dir)) != NULL) {
				/* Load the typelib. Ignore failure. */

				std::string _typelib_filename (typelib_filename);
				std::string::size_type last_dot = _typelib_filename.find_last_of (".");
				if (last_dot == std::string::npos) {
					/* No ‘.typelib’ suffix — ignore. */
					continue;
				}

				std::string gi_namespace_and_version = _typelib_filename.substr (0, last_dot);
				this->_load_typelib (CI, gi_namespace_and_version);
			}

			g_dir_close (dir);
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
		/* Load all typelibs. */
		this->_load_gi_repositories (CI);

		/* Enable the default set of checkers. */
		for (std::vector<std::string>::const_iterator it = args.begin();
		     it != args.end (); ++it) {
			std::string arg = *it;

			if (arg == "--help") {
				this->PrintHelp (llvm::outs ());
			} else if (arg == "--quiet") {
				this->_verbosity = VERBOSITY_QUIET;
			} else if (arg == "--verbose") {
				this->_verbosity = VERBOSITY_VERBOSE;
			} else if (arg == "--enable-checker") {
				const std::string checker = *(++it);
				if (checker == "all") {
					this->_disabled_checkers.get ()->clear ();
				} else {
					this->_disabled_checkers.get ()->erase (std::string (checker));
				}
			} else if (arg == "--disable-checker") {
				const std::string checker = *(++it);
				this->_disabled_checkers.get ()->insert (std::string (checker));
			}
		}

		/* Listen to the V environment variable (as standard in automake) too. */
		const char *v_value = getenv ("V");
		if (v_value != NULL && strcmp (v_value, "0") == 0) {
			this->_verbosity = VERBOSITY_QUIET;
		}

		/* Output a version message. */
		if (this->_verbosity > VERBOSITY_NORMAL) {
			llvm::outs () << "Tartan version " << VERSION << " "
			                 "compiled for LLVM " <<
			                 LLVM_CONFIG_VERSION << ".\n" <<
			                 "Disabled checkers: ";

			for (std::unordered_set<std::string>::const_iterator it = this->_disabled_checkers.get ()->begin ();
			     it != this->_disabled_checkers.get ()->end (); ++it) {
				std::string checker = *it;

				if (it != this->_disabled_checkers.get ()->begin ()) {
					llvm::outs () << ", ";
				}
				llvm::outs () << checker;
			}
			if (this->_disabled_checkers.get ()->begin () ==
			    this->_disabled_checkers.get ()->end ()) {
				llvm::outs () << "(none)";
			}

			llvm::outs () << "\n";
		}

		return true;
	}

	/* Print plugin-specific help. */
	void
	PrintHelp (llvm::raw_ostream& out)
	{
		/* TODO: i18n */
		out << "A plugin to enable extra static analysis checks and "
		       "warnings for C code which\nuses GLib, by making use of "
		       "GIR metadata and other GLib coding conventions.\n"
		       "\n"
		       "Arguments:\n"
		       "    --enable-checker [name]\n"
		       "        Enable the given Tartan checker, which may be "
		               "‘all’. All checkers are\n"
		       "        enabled by default.\n"
		       "    --disable-checker [name]\n"
		       "        Disable the given Tartan checker, which may be "
		               "‘all’. All checkers are\n"
		       "        enabled by default.\n"
		       "    --quiet\n"
		       "        Disable all plugin output except code "
		               "diagnostics (remarks,\n"
		       "        warnings and errors).\n"
		       "    --verbose\n"
		       "        Output additional versioning information.\n"
		       "\n"
		       "Usage:\n"
		       "    clang -cc1 -load /path/to/libtartan.so "
		           "-add-plugin tartan \\\n"
		           "-analyzer-checker tartan\\\n"
		       "        -plugin-arg-tartan --disable-checker \\\n"
		       "        -plugin-arg-tartan all \\\n"
		       "        -plugin-arg-tartan --enable-checker \\\n"
		       "        -plugin-arg-tartan gir-attributes\n";
	}

	bool
	shouldEraseOutputFiles ()
	{
		/* TODO: Make this conditional on an error occurring. */
		return false;
	}
};


/* Register the AST checkers with LLVM. */
static FrontendPluginRegistry::Add<TartanAction>
X("tartan", "add attributes and warnings using GLib-specific metadata");

/* Register the path-dependent plugins with Clang. */
extern "C"
void clang_registerCheckers (ento::CheckerRegistry &registry) {
	registry.addChecker<GErrorChecker> ("tartan.GErrorChecker",
	                                    "Check GError API usage");
}

extern "C"
const char clang_analyzerAPIVersionString[] = CLANG_ANALYZER_API_VERSION_STRING;

} /* namespace tartan */
