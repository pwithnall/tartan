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

#include <unordered_set>

#include <clang/AST/Attr.h>
#include <clang/Lex/Lexer.h>

#include "assertion-extracter.h"
#include "debug.h"
#include "gassert-attributes.h"

namespace tartan {

GAssertAttributesConsumer::GAssertAttributesConsumer ()
{
	/* Nothing to see here. */
}

GAssertAttributesConsumer::~GAssertAttributesConsumer ()
{
	/* Nothing to see here. */
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
	DEBUG_EXPR ("Handling assertion: ", assertion_expr);

	/* If the assertion is a non-NULL check, add nonnull attributes to the
	 * function’s parameters accordingly. */
	std::unordered_set<const ValueDecl*> ret;
	AssertionExtracter::assertion_is_nonnull_check (assertion_expr,
	                                                context, ret);

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
			/* People can use statically declared variables, etc.,
			 * in their assertions. Ignore those. */
			DEBUG ("non-ParmVarDecl " <<
			       val_decl->getNameAsString ());
			continue;
		}

		unsigned int j = parm_decl->getFunctionScopeIndex ();
		DEBUG ("Got nonnull arg " << j << " (" <<
		       val_decl->getNameAsString () << ") from assertion.");
		non_null_args.push_back (j);
	}

	if (non_null_args.size () > 0) {
#ifdef HAVE_LLVM_3_5
		nonnull_attr = ::new (func.getASTContext ())
			NonNullAttr (func.getSourceRange (),
			             func.getASTContext (),
			             non_null_args.data (),
			             non_null_args.size (), 0);
#else /* if !HAVE_LLVM_3_5 */
		nonnull_attr = ::new (func.getASTContext ())
			NonNullAttr (func.getSourceRange (),
			             func.getASTContext (),
			             non_null_args.data (),
			             non_null_args.size ());
#endif /* !HAVE_LLVM_3_5 */
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

	ASTContext& context = func.getASTContext ();

	/* Iterate through the function body until the first non-assertion and
	 * non-declaration statement is reached. Specifically stop before the
	 * first assignment, as that could affect the outcome of any subsequent
	 * assertions. */
	for (CompoundStmt::const_body_iterator it = stmt->body_begin (),
	     ie = stmt->body_end (); it != ie; ++it) {
		Stmt* body_stmt = *it;

		Expr* assertion_expr =
			AssertionExtracter::is_assertion_stmt (*body_stmt,
			                                       context);

		if (assertion_expr == NULL) {
			/* Potential program state mutation reached, so run
			 * away. */
			break;
		}

		/* Modify the FunctionDecl to take advantage of the extracted
		 * assertion expression. */
		_handle_assertion (func, *assertion_expr, context);
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

} /* namespace tartan */
