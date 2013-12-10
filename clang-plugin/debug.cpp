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

#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/CompilerInstance.h>

#include "debug.h"

using namespace clang;

/* Build and emit a warning or error report about the user’s code. */
DiagnosticBuilder
Debug::emit_report (DiagnosticsEngine::Level level, const std::string& message,
                    CompilerInstance& compiler, SourceLocation location)
{
	DiagnosticsEngine& engine = compiler.getDiagnostics ();

	/* Fix up the message levels according to command line
	 * options. */
	if (level == DiagnosticsEngine::Warning &&
	    engine.getWarningsAsErrors ())
		level = DiagnosticsEngine::Error;
	if (level == DiagnosticsEngine::Error &&
	    engine.getErrorsAsFatal ())
		level = DiagnosticsEngine::Fatal;

	const std::string prefixed_message = "[gnome]: " + message;

	if (!location.isValid ()) {
		return engine.Report (engine.getCustomDiagID (level,
		                                              prefixed_message));
	}

	return engine.Report (location,
	                      engine.getCustomDiagID (level,
	                                              prefixed_message));
}

/* Convenience wrappers. */
DiagnosticBuilder
Debug::emit_error (const std::string& message, CompilerInstance& compiler,
                   SourceLocation location)
{
	return Debug::emit_report (DiagnosticsEngine::Error, message,
	                           compiler, location);
}

DiagnosticBuilder
Debug::emit_warning (const std::string& message, CompilerInstance& compiler,
                     SourceLocation location)
{
	return Debug::emit_report (DiagnosticsEngine::Warning, message,
	                           compiler, location);
}
