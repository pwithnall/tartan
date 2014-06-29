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

#ifndef TARTAN_DEBUG_H
#define TARTAN_DEBUG_H

#include <llvm/Support/Debug.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/CompilerInstance.h>

using namespace clang;

namespace Debug {
#ifndef NDEBUG

#undef DEBUG
#define DEBUG(M) DEBUG_WITH_TYPE("tartan", llvm::dbgs () << M << "\n")

#define DEBUG_EXPR(M, E) DEBUG_WITH_TYPE ("tartan", \
	llvm::dbgs () << M; \
	(E).printPretty (llvm::dbgs (), NULL, context.getPrintingPolicy ()); \
	llvm::dbgs () << "\n")

#else
#define DEBUG(M)
#define DEBUG_EXPR(M, E)
#endif

/* For use with internal errors, such as unexpected precondition failures or
 * states reached in the plugin internals. Not for user code warnings. */
#define WARN(M) llvm::errs () << "Warning: " << M << "\n"
#define WARN_EXPR(M, E) llvm::errs () << "Warning: " << M << " in:\n\t"; \
	(E).printPretty (llvm::errs (), NULL, context.getPrintingPolicy ()); \
	llvm::errs () << "\n"

	DiagnosticBuilder emit_report (DiagnosticsEngine::Level level,
	                               const char *format_string,
	                               CompilerInstance& compiler,
	                               SourceLocation location);
	DiagnosticBuilder emit_error (const char *format_string,
	                              CompilerInstance& compiler,
	                              SourceLocation location);
	DiagnosticBuilder emit_warning (const char *format_string,
	                                CompilerInstance& compiler,
	                                SourceLocation location);
	DiagnosticBuilder emit_remark (const char *format_string,
	                               CompilerInstance& compiler,
	                               SourceLocation location);
}

#endif /* !TARTAN_DEBUG_H */
