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

#include <unordered_set>

#include <clang/AST/Attr.h>
#include <clang/Lex/Lexer.h>

#include "assertion-extracter.h"
#include "debug.h"

static bool
_is_assertion_name (const std::string& name)
{
	return (name == "g_return_if_fail" ||
	        name == "g_return_val_if_fail" ||
	        name == "g_assert_cmpstr" ||
	        name == "g_assert_cmpint" ||
	        name == "g_assert_cmpuint" ||
	        name == "g_assert_cmphex" ||
	        name == "g_assert_cmpfloat" ||
	        name == "g_assert_no_error" ||
	        name == "g_assert_error" ||
	        name == "g_assert_true" ||
	        name == "g_assert_false" ||
	        name == "g_assert_null" ||
	        name == "g_assert_nonnull" ||
	        name == "g_assert_not_reached" ||
	        name == "g_assert" ||
	        name == "assert" ||
	        name == "assert_perror");
}

static bool
_is_assertion_fail_func_name (const std::string& name)
{
	return (name == "g_return_if_fail_warning" ||
	        name == "g_assertion_message_cmpstr" ||
	        name == "g_assertion_message_cmpnum" ||
	        name == "g_assertion_message_error" ||
	        name == "g_assertion_message" ||
	        name == "g_assertion_message_expr" ||
	        name == "__assert_fail" ||
	        name == "__assert_perror_fail");
}

/* Return the negation of the given expression. */
static Expr*
_negation_expr (Expr* e, const ASTContext& context)
{
	return new (context)
		UnaryOperator (e, UnaryOperatorKind::UO_LNot,
		               context.getLogicalOperationType (),
		               VK_RValue, OK_Ordinary, SourceLocation ());
}

/* Combine expressions A and B to give (A && B). */
static Expr*
_conjunction_expr (Expr* lhs, Expr* rhs, const ASTContext& context)
{
	return new (context)
		BinaryOperator (lhs, rhs, BinaryOperatorKind::BO_LAnd,
		                context.getLogicalOperationType (),
		                VK_RValue, OK_Ordinary, SourceLocation (),
		                false);
}

/* Combine expressions A and B to give (A || B). */
static Expr*
_disjunction_expr (Expr* lhs, Expr* rhs, const ASTContext& context)
{
	return new (context)
		BinaryOperator (lhs, rhs, BinaryOperatorKind::BO_LOr,
		                context.getLogicalOperationType (),
		                VK_RValue, OK_Ordinary, SourceLocation (),
		                false);
}

/* Does the given statement look like:
 *  • g_return_if_fail(…)
 *  • g_return_val_if_fail(…)
 *  • g_assert(…)
 *  • g_assert_*(…)
 *  • assert(…)
 * This is complicated by the fact that if the gmessages.h header isn’t
 * available, they’ll present as CallExpr function calls with those names; if it
 * is available, they’ll be expanded as macros and turn into DoStmts with misc.
 * rubbish beneath.
 *
 * If the statement changes program state at all, return NULL. Otherwise, return
 * the condition which holds for the assertion to be bypassed (i.e. for the
 * assertion to succeed). This function is built recursively, building a boolean
 * expression for the condition based on avoiding branches which call
 * abort()-like functions.
 *
 * This function is based on a transformation of the AST to an augmented boolean
 * expression, using rules documented in each switch case. In this
 * documentation, calc(S) refers to the transformation function. The augmented
 * boolean expressions can be either NULL, or a normal boolean expression
 * (TRUE, FALSE, ∧, ∨, ¬). NULL is used iff the statement potentially changes
 * program state, and poisons any boolean expression:
 *     B ∧ NULL ≡ NULL
 *     B ∨ NULL ≡ NULL
 *     ¬NULL ≡ NULL
 */
Expr*
AssertionExtracter::is_assertion_stmt (Stmt& stmt, const ASTContext& context)
{
	DEBUG ("Checking " << stmt.getStmtClassName () << " for assertions.");

	/* Slow path: walk through the AST, aborting on statements which
	 * potentially mutate program state, and otherwise trying to find a base
	 * function call such as:
	 *  • g_return_if_fail_warning()
	 *  • g_assertion_message()
	 *  • g_assertion_message_*()
	 */
	switch (stmt.getStmtClass ()) {
	case Stmt::StmtClass::CallExprClass: {
		/* Handle a direct function call.
		 * Transformations:
		 *     [g_return_if_fail|assert|…](C) ↦ C
		 *     [g_return_if_fail_warning|__assert_fail|…](C) ↦ FALSE
		 *     other_funcs(…) ↦ NULL */
		CallExpr& call_expr = cast<CallExpr> (stmt);
		FunctionDecl* func = call_expr.getDirectCallee ();
		if (func == NULL)
			return NULL;

		std::string func_name = func->getNameAsString ();
		DEBUG ("CallExpr to function " << func_name);

		if (_is_assertion_name (func_name)) {
			/* Assertion path where the compiler hasn't seen the
			 * definition of the assertion macro, so still thinks
			 * it's a function.
			 *
			 * Extract the assertion condition as the first function
			 * parameter.
			 *
			 * TODO: May need to fix up the condition for macros
			 * like g_assert_null(). */
			return call_expr.getArg (0);
		} else if (_is_assertion_fail_func_name (func_name)) {
			/* Assertion path where the assertion macro has been
			 * expanded and we're on the assertion failure branch.
			 *
			 * In this case, the assertion condition has been
			 * grabbed from an if statement already, so negate it
			 * (to avoid the failure condition) and return. */
			return new (context)
				IntegerLiteral (context, context.MakeIntValue (0, context.getLogicalOperationType ()),
				                context.getLogicalOperationType (),
				                SourceLocation ());
		}

		/* Not an assertion path. */
		return NULL;
	}
	case Stmt::StmtClass::DoStmtClass: {
		/* Handle a do { … } while (0) block (commonly used to allow
		 * macros to optionally be suffixed by a semicolon).
		 * Transformations:
		 *     do { S } while (0) ↦ calc(S)
		 *     do { S } while (C) ↦ NULL
		 * Note the second condition is overly-conservative. No
		 * solutions for the halting problem here. */
		DoStmt& do_stmt = cast<DoStmt> (stmt);
		Stmt* body = do_stmt.getBody ();
		Stmt* cond = do_stmt.getCond ();
		Expr* expr = dyn_cast<Expr> (cond);
		llvm::APSInt bool_expr;

		if (body != NULL &&
		    expr != NULL &&
		    expr->isIntegerConstantExpr (bool_expr, context) &&
		    !bool_expr.getBoolValue ()) {
			return is_assertion_stmt (*body, context);
		}

		return NULL;
	}
	case Stmt::StmtClass::IfStmtClass: {
		/* Handle an if(…) { … } else { … } block.
		 * Transformations:
		 *     if (C) { S1 } else { S2 } ↦
		 *       (C ∧ calc(S1)) ∨ (¬C ∧ calc(S2))
		 *     if (C) { S } ↦ (C ∧ calc(S)) ∨ ¬C
		 * i.e.
		 *     if (C) { S } ≡ if (C) { S } else {}
		 * where {} is an empty compound statement, below. */
		IfStmt& if_stmt = cast<IfStmt> (stmt);
		assert (if_stmt.getThen () != NULL);

		Expr* neg_cond = _negation_expr (if_stmt.getCond (), context);

		Expr* then_assertion =
			is_assertion_stmt (*(if_stmt.getThen ()), context);
		if (then_assertion == NULL)
			return NULL;

		then_assertion =
			_conjunction_expr (if_stmt.getCond (), then_assertion,
			                   context);

		if (if_stmt.getElse () == NULL)
			return _disjunction_expr (then_assertion, neg_cond,
			                          context);

		Expr* else_assertion =
			is_assertion_stmt (*(if_stmt.getElse ()), context);
		if (else_assertion == NULL)
			return NULL;

		else_assertion =
			_conjunction_expr (neg_cond, else_assertion, context);

		return _disjunction_expr (then_assertion, else_assertion,
		                          context);
	}
	case Stmt::StmtClass::ConditionalOperatorClass: {
		/* Handle a ternary operator.
		 * Transformations:
		 *     C ? S1 : S2 ↦
		 *       (C ∧ calc(S1)) ∨ (¬C ∧ calc(S2)) */
		ConditionalOperator& op_expr = cast<ConditionalOperator> (stmt);
		assert (op_expr.getTrueExpr () != NULL);
		assert (op_expr.getFalseExpr () != NULL);

		Expr* neg_cond = _negation_expr (op_expr.getCond (), context);

		Expr* then_assertion =
			is_assertion_stmt (*(op_expr.getTrueExpr ()), context);
		if (then_assertion == NULL)
			return NULL;

		then_assertion =
			_conjunction_expr (op_expr.getCond (), then_assertion,
			                   context);

		Expr* else_assertion =
			is_assertion_stmt (*(op_expr.getFalseExpr ()),
			                   context);
		if (else_assertion == NULL)
			return NULL;

		else_assertion =
			_conjunction_expr (neg_cond, else_assertion, context);

		return _disjunction_expr (then_assertion, else_assertion,
		                          context);
	}
	case Stmt::StmtClass::SwitchStmtClass: {
		/* Handle a switch statement.
		 * Transformations:
		 *     switch (C) { L1: S1; L2: S2; …; Lz: Sz } ↦ NULL
		 * FIXME: This should get a proper transformation sometime. */
		return NULL;
	}
	case Stmt::StmtClass::AttributedStmtClass: {
		/* Handle an attributed statement, e.g. G_LIKELY(…).
		 * Transformations:
		 *     att S ↦ calc(S) */
		AttributedStmt& attr_stmt = cast<AttributedStmt> (stmt);

		Stmt* sub_stmt = attr_stmt.getSubStmt ();
		if (sub_stmt == NULL)
			return NULL;

		return is_assertion_stmt (*sub_stmt, context);
	}
	case Stmt::StmtClass::CompoundStmtClass: {
		/* Handle a compound statement, e.g. { stmt1; stmt2; }.
		 * Transformations:
		 *     S1; S2; …; Sz ↦ calc(S1) ∧ calc(S2) ∧ … ∧ calc(Sz)
		 *     {} ↦ TRUE
		 *
		 * This is implemented by starting with a base TRUE case in the
		 * compound_condition, then taking the conjunction with the next
		 * statement’s assertion condition for each statement in the
		 * compound.
		 *
		 * If the compound is empty, the compound_condition will be
		 * TRUE. Otherwise, it will be (TRUE ∧ …), which will be
		 * simplified later. */
		CompoundStmt& compound_stmt = cast<CompoundStmt> (stmt);
		Expr* compound_condition =
			new (context)
				IntegerLiteral (context,
				                context.MakeIntValue (1, context.getLogicalOperationType ()),
				                context.getLogicalOperationType (),
				                SourceLocation ());

		for (CompoundStmt::const_body_iterator it =
		     compound_stmt.body_begin (),
		     ie = compound_stmt.body_end (); it != ie; ++it) {
			Stmt* body_stmt = *it;
			Expr* body_assertion =
				is_assertion_stmt (*body_stmt, context);

			if (body_assertion == NULL) {
				/* Reached a program state mutation. */
				return NULL;
			}

			/* Update the compound condition. */
			compound_condition =
				_conjunction_expr (compound_condition,
				                   body_assertion, context);

			DEBUG_EXPR ("Compound condition: ", *compound_condition);
		}

		return compound_condition;
	}
	case Stmt::StmtClass::GotoStmtClass:
		/* Handle a goto statement.
		 * Transformations:
		 *     goto L ↦ FALSE */
	case Stmt::StmtClass::ReturnStmtClass: {
		/* Handle a return statement.
		 * Transformations:
		 *     return ↦ FALSE */
		return new (context)
			IntegerLiteral (context,
			                context.MakeIntValue (0, context.getLogicalOperationType ()),
			                context.getLogicalOperationType (),
			                SourceLocation ());
	}
	case Stmt::StmtClass::NullStmtClass:
		/* Handle a null statement.
		 * Transformations:
		 *     ; ↦ TRUE */
	case Stmt::StmtClass::DeclRefExprClass:
		/* Handle a variable reference expression. These don’t modify
		 * program state.
		 * Transformations:
		 *     E ↦ TRUE */
	case Stmt::StmtClass::DeclStmtClass: {
		/* Handle a variable declaration statement. These don’t modify
		 * program state; they only introduce new state, so can’t affect
		 * subsequent assertions. (FIXME: For the moment, we ignore the
		 * possibility of the rvalue modifying program state.)
		 * Transformations:
		 *     T S1 ↦ TRUE
		 *     T S1 = S2 ↦ TRUE */
		return new (context)
			IntegerLiteral (context,
			                context.MakeIntValue (1, context.getLogicalOperationType ()),
			                context.getLogicalOperationType (),
			                SourceLocation ());
	}
	case Stmt::StmtClass::IntegerLiteralClass: {
		/* Handle an integer literal. This doesn’t modify program state,
		 * and evaluates directly to a boolean.
		 * Transformations:
		 *     0 ↦ FALSE
		 *     I ↦ TRUE */
		return dyn_cast<Expr> (&stmt);
	}
	case Stmt::StmtClass::ParenExprClass: {
		/* Handle a parenthesised expression.
		 * Transformations:
		 *     ( S ) ↦ calc(S) */
		ParenExpr& paren_expr = cast<ParenExpr> (stmt);

		Stmt* sub_expr = paren_expr.getSubExpr ();
		if (sub_expr == NULL)
			return NULL;

		return is_assertion_stmt (*sub_expr, context);
	}
	case Stmt::StmtClass::LabelStmtClass: {
		/* Handle a label statement.
		 * Transformations:
		 *     label: S ↦ calc(S) */
		LabelStmt& label_stmt = cast<LabelStmt> (stmt);

		Stmt* sub_stmt = label_stmt.getSubStmt ();
		if (sub_stmt == NULL)
			return NULL;

		return is_assertion_stmt (*sub_stmt, context);
	}
	case Stmt::StmtClass::ImplicitCastExprClass:
	case Stmt::StmtClass::CStyleCastExprClass: {
		/* Handle an explicit or implicit cast.
		 * Transformations:
		 *     (T) S ↦ calc(S) */
		CastExpr& cast_expr = cast<CastExpr> (stmt);

		Stmt* sub_expr = cast_expr.getSubExpr ();
		if (sub_expr == NULL)
			return NULL;

		return is_assertion_stmt (*sub_expr, context);
	}
	case Stmt::StmtClass::GCCAsmStmtClass:
	case Stmt::StmtClass::MSAsmStmtClass:
		/* Inline assembly. There is no way we are parsing this, so
		 * conservatively assume it modifies program state.
		 * Transformations:
		 *     A ↦ NULL */
	case Stmt::StmtClass::BinaryOperatorClass:
		/* Handle a binary operator statement. Since this is being
		 * processed at the top level, it’s most likely an assignment,
		 * so conservatively assume it modifies program state.
		 * Transformations:
		 *     S1 op S2 ↦ NULL */
	case Stmt::StmtClass::UnaryOperatorClass:
		/* Handle a unary operator statement. Since this is being
		 * processed at the top level, it’s not very interesting re.
		 * assertions, even though it probably won’t modify program
		 * state (unless it’s a pre- or post-increment or -decrement
		 * operator). Be conservative and assume it does, though.
		 * Transformations:
		 *     op S ↦ NULL */
	case Stmt::StmtClass::CompoundAssignOperatorClass:
		/* Handle a compound assignment operator, e.g. x += 5. This
		 * definitely modifies program state, so ignore it.
		 * Transformations:
		 *     S1 op S2 ↦ NULL */
	case Stmt::StmtClass::ForStmtClass:
		/* Handle a for statement. We assume these *always* change
		 * program state.
		 * Transformations:
		 *     for (…) { … } ↦ NULL */
	case Stmt::StmtClass::WhileStmtClass: {
		/* Handle a while(…) { … } block. Because we don't want to solve
		 * the halting problem, just assume all while statements cannot
		 * be assertion statements.
		 * Transformations:
		 *     while (C) { S } ↦ NULL
		 */
		return NULL;
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN_EXPR (__func__ << "() can’t handle statements of type " <<
		           stmt.getStmtClassName (), stmt);
		return NULL;
	}
}

/* Simplify a logical expression. Currently this eliminates extra parens and
 * casts, and performs basic boolean simplification according to common
 * identities.
 *
 * FIXME: Ideally, this should should be a full boolean expression minimiser,
 * returning in disjunctive normal form. */
static Expr*
_simplify_boolean_expr (Expr* expr, const ASTContext& context)
{
	expr = expr->IgnoreParens ();

	DEBUG ("Simplifying boolean expression of type " <<
	       expr->getStmtClassName ());

	if (expr->getStmtClass () == Expr::UnaryOperatorClass) {
		UnaryOperator& op_expr = cast<UnaryOperator> (*expr);
		Expr* sub_expr =
			_simplify_boolean_expr (op_expr.getSubExpr (), context);

		if (op_expr.getOpcode () != UnaryOperatorKind::UO_LNot) {
			/* op S ↦ op simplify(S) */
			op_expr.setSubExpr (sub_expr);
			return expr;
		}

		if (sub_expr->getStmtClass () == Expr::UnaryOperatorClass) {
			UnaryOperator& op_sub_expr =
				cast<UnaryOperator> (*sub_expr);
			Expr* sub_sub_expr =
				_simplify_boolean_expr (
					op_sub_expr.getSubExpr (), context);

			if (op_sub_expr.getOpcode () ==
			    UnaryOperatorKind::UO_LNot) {
				/* ! ! S ↦ simplify(S) */
				return sub_sub_expr;
			}

			/* ! op S ↦ ! op simplify(S) */
			op_sub_expr.setSubExpr (sub_sub_expr);
			return expr;
		} else if (sub_expr->getStmtClass () ==
		           Expr::BinaryOperatorClass) {
			BinaryOperator& op_sub_expr =
				cast<BinaryOperator> (*sub_expr);
			Expr* lhs =
				_simplify_boolean_expr (
					op_sub_expr.getLHS (), context);
			Expr* rhs =
				_simplify_boolean_expr (
					op_sub_expr.getRHS (), context);

			if (op_sub_expr.getOpcode () ==
			    BinaryOperatorKind::BO_EQ ||
			    op_sub_expr.getOpcode () ==
			    BinaryOperatorKind::BO_NE) {
				/* ! (S1 == S2) ↦
				 *     simplify(S1) != simplify(S2)
				 * or
				 * ! (S1 != S2) ↦
				 *     simplify(S1) == simplify(S2) */
				BinaryOperatorKind opcode =
					(op_sub_expr.getOpcode () ==
					 BinaryOperatorKind::BO_EQ) ?
						BinaryOperatorKind::BO_NE :
						BinaryOperatorKind::BO_EQ;

				return new (context)
					BinaryOperator (lhs, rhs, opcode,
					                context.getLogicalOperationType (),
					                VK_RValue, OK_Ordinary, SourceLocation (),
					                false);
			}

			/* ! (S1 op S2) ↦ ! (simplify(S1) op simplify(S2)) */
			op_sub_expr.setLHS (lhs);
			op_sub_expr.setRHS (rhs);
			return expr;
		}
	} else if (expr->getStmtClass () == Expr::BinaryOperatorClass) {
		BinaryOperator& op_expr = cast<BinaryOperator> (*expr);
		Expr* lhs = _simplify_boolean_expr (op_expr.getLHS (), context);
		Expr* rhs = _simplify_boolean_expr (op_expr.getRHS (), context);

		/* Guaranteed one-hot. */
		bool is_and =
			op_expr.getOpcode () == BinaryOperatorKind::BO_LAnd;
		bool is_or =
			op_expr.getOpcode () == BinaryOperatorKind::BO_LOr;

		if (!is_and && !is_or) {
			/* S1 op S2 ↦ simplify(S1) op simplify(S2) */
			op_expr.setLHS (lhs);
			op_expr.setRHS (rhs);
			return expr;
		}

		llvm::APSInt bool_expr;

		if (lhs->isIntegerConstantExpr (bool_expr, context)) {
			if (is_or && bool_expr.getBoolValue ()) {
				/* 1 || S2 ↦ 1 */
				return new (context)
					IntegerLiteral (context,
					                context.MakeIntValue (1, context.getLogicalOperationType ()),
					                context.getLogicalOperationType (),
					                SourceLocation ());
			} else if (is_and && !bool_expr.getBoolValue ()) {
				/* 0 && S2 ↦ 0 */
				return new (context)
					IntegerLiteral (context,
					                context.MakeIntValue (0, context.getLogicalOperationType ()),
					                context.getLogicalOperationType (),
					                SourceLocation ());
			} else {
				/* 1 && S2 ↦ simplify(S2)
				 * or
				 * 0 || S2 ↦ simplify(S2) */
				return rhs;
			}
		} else if (rhs->isIntegerConstantExpr (bool_expr, context)) {
			if (is_or && bool_expr.getBoolValue ()) {
				/* S1 || 1 ↦ 1 */
				return new (context)
					IntegerLiteral (context,
					                context.MakeIntValue (1, context.getLogicalOperationType ()),
					                context.getLogicalOperationType (),
					                SourceLocation ());
			} else if (is_and && !bool_expr.getBoolValue ()) {
				/* S2 && 0 ↦ 0 */
				return new (context)
					IntegerLiteral (context,
					                context.MakeIntValue (0, context.getLogicalOperationType ()),
					                context.getLogicalOperationType (),
					                SourceLocation ());
			} else {
				/* S1 && 1 ↦ simplify(S1)
				 * or
				 * S1 || 0 ↦ simplify(S1) */
				return lhs;
			}
		}

		/* S1 op S2 ↦ simplify(S1) op simplify(S2) */
		op_expr.setLHS (lhs);
		op_expr.setRHS (rhs);
		return expr;
	}

	return expr;
}

/* Calculate whether an assertion is a standard GObject type check.
 * .e.g. NSPACE_IS_OBJ(x).
 *
 * This is complicated by the fact that type checking is done by macros, which
 * expand to something like:
 * (((__extension__ ({
 *    GTypeInstance *__inst = (GTypeInstance *)((x));
 *    GType __t = ((nspace_obj_get_type()));
 *    gboolean __r;
 *    if (!__inst)
 *        __r = (0);
 *    else if (__inst->g_class && __inst->g_class->g_type == __t)
 *        __r = (!(0));
 *    else
 *        __r = g_type_check_instance_is_a(__inst, __t);
 *    __r;
 * }))))
 *
 * Insert the ValueDecls of the variables being checked into the provided
 * unordered_set, and return the number of such insertions (this will be 0 if no
 * variables are type checked). The returned number may be an over-estimate
 * of the number of elements in the set, as it doesn’t account for
 * duplicates. */
unsigned int
_assertion_is_gobject_type_check (Expr& assertion_expr,
                                  const ASTContext& context,
                                  std::unordered_set<const ValueDecl*>& ret)
{
	DEBUG_EXPR (__func__ << ": ", assertion_expr);

	switch (assertion_expr.getStmtClass ()) {
	case Expr::StmtExprClass: {
		/* Parse all the way through the statement expression, checking
		 * if the first statement is an assignment to the __inst
		 * variable, as in the macro expansion given above.
		 *
		 * This is a particularly shoddy way of checking for a GObject
		 * type check (we should really check for a
		 * g_type_check_instance_is_a() call) but this will do for
		 * now. */
		StmtExpr& stmt_expr = cast<StmtExpr> (assertion_expr);
		CompoundStmt* compound_stmt = stmt_expr.getSubStmt ();
		const Stmt* first_stmt = *(compound_stmt->body_begin ());

		if (first_stmt->getStmtClass () != Expr::DeclStmtClass)
			return 0;

		const DeclStmt& decl_stmt = cast<DeclStmt> (*first_stmt);
		const VarDecl* decl =
			dyn_cast<VarDecl> (decl_stmt.getSingleDecl ());

		if (decl == NULL)
			return 0;

		if (decl->getNameAsString () != "__inst")
			return 0;

		const Expr* init =
			decl->getAnyInitializer ()->IgnoreParenCasts ();
		const DeclRefExpr* decl_expr = dyn_cast<DeclRefExpr> (init);
		if (decl_expr != NULL) {
			ret.insert (decl_expr->getDecl ());
			return 1;
		}

		return 0;
	}
	case Expr::IntegerLiteralClass:
	case Expr::BinaryOperatorClass:
	case Expr::UnaryOperatorClass:
	case Expr::ConditionalOperatorClass:
	case Expr::CallExprClass:
	case Expr::ImplicitCastExprClass: {
		/* These can’t be type checks. */
		return 0;
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN_EXPR (__func__ << "() can’t handle expressions of type " <<
		           assertion_expr.getStmtClassName (), assertion_expr);
		return 0;
	}
}

/* Calculate whether an assertion is a standard non-NULL check.
 * e.g. (x != NULL), (x), (x != NULL && …) or (x && …).
 *
 * Insert the ValueDecls of the variables being checked into the provided
 * unordered_set, and return the number of such insertions (this will be 0 if no
 * variables are non-NULL checked). The returned number may be an over-estimate
 * of the number of elements in the set, as it doesn’t account for
 * duplicates. */
unsigned int
_assertion_is_explicit_nonnull_check (Expr& assertion_expr,
                                      const ASTContext& context,
                                      std::unordered_set<const ValueDecl*>& ret)
{
	DEBUG_EXPR (__func__ << ": ", assertion_expr);

	switch (assertion_expr.getStmtClass ()) {
	case Expr::BinaryOperatorClass: {
		BinaryOperator& bin_expr =
			cast<BinaryOperator> (assertion_expr);
		BinaryOperatorKind opcode = bin_expr.getOpcode ();

		if (opcode == BinaryOperatorKind::BO_LAnd) {
			/* LHS && RHS */
			unsigned int lhs_count =
				AssertionExtracter::assertion_is_nonnull_check (*(bin_expr.getLHS ()), context, ret);
			unsigned int rhs_count =
				AssertionExtracter::assertion_is_nonnull_check (*(bin_expr.getRHS ()), context, ret);

			return lhs_count + rhs_count;
		} else if (opcode == BinaryOperatorKind::BO_LOr) {
			/* LHS || RHS */
			std::unordered_set<const ValueDecl*> lhs_vars, rhs_vars;

			unsigned int lhs_count =
				AssertionExtracter::assertion_is_nonnull_check (*(bin_expr.getLHS ()), context, lhs_vars);
			unsigned int rhs_count =
				AssertionExtracter::assertion_is_nonnull_check (*(bin_expr.getRHS ()), context, rhs_vars);

			std::set_intersection (lhs_vars.begin (),
			                       lhs_vars.end (),
			                       rhs_vars.begin (),
			                       rhs_vars.end (),
			                       std::inserter (ret, ret.end ()));

			return lhs_count + rhs_count;
		} else if (opcode == BinaryOperatorKind::BO_NE) {
			/* LHS != RHS */
			Expr* rhs = bin_expr.getRHS ();
			Expr::NullPointerConstantKind k =
				rhs->isNullPointerConstant (const_cast<ASTContext&> (context),
				                            Expr::NullPointerConstantValueDependence::NPC_ValueDependentIsNotNull);
			if (k != Expr::NullPointerConstantKind::NPCK_NotNull &&
			    bin_expr.getLHS ()->IgnoreParenCasts ()->getStmtClass () == Expr::DeclRefExprClass) {
				DEBUG ("Found non-NULL check.");
				ret.insert (cast<DeclRefExpr> (bin_expr.getLHS ()->IgnoreParenCasts ())->getDecl ());
				return 1;
			}

			/* Either not a comparison to NULL, or the expr being
			 * compared is not a DeclRefExpr. */
			return 0;
		}

		return 0;
	}
	case Expr::UnaryOperatorClass: {
		/* A unary operator. For the moment, assume this isn't a
		 * non-null check.
		 *
		 * FIXME: In the future, define a proper program transformation
		 * to check for non-null checks, since we could have expressions
		 * like:
		 *     !(my_var == NULL)
		 * or (more weirdly):
		 *     ~(my_var == NULL)
		 */
		return 0;
	}
	case Expr::ConditionalOperatorClass: {
		/* A conditional operator. For the moment, assume this isn’t a
		 * non-null check.
		 *
		 * FIXME: In the future, define a proper program transformation
		 * to check for non-null checks, since we could have expressions
		 * like:
		 *     (x == NULL) ? TRUE : FALSE
		 */
		return 0;
	}
	case Expr::CStyleCastExprClass:
	case Expr::ImplicitCastExprClass: {
		/* A (explicit or implicit) cast. This can either be:
		 *     (void*)0
		 * or
		 *     (bool)my_var */
		CastExpr& cast_expr = cast<CastExpr> (assertion_expr);
		Expr* sub_expr = cast_expr.getSubExpr ()->IgnoreParenCasts ();

		if (sub_expr->getStmtClass () == Expr::DeclRefExprClass) {
			DEBUG ("Found non-NULL check.");
			ret.insert (cast<DeclRefExpr> (sub_expr)->getDecl ());
			return 1;
		}

		/* Not a cast to NULL, or the expr being casted is not a
		 * DeclRefExpr. */
		return 0;
	}
	case Expr::DeclRefExprClass: {
		/* A variable reference, which will implicitly become a non-NULL
		 * check. */
		DEBUG ("Found non-NULL check.");
		DeclRefExpr& decl_ref_expr = cast<DeclRefExpr> (assertion_expr);
		ret.insert (decl_ref_expr.getDecl ());
		return 1;
	}
	case Expr::StmtExprClass:
		/* FIXME: Statement expressions can be nonnull checks, but
		 * detecting them requires a formal program transformation which
		 * has not been implemented yet. */
	case Expr::CallExprClass:
		/* Function calls can’t be nonnull checks. */
	case Expr::IntegerLiteralClass: {
		/* Integer literals can’t be nonnull checks. */
		return 0;
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN_EXPR (__func__ << "() can’t handle expressions of type " <<
		           assertion_expr.getStmtClassName (), assertion_expr);
		return 0;
	}
}

unsigned int
AssertionExtracter::assertion_is_nonnull_check (Expr& assertion_expr,
                                                const ASTContext& context,
                                                std::unordered_set<const ValueDecl*>& param_decls)
{
	/* After this call, assume expr is in boolean disjunctive normal
	 * form. */
	Expr* expr = _simplify_boolean_expr (&assertion_expr, context);

	unsigned int explicit_nonnull_count =
		_assertion_is_explicit_nonnull_check (*expr, context, param_decls);
	unsigned int type_check_count =
		_assertion_is_gobject_type_check (*expr, context, param_decls);

	return explicit_nonnull_count + type_check_count;
}
