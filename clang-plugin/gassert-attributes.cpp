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

#include <unordered_set>

#include <clang/AST/Attr.h>
#include <clang/Lex/Lexer.h>

#include "gassert-attributes.h"

#ifdef ENABLE_DEBUG
#define DEBUG(M) llvm::errs () << M << "\n"
#define DEBUG_EXPR(M, E) llvm::errs () << M; \
	(E).printPretty (llvm::errs (), NULL, context.getPrintingPolicy ()); \
	llvm::errs () << "\n"
#else
#define DEBUG(M)
#define DEBUG_EXPR(M, E)
#endif

#define WARN(M) llvm::errs () << "Warning: " << M << "\n"

GAssertAttributesConsumer::GAssertAttributesConsumer ()
{
	/* Nothing to see here. */
}

GAssertAttributesConsumer::~GAssertAttributesConsumer ()
{
	/* Nothing to see here. */
}

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
static Expr*
_is_assertion_stmt (Stmt& stmt, const ASTContext& context)
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
			return _is_assertion_stmt (*body, context);
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
			_is_assertion_stmt (*(if_stmt.getThen ()), context);
		if (then_assertion == NULL)
			return NULL;

		then_assertion =
			_conjunction_expr (if_stmt.getCond (), then_assertion,
			                   context);

		if (if_stmt.getElse () == NULL)
			return _disjunction_expr (then_assertion, neg_cond,
			                          context);

		Expr* else_assertion =
			_is_assertion_stmt (*(if_stmt.getElse ()), context);
		if (else_assertion == NULL)
			return NULL;

		else_assertion =
			_conjunction_expr (neg_cond, else_assertion, context);

		return _disjunction_expr (then_assertion, else_assertion,
		                          context);
	}
	case Stmt::StmtClass::AttributedStmtClass: {
		/* Handle an attributed statement, e.g. G_LIKELY(…).
		 * Transformations:
		 *     att S ↦ calc(S) */
		AttributedStmt& attr_stmt = cast<AttributedStmt> (stmt);

		Stmt* sub_stmt = attr_stmt.getSubStmt ();
		if (sub_stmt == NULL)
			return NULL;

		return _is_assertion_stmt (*sub_stmt, context);
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
				_is_assertion_stmt (*body_stmt, context);

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
	case Stmt::StmtClass::BinaryOperatorClass:
		/* Handle a binary operator statement. Since this is being
		 * processed at the top level, it’s most likely an assignment,
		 * so conservatively assume it modifies program state.
		 * Transformations:
		 *     S1 op S2 ↦ NULL */
	case Stmt::StmtClass::ForStmtClass: {
		/* Handle a for statement. We assume these *always* change
		 * program state.
		 * Transformations:
		 *     for (…) { … } ↦ NULL */
		return NULL;
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN (__func__ << "() can’t handle statements of type " <<
		      stmt.getStmtClassName ());
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

		DEBUG_EXPR ("LHS: ", *lhs);
		DEBUG_EXPR ("RHS: ", *rhs);

		llvm::APSInt bool_expr;

		if (lhs->isIntegerConstantExpr (bool_expr, context)) {
			DEBUG ("FASD " << bool_expr.toString (10) << " " << is_or << " " << is_and);
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
			DEBUG ("DASD " << bool_expr.toString (10) << " " << is_or << " " << is_and);
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

		DEBUG ("BASD " << is_or << " " << is_and);

		/* S1 op S2 ↦ simplify(S1) op simplify(S2) */
		op_expr.setLHS (lhs);
		op_expr.setRHS (rhs);
		return expr;
	}

	return expr;
}

static std::unordered_set<const ValueDecl*>
_assertion_is_nonnull_check (Expr& assertion_expr,
                             const ASTContext& context);

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
 * Return a set of the ValueDecls of the variables being checked. This will be
 * empty if no variables are type checked. */
static std::unordered_set<const ValueDecl*>
_assertion_is_gobject_type_check (Expr& assertion_expr,
                                  const ASTContext& context)
{
	std::unordered_set<const ValueDecl*> ret;

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
			return ret;

		const DeclStmt& decl_stmt = cast<DeclStmt> (*first_stmt);
		const VarDecl* decl =
			dyn_cast<VarDecl> (decl_stmt.getSingleDecl ());

		if (decl == NULL)
			return ret;

		if (decl->getNameAsString () != "__inst")
			return ret;

		const Expr* init =
			decl->getAnyInitializer ()->IgnoreParenCasts ();
		const DeclRefExpr* decl_expr = dyn_cast<DeclRefExpr> (init);
		if (decl_expr != NULL)
			ret.insert (decl_expr->getDecl ());

		return ret;
	}
	case Expr::IntegerLiteralClass: {
		/* Integer literals can’t be type checks. */
		return ret;
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN (__func__ << "() can’t handle expressions of type " <<
		      assertion_expr.getStmtClassName ());
		return ret;
	}
}

/* Calculate whether an assertion is a standard non-NULL check.
 * e.g. (x != NULL), (x), (x != NULL && …) or (x && …).
 *
 * Return a set of the ValueDecls of the variables being checked. This will be
 * empty if no variables are non-NULL checked. */
static std::unordered_set<const ValueDecl*>
_assertion_is_explicit_nonnull_check (Expr& assertion_expr,
                                      const ASTContext& context)
{
	std::unordered_set<const ValueDecl*> ret;

	DEBUG_EXPR (__func__ << ": ", assertion_expr);

	switch (assertion_expr.getStmtClass ()) {
	case Expr::BinaryOperatorClass: {
		BinaryOperator& bin_expr =
			cast<BinaryOperator> (assertion_expr);
		BinaryOperatorKind opcode = bin_expr.getOpcode ();

		if (opcode == BinaryOperatorKind::BO_LAnd) {
			/* LHS && RHS */
			std::unordered_set<const ValueDecl*> lhs_vars =
				_assertion_is_nonnull_check (*(bin_expr.getLHS ()), context);
			std::unordered_set<const ValueDecl*> rhs_vars =
				_assertion_is_nonnull_check (*(bin_expr.getRHS ()), context);

			std::set_union (lhs_vars.begin (),
			                lhs_vars.end (),
			                rhs_vars.begin (),
			                rhs_vars.end (),
			                std::inserter (ret, ret.end ()));

			return ret;
		} else if (opcode == BinaryOperatorKind::BO_LOr) {
			/* LHS || RHS */
			std::unordered_set<const ValueDecl*> lhs_vars =
				_assertion_is_nonnull_check (*(bin_expr.getLHS ()), context);
			std::unordered_set<const ValueDecl*> rhs_vars =
				_assertion_is_nonnull_check (*(bin_expr.getRHS ()), context);

			std::set_intersection (lhs_vars.begin (),
			                       lhs_vars.end (),
			                       rhs_vars.begin (),
			                       rhs_vars.end (),
			                       std::inserter (ret, ret.end ()));

			return ret;
		} else if (opcode == BinaryOperatorKind::BO_NE) {
			/* LHS != RHS */
			Expr* rhs = bin_expr.getRHS ();
			Expr::NullPointerConstantKind k =
				rhs->isNullPointerConstant (const_cast<ASTContext&> (context),
				                            Expr::NullPointerConstantValueDependence::NPC_ValueDependentIsNotNull);
			if (k != Expr::NullPointerConstantKind::NPCK_NotNull &&
			    bin_expr.getLHS ()->IgnoreParenCasts ()->getStmtClass () == Expr::DeclRefExprClass) {
				ret.insert (cast<DeclRefExpr> (bin_expr.getLHS ()->IgnoreParenCasts ())->getDecl ());
				return ret;
			}

			/* Either not a comparison to NULL, or the expr being
			 * compared is not a DeclRefExpr. */
			return ret;
		}

		return ret;
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
			ret.insert (cast<DeclRefExpr> (sub_expr)->getDecl ());
			return ret;
		}

		/* Not a cast to NULL, or the expr being casted is not a
		 * DeclRefExpr. */
		return ret;
	}
	case Expr::DeclRefExprClass: {
		/* A variable reference, which will implicitly become a non-NULL
		 * check. */
		DeclRefExpr& decl_ref_expr = cast<DeclRefExpr> (assertion_expr);
		ret.insert (decl_ref_expr.getDecl ());
		return ret;
	}
	case Expr::IntegerLiteralClass: {
		/* Integer literals can’t be nonnull checks. */
		return ret;
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN (__func__ << "() can’t handle expressions of type " <<
		      assertion_expr.getStmtClassName ());
		return ret;
	}
}

static std::unordered_set<const ValueDecl*>
_assertion_is_nonnull_check (Expr& assertion_expr,
                             const ASTContext& context)
{
	/* After this call, assume expr is in boolean disjunctive normal
	 * form. */
	Expr* expr = _simplify_boolean_expr (&assertion_expr, context);

	std::unordered_set<const ValueDecl*> ret;

	std::unordered_set<const ValueDecl*> explicit_vars =
		_assertion_is_explicit_nonnull_check (*expr, context);
	std::unordered_set<const ValueDecl*> type_check_vars =
		_assertion_is_gobject_type_check (*expr, context);

	std::set_union (explicit_vars.begin (),
	                explicit_vars.end (),
	                type_check_vars.begin (),
	                type_check_vars.end (),
	                std::inserter (ret, ret.end ()));

	return ret;
}

/* Given an expression which is asserted to be true by an assertion statement,
 * work out what type of assertion it is (e.g. GObject type check, non-NULL
 * check, etc.) and modify the FunctionDecl as appropriate. For example, for
 * non-NULL checks this involves adding a nonnull attribute on the function.
 *
 * assertion_expr may be modified in-place to simplify its boolean form. */
static void
_handle_assertion (FunctionDecl& func, Expr& assertion_expr,
                   const ASTContext& context)
{
	DEBUG ("Handling assertion.");

	/* If the assertion is a non-NULL check, add nonnull attributes to the
	 * function’s parameters accordingly. */
	std::unordered_set<const ValueDecl*> ret =
		_assertion_is_nonnull_check (assertion_expr, context);

	if (ret.size () == 0)
		return;

	/* TODO: Factor out the code to augment a nonnull attribute. */
	std::vector<unsigned int> non_null_args;

	NonNullAttr* nonnull_attr = func.getAttr<NonNullAttr> ();
	if (nonnull_attr != NULL) {
		/* Extend and replace the existing attribute. */
		DEBUG ("Extending existing attribute.");
		non_null_args.insert (non_null_args.begin (),
		                      nonnull_attr->args_begin (),
		                      nonnull_attr->args_end ());
	}

	for (std::unordered_set<const ValueDecl*>::iterator si = ret.begin (),
	     se = ret.end (); si != se; ++si) {
		const ValueDecl* val_decl = *si;

		const ParmVarDecl* parm_decl = dyn_cast<ParmVarDecl> (val_decl);
		if (parm_decl == NULL) {
			WARN ("non-ParmVarDecl " <<
			      val_decl->getNameAsString ());
			continue;
		}

		unsigned int j = parm_decl->getFunctionScopeIndex ();
		DEBUG ("Got nonnull arg " << j << " (" <<
		       val_decl->getNameAsString () << ") from assertion.");
		non_null_args.push_back (j);
	}

	if (non_null_args.size () > 0) {
		nonnull_attr = ::new (func.getASTContext ())
			NonNullAttr (func.getSourceRange (),
			             func.getASTContext (),
			             non_null_args.data (),
			             non_null_args.size ());
		func.addAttr (nonnull_attr);
	}
}

void
GAssertAttributesConsumer::_handle_function_decl (FunctionDecl& func)
{
	/* Can only handle functions which have a body defined. */
	Stmt* func_body = func.getBody ();
	if (func_body == NULL) {
		return;
	}

	/* The body should be a compound statement, e.g.
	 * { stmt; stmt; } */
	CompoundStmt* stmt = dyn_cast<CompoundStmt> (func_body);
	if (stmt == NULL) {
		DEBUG ("Ignoring function " << func.getNameAsString () <<
		       " due to having a non-compound statement body.");
		return;
	}

	DEBUG ("Examining " << func.getNameAsString());

	/* Iterate through the function body until the first non-assertion and
	 * non-declaration statement is reached. Specifically stop before the
	 * first assignment, as that could affect the outcome of any subsequent
	 * assertions. */
	for (CompoundStmt::const_body_iterator it = stmt->body_begin (),
	     ie = stmt->body_end (); it != ie; ++it) {
		Stmt* body_stmt = *it;

		Expr* assertion_expr =
			_is_assertion_stmt (*body_stmt, func.getASTContext ());

		if (assertion_expr == NULL) {
			/* Potential program state mutation reached, so run
			 * away. */
			break;
		}

		/* Modify the FunctionDecl to take advantage of the extracted
		 * assertion expression. */
		_handle_assertion (func, *assertion_expr,
		                   func.getASTContext ());
	}

	DEBUG ("");
}

bool
GAssertAttributesConsumer::HandleTopLevelDecl (DeclGroupRef decl_group)
{
	DeclGroupRef::iterator i, e;

	for (i = decl_group.begin (), e = decl_group.end (); i != e; i++) {
		Decl *decl = *i;
		FunctionDecl *func = dyn_cast<FunctionDecl> (decl);

		/* We’re only interested in function declarations. */
		if (func == NULL)
			continue;

		this->_handle_function_decl (*func);
	}

	return true;
}
