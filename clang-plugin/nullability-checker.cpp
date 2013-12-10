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

/**
 * NullabilityVisitor:
 *
 * This AST visitor inspects function declarations, checking the nullability of
 * their parameters is correctly annotated. It checks for the presence of:
 *  • A nonnull attribute on the function.
 *  • (allow-none) annotations on the parameters.
 *  • g_return[_val]_if_fail() precondition assertions in the function body.
 *
 * It then checks that the assertions implied by these three sources agree, and
 * that a reasonable number of the sources exist. If two sources conflict (e.g.
 * a parameter has an (allow-none) attribute but also has a
 * g_return_if_fail(param != NULL) precondition assertion), an error is emitted.
 * Otherwise, if the nullability of a parameter is uncertain, warnings are
 * emitted to ask the user to add a precondition assertion or an (allow-none)
 * annotation. The user is only optionally requested to add nonnull attributes,
 * as many users have voiced opinions that such attributes are ugly.
 *
 * Note that precondition assertions can only be used for non-NULL checks by the
 * static analyser within a single code base. For analysis across code bases,
 * (allow-none) annotations or nonnull attributes are needed. Hence the warnings
 * emitted by this checker try to gently push the user towards adding them.
 */

#include <unordered_set>

#include <girepository.h>
#include <gitypes.h>

#include <clang/AST/Attr.h>

#include "assertion-extracter.h"
#include "debug.h"
#include "nullability-checker.h"

void
NullabilityConsumer::HandleTranslationUnit (ASTContext& context)
{
	this->_visitor.TraverseDecl (context.getTranslationUnitDecl ());
}

/* Note: Specifically overriding the Traverse* method here to re-implement
 * recursion to child nodes. */
bool
NullabilityVisitor::TraverseFunctionDecl (FunctionDecl* func)
{
	/* Ignore static functions immediately; they shouldn’t have any
	 * GIR data, and searching for it massively slows down
	 * compilation. */
	StorageClass sc = func->getStorageClass ();
	if (sc != SC_None && sc != SC_Extern)
		return true;

	/* Can only handle functions which have a body defined. */
	Stmt* func_body = func->getBody ();
	if (func_body == NULL)
		return true;

	/* The body should be a compound statement, e.g.
	 * { stmt; stmt; } */
	CompoundStmt* body_stmt = dyn_cast<CompoundStmt> (func_body);
	if (body_stmt == NULL) {
		DEBUG ("Ignoring function " << func->getNameAsString () <<
		       " due to having a non-compound statement body.");
		return true;
	}

	/* For each parameter, check whether it has an (allow-none) annotation,
	 * a nonnull attribute, and a non-NULL assertion. */
	NonNullAttr* nonnull_attr = func->getAttr<NonNullAttr> ();

	/* Try to find typelib information about the function. */
	std::string func_name = func->getNameAsString ();  /* TODO: expensive? */
	GIBaseInfo* info = this->_gir_manager.find_function_info (func_name);

	if (info == NULL)
		return true;

	if (g_base_info_get_type (info) != GI_INFO_TYPE_FUNCTION) {
		llvm::errs () << "Error: Unhandled GI type " <<
		                 g_base_info_get_type (info) <<
		                 " in introspection info for function ‘" <<
		                 func_name << "’.\n";
		return true;
	}

	/* Parse the function’s body for assertions. */
	DEBUG ("Examining " << func->getNameAsString ());
	std::unordered_set<const ValueDecl*> asserted_parms;
	ASTContext& context = func->getASTContext ();

	/* Iterate through the function body until the first non-assertion and
	 * non-declaration statement is reached. Specifically stop before the
	 * first assignment, as that could affect the outcome of any subsequent
	 * assertions. */
	for (CompoundStmt::const_body_iterator it = body_stmt->body_begin (),
	     ie = body_stmt->body_end (); it != ie; ++it) {
		Stmt* st = *it;

		Expr* assertion_expr =
			AssertionExtracter::is_assertion_stmt (*st, context);

		if (assertion_expr == NULL) {
			/* Potential program state mutation reached, so run
			 * away. */
			break;
		}

		/* If the assertion is a non-NULL check, record the parameters
		 * it checks. */
		AssertionExtracter::assertion_is_nonnull_check (*assertion_expr,
		                                                context,
		                                                asserted_parms);
	}

	DEBUG ("");

	GICallableInfo *callable_info = (GICallableInfo *) info;

	/* Handle the parameters. */
	for (FunctionDecl::param_const_iterator it = func->param_begin (),
	     ie = func->param_end (); it != ie; ++it) {
		ParmVarDecl* parm_decl = *it;
		unsigned int idx = parm_decl->getFunctionScopeIndex ();
		GIArgInfo arg;
		GITypeInfo type_info;

		/* Skip non-pointer arguments. */
		if (!parm_decl->getType ()->isPointerType ())
			continue;

		g_callable_info_load_arg (callable_info, idx, &arg);
		g_arg_info_load_type (&arg, &type_info);

		enum {
			EXPLICIT_NULLABLE,  /* 0 */
			MAYBE,  /* ? */
			EXPLICIT_NONNULL,  /* 1 */
		} has_nonnull =
			(nonnull_attr == NULL) ? MAYBE :
			(nonnull_attr->isNonNull (idx)) ?
				EXPLICIT_NONNULL: EXPLICIT_NULLABLE;
		bool has_allow_none = g_arg_info_may_be_null (&arg);
		bool has_assertion = (asserted_parms.count (parm_decl) > 0);

		/* Analysis:
		 *
		 * nonnull | allow-none | assertion | Outcome
		 * --------+------------+-----------+-------------
		 *       0 |          0 |         0 | Warning
		 *       0 |          0 |         1 | Warning
		 *       0 |          1 |         0 | Perfect
		 *       0 |          1 |         1 | Error
		 *       ? |          0 |         0 | Warning
		 *       ? |          0 |         1 | Soft warning
		 *       ? |          1 |         0 | Perfect
		 *       ? |          1 |         1 | Error
		 *       1 |          0 |         0 | Warning
		 *       1 |          0 |         1 | Perfect
		 *       1 |          1 |         0 | Error
		 *       1 |          1 |         1 | Error
		 *
		 * Which gives the following rules:
		 *      nonnull ∧ allow-none ⇒ Error (conflict)
		 *      allow-none ∧ assertion ⇒ Error (conflict)
		 *      ¬allow-none ∧ ¬assertion ⇒ Warning (need one or other)
		 *      ¬nonnull ∧ assertion ⇒ Warning (incomplete nonnull)
		 *      (nonnull = ?) ∧ assertion ⇒
		 *          Soft warning (missing nonnull)
		 */
		if (has_nonnull == EXPLICIT_NONNULL && has_allow_none) {
			Debug::emit_error (
				"Conflict between nonnull attribute and "
				"(allow-none) annotation on the ‘" +
				parm_decl->getNameAsString () +
				"’ parameter of function " +
				func->getNameAsString () + "().",
				this->_compiler,
				parm_decl->getLocStart ());
		} else if (has_allow_none && has_assertion) {
			Debug::emit_error (
				"Conflict between (allow-none) annotation and "
				"non-NULL precondition assertion on the ‘" +
				parm_decl->getNameAsString () +
				"’ parameter of function " +
				func->getNameAsString () + "().",
				this->_compiler,
				parm_decl->getLocStart ());
		} else if (!has_allow_none && !has_assertion) {
			switch (has_nonnull) {
			case EXPLICIT_NULLABLE:
				Debug::emit_warning (
					"Missing (allow-none) annotation on "
					"the ‘" +
					parm_decl->getNameAsString () +
					"’ parameter of function " +
					func->getNameAsString () + "() "
					"(already has a nonnull attribute).",
					this->_compiler,
					parm_decl->getLocStart ());
				break;
			case MAYBE:
				Debug::emit_warning (
					"Missing (allow-none) annotation or "
					"non-NULL precondition assertion on "
					"the ‘" +
					parm_decl->getNameAsString () +
					"’ parameter of function " +
					func->getNameAsString () + "().",
					this->_compiler,
					parm_decl->getLocStart ());
				break;
			case EXPLICIT_NONNULL:
				Debug::emit_warning (
					"Missing non-NULL precondition "
					"assertion on the ‘" +
					parm_decl->getNameAsString () +
					"’ parameter of function " +
					func->getNameAsString () + "() "
					"(already has a nonnull attribute).",
					this->_compiler,
					parm_decl->getLocStart ());
				break;
			}
		} else if (has_nonnull == EXPLICIT_NULLABLE && has_assertion) {
			Debug::emit_warning (
				"Conflict between nonnull attribute and "
				"non-NULL precondition annotation on the ‘" +
				parm_decl->getNameAsString () +
				"’ parameter of function " +
				func->getNameAsString () + "().",
				this->_compiler,
				parm_decl->getLocStart ());
		} else if (has_nonnull == MAYBE && has_assertion) {
			/* TODO: Make this a soft warning (disabled by default)
			 * if it comes up with too many false positives. */
			Debug::emit_warning (
				"Missing nonnull attribute for the ‘" +
				parm_decl->getNameAsString () +
				"’ parameter of function " +
				func->getNameAsString () + "() "
				"(already has a non-NULL precondition "
				"assertion).",
				this->_compiler,
				parm_decl->getLocStart ());
		}
	}

	g_base_info_unref (info);

	return true;
}
