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

#ifndef GNOME_CLANG_DEBUG_H
#define GNOME_CLANG_DEBUG_H

#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/CompilerInstance.h>

using namespace clang;

namespace Debug {
#ifdef GNOME_ENABLE_DEBUG
#define DEBUG(M) llvm::errs () << M << "\n"
#define DEBUG_EXPR(M, E) llvm::errs () << M; \
	(E).printPretty (llvm::errs (), NULL, context.getPrintingPolicy ()); \
	llvm::errs () << "\n"
#else
#define DEBUG(M)
#define DEBUG_EXPR(M, E)
#endif

/* For use with internal errors, such as unexpected precondition failures or
 * states reached in the plugin internals. Not for user code warnings. */
#define WARN(M) llvm::errs () << "Warning: " << M << "\n"
#define WARN_EXPR(M, E) llvm::errs () << "Warrning: " << M; \
	(E).printPretty (llvm::errs (), NULL, context.getPrintingPolicy ()); \
	llvm::errs () << "\n"

	DiagnosticBuilder emit_report (DiagnosticsEngine::Level level,
	                               const std::string& message,
	                               CompilerInstance& compiler,
	                               SourceLocation location);
	DiagnosticBuilder emit_error (const std::string& message,
	                              CompilerInstance& compiler,
	                              SourceLocation location);
	DiagnosticBuilder emit_warning (const std::string& message,
	                                CompilerInstance& compiler,
	                                SourceLocation location);
}

#endif /* !GNOME_CLANG_DEBUG_H */
