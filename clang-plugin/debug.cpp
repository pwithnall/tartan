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

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/StaticAnalyzer/Core/BugReporter/BugType.h>
#include <clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h>

#include "debug.h"

using namespace clang;
using namespace ento;

void
Debug::emit_bug_report (std::unique_ptr<BugReport> report,
                        CheckerContext &context)
{
	#ifndef HAVE_LLVM_3_7
	context.emitReport (report.get ());
	#else
	context.emitReport (std::move (report));
	#endif
}

/* Build and emit a warning or error report about the user’s code. */
DiagnosticBuilder
Debug::emit_report (DiagnosticsEngine::Level level, const char *format_string,
                    CompilerInstance& compiler, SourceLocation location)
{
	DiagnosticsEngine& engine = compiler.getDiagnostics ();
	DiagnosticIDs& ids = *engine.getDiagnosticIDs ();

	/* Fix up the message levels according to command line
	 * options. */
	if (level == DiagnosticsEngine::Warning &&
	    engine.getWarningsAsErrors ())
		level = DiagnosticsEngine::Error;
	if (level == DiagnosticsEngine::Error &&
	    engine.getErrorsAsFatal ())
		level = DiagnosticsEngine::Fatal;

	/* Add a prefix. */
	std::string prefixed_format_string =
		"[tartan]: " + std::string (format_string);

	unsigned diag_id = ids.getCustomDiagID ((DiagnosticIDs::Level) level,
	                                        prefixed_format_string);

	if (!location.isValid ()) {
		return engine.Report (diag_id);
	}

	return engine.Report (location, diag_id);
}

/* Convenience wrappers. */
DiagnosticBuilder
Debug::emit_error (const char *format_string, CompilerInstance& compiler,
                   SourceLocation location)
{
	return Debug::emit_report (DiagnosticsEngine::Error, format_string,
	                           compiler, location);
}

DiagnosticBuilder
Debug::emit_warning (const char *format_string, CompilerInstance& compiler,
                     SourceLocation location)
{
	return Debug::emit_report (DiagnosticsEngine::Warning, format_string,
	                           compiler, location);
}

DiagnosticBuilder
Debug::emit_remark (const char *format_string, CompilerInstance& compiler,
                    SourceLocation location)
{
#if (CLANG_VERSION_MAJOR > 3) || \
    (CLANG_VERSION_MAJOR == 3 && CLANG_VERSION_MINOR > 4)
	return Debug::emit_report (DiagnosticsEngine::Remark, format_string,
	                           compiler, location);
#else
	return Debug::emit_report (DiagnosticsEngine::Warning, format_string,
	                           compiler, location);
#endif
}

/* Well-known strings used for the category of Tartan static analysis issues. */
namespace Debug { namespace Categories {
	const char * const GError = "GError API";
}}
