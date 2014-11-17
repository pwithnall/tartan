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

#ifndef TARTAN_GERROR_CHECKER_H
#define TARTAN_GERROR_CHECKER_H

#include <clang/AST/AST.h>
#include <clang/StaticAnalyzer/Core/BugReporter/BugType.h>
#include <clang/StaticAnalyzer/Core/Checker.h>
#include <clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h>
#include <clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h>

#include "checker.h"
#include "gir-manager.h"

namespace tartan {

using namespace clang;
using namespace ento;

class GErrorChecker : public ento::Checker<check::PreCall,
                                           eval::Call,
                                           check::Bind,
                                           check::DeadSymbols>,
                      public tartan::Checker {
public:
	explicit GErrorChecker () {};

	struct GErrorChecksFilter {
		DefaultBool check_overwrite_set;
		DefaultBool check_overwrite_freed;
		DefaultBool check_double_free;
		DefaultBool check_free_cleared;
		DefaultBool check_use_uninitialised;
		DefaultBool check_memory_leak;

		CheckName check_name_overwrite_set;
		CheckName check_name_overwrite_freed;
		CheckName check_name_double_free;
		CheckName check_name_free_cleared;
		CheckName check_name_use_uninitialised;
		CheckName check_name_memory_leak;
	};

	GErrorChecksFilter filter;

private:
	/* Cached type identifiers. */
	mutable QualType _gerror_type;

	/* Cached function identifiers. */
	mutable IdentifierInfo *_identifier_g_set_error;
	mutable IdentifierInfo *_identifier_g_set_error_literal;
	mutable IdentifierInfo *_identifier_g_error_new;
	mutable IdentifierInfo *_identifier_g_error_new_literal;
	mutable IdentifierInfo *_identifier_g_error_new_valist;
	mutable IdentifierInfo *_identifier_g_error_free;
	mutable IdentifierInfo *_identifier_g_clear_error;
	mutable IdentifierInfo *_identifier_g_propagate_error;
	mutable IdentifierInfo *_identifier_g_propagate_prefixed_error;

	void _initialise_identifiers (const ASTContext &context) const;

	/* Cached bug reports. */
	mutable std::unique_ptr<BuiltinBug> _overwrite_set;
	mutable std::unique_ptr<BuiltinBug> _overwrite_freed;
	mutable std::unique_ptr<BuiltinBug> _double_free;
	mutable std::unique_ptr<BuiltinBug> _free_cleared;
	mutable std::unique_ptr<BuiltinBug> _use_uninitialised;
	mutable std::unique_ptr<BuiltinBug> _memory_leak;

	void _initialise_bug_reports () const;

	ProgramStateRef _handle_pre_g_set_error (CheckerContext &context,
	                                         const CallEvent &call_event) const;
	ProgramStateRef _handle_pre_g_error_new (CheckerContext &context,
	                                         const CallEvent &call_event) const;
	ProgramStateRef _handle_pre_g_error_free (CheckerContext &context,
	                                          const CallEvent &call_event) const;
	ProgramStateRef _handle_pre_g_clear_error (CheckerContext &context,
	                                           const CallEvent &call_event) const;
	ProgramStateRef _handle_pre_g_propagate_error (CheckerContext &context,
	                                               const CallEvent &call_event) const;

	ProgramStateRef _handle_eval_g_set_error (CheckerContext &context,
	                                          const CallExpr &call_expr) const;
	ProgramStateRef _handle_eval_g_error_new (CheckerContext &context,
	                                          const CallExpr &call_expr) const;
	ProgramStateRef _handle_eval_g_error_free (CheckerContext &context,
	                                           const CallExpr &call_expr) const;
	ProgramStateRef _handle_eval_g_clear_error (CheckerContext &context,
	                                            const CallExpr &call_expr) const;
	ProgramStateRef _handle_eval_g_propagate_error (CheckerContext &context,
	                                                const CallExpr &call_expr) const;

	bool _assert_gerror_set (SVal error_location,
	                         bool null_allowed,
	                         ProgramStateRef state,
	                         CheckerContext &context,
	                         const SourceRange &source_range) const;
	bool _assert_gerror_unset (SVal error_location,
	                           bool undef_allowed,
	                           ProgramStateRef state,
	                           CheckerContext &context,
	                           const SourceRange &source_range) const;
	bool _assert_gerror_ptr_clear (SVal error_location,
	                               ProgramStateRef state,
	                               CheckerContext &context,
	                               const SourceRange &source_range) const;
	bool _assert_code_in_domain (SVal domain,
	                             SVal code,
	                             ProgramStateRef state,
	                             CheckerContext &context,
	                             const SourceRange &domain_source_range,
	                             const SourceRange &code_source_range) const;

	ProgramStateRef _gerror_new (const Expr *call_expr,
	                             bool bind_to_call,
	                             DefinedSVal **allocated_sval,
	                             ProgramStateRef state,
	                             CheckerContext &context,
	                             const SourceRange &source_range) const;
	ProgramStateRef _set_gerror (SVal error_location,
	                             DefinedSVal new_error,
	                             ProgramStateRef state,
	                             CheckerContext &context,
	                             const SourceRange &source_range) const;
	ProgramStateRef _clear_gerror (SVal error_location,
	                               ProgramStateRef state,
	                               CheckerContext &context,
	                               const SourceRange &source_range) const;
	ProgramStateRef _gerror_free (SVal error_location,
	                              ProgramStateRef state,
	                              CheckerContext &context,
	                              const SourceRange &source_range) const;

	SVal _error_from_error_ptr (SVal ptr_error_location,
	                            CheckerContext &context) const;

public:
	void checkPreCall (const CallEvent &call,
	                   CheckerContext &context) const;
	bool evalCall (const CallExpr *call,
	               CheckerContext &context) const;
	void checkBind (SVal loc, SVal val, const Stmt *stmt,
	                CheckerContext &context) const;
	void checkDeadSymbols (SymbolReaper &symbol_reaper,
	                       CheckerContext &context) const;

	const std::string get_name () const { return "gerror"; }
};

} /* namespace tartan */

#endif /* !TARTAN_GERROR_CHECKER_H */
