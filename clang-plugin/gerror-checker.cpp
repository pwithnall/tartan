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

/**
 * GErrorChecker:
 *
 * This is a checker for #GError usage, both with the g_error_*() API, and with
 * normal C pointer operations on #GErrors. It validates that all #GError
 * pointers are initialised to %NULL, that valid #GErrors are not overwritten,
 * and that #GErrors are not double-freed or leaked. It also validates more
 * mundane things, like whether error codes actually belong in the domain passed
 * to g_error_new() (for example).
 *
 * This is effectively a highly specific memory allocation checker, imposing the
 * rules about clearing #GError pointers to %NULL which #GError convention
 * dictates.
 *
 * The checker uses full path-dependent analysis, so will catch bugs arising
 * from #GErrors being handled differently on different control paths, which is
 * empirically where most #GError bugs arise.
 *
 * The checker is implemented using a combination of Clang’s internal symbolic
 * value model, and a custom ErrorMap using Clang’s program state maps. The
 * ErrorMap tracks state for each GError* pointer it knows about, using three
 * states:
 *  • Clear: error = NULL
 *  • Set: error ≠ NULL ∧ valid_allocation(error)
 *  • Freed: error ≠ NULL ∧ ¬valid_allocation(error)
 *
 * In the comments below, the following modelling functions are used:
 *     valid_allocation(error):
 *         True iff error has been allocated as a GError, but not yet freed.
 *         Corresponds to the ErrorState.Set state.
 *     error_codes(domain):
 *         Returns a set of error codes which are valid for the given domain,
 *         as defined by the enum associated with that error domain.
 *
 * FIXME: Future work could be to implement:
 *  • Support for user-defined functions which take GError** parameters.
 *  • Add support for g_error_copy()
 *  • Add support for g_error_matches()
 *  • Add support for g_prefix_error()
 *  • Implement check::DeadSymbols  (for cleaning up internal state)
 *  • Implement check::PointerEscape  (for leaks)
 *  • Implement check::ConstPointerEscape  (for leaks)
 *  • Implement check::PreStmt<ReturnStmt>  (for leaks)
 *  • Implement check::PostStmt<BlockExpr>  (for leaks)
 *  • Implement check::Location  (for bad dereferences)
 *  • Implement eval::Assume
 *  • Check that error codes match their domains.
 *  • Set the MemRegion contents more explicitly in _gerror_new() — it would be
 *    nice to get static analysis on code and domain values.
 *  • Domain analysis on propagated GErrors: track which error domains each
 *    function can possibly return, and warn if they’re not all handled by
 *    callers.
 */

#include <clang/StaticAnalyzer/Core/BugReporter/BugType.h>
#include <clang/StaticAnalyzer/Core/Checker.h>
#include <clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h>
#include <clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h>

#include "gerror-checker.h"
#include "type-manager.h"
#include "debug.h"

namespace tartan {

using namespace clang;

struct ErrorState {
	enum Kind { Clear, Set, Freed } K;
	SourceRange S;

	ErrorState (Kind k, const SourceRange &s) : K (k), S (s) {}

	bool isClear () const { return K == Clear; }
	bool isSet () const { return K == Set; }
	bool isFreed () const { return K == Freed; }

	bool operator== (const ErrorState &X) const {
		return K == X.K && S == X.S;
	}

	static ErrorState getClear (const SourceRange &s) { return ErrorState (Clear, s); }
	static ErrorState getSet (const SourceRange &s) { return ErrorState (Set, s); }
	static ErrorState getFreed (const SourceRange &s) { return ErrorState (Freed, s); }

	void Profile (llvm::FoldingSetNodeID &ID) const {
		ID.AddInteger (K);
		ID.AddPointer (&S);
	}
};

} /* namespace tartan */

/* Track GError*s and their states in a map stored on the ProgramState.
 * The namespacing is necessary to be able to specialise a Clang template. */
REGISTER_MAP_WITH_PROGRAMSTATE (ErrorMap, clang::ento::SymbolRef,
                                tartan::ErrorState)

namespace tartan {

static ProgramStateRef
_error_map_remove (ProgramStateRef state, SymbolRef symbol)
{
	DEBUG ("error_map_remove: " << symbol);
	return state->remove<ErrorMap> (symbol);
}

static ProgramStateRef
_error_map_set (ProgramStateRef state, SymbolRef symbol, ErrorState error_state)
{
	DEBUG ("error_map_set: " << symbol);
	return state->set<ErrorMap> (symbol, error_state);
}

static const ErrorState *
_error_map_get (ProgramStateRef state, SymbolRef symbol)
{
	DEBUG ("error_map_get: " << symbol);
	return state->get<ErrorMap> (symbol);
}

/* Try to get the SVal for the GError* pointed to by a GError** SVal. */
SVal
GErrorChecker::_error_from_error_ptr (SVal ptr_error_location,
                                      CheckerContext &context) const
{
	DEBUG_DUMPABLE ("Getting GError* location from call:",
	                ptr_error_location);

return context.getState()->getSVal(ptr_error_location.castAs<Loc>());

	const MemRegion *region = ptr_error_location.getAsRegion ();
	if (region == NULL) {
		DEBUG ("Not a MemRegion.");
		return UnknownVal ();
	}

	/* We only support typed regions at the moment, as those come from
	 * taking a pointer address. */
	const TypedValueRegion *typed_region =
		region->getAs<TypedValueRegion> ();
	if (typed_region == NULL) {
		return UnknownVal ();
	}

	/* Sanity check the type. */
	const ASTContext &ast_context = context.getASTContext ();
	QualType error_ptr_type =
		ast_context.getPointerType (this->_gerror_type);

	this->_initialise_identifiers (ast_context);
	assert (ast_context.hasSameType (typed_region->getValueType (),
	                                 error_ptr_type));

	const ProgramStateRef state = context.getState ();

	/* Get the SVal bound to the MemRegion pointed at by the argument, or
	 * UnknownVal if no SVal is currently bound. */
	SVal retval = state->getSVal (region);

	DEBUG_DUMPABLE ("Got SVal for MemRegion:", retval);

	return retval;
}

/**
 * Just before a g_set_error(error_ptr, domain, code, format, …) call, check
 * that:
 *     (error_ptr = NULL) ∨ (*error_ptr = NULL)
 *     code ϵ error_codes(domain)
 */
ProgramStateRef
GErrorChecker::_handle_pre_g_set_error (CheckerContext &context,
                                        const CallEvent &call_event) const
{
	if (!this->_assert_gerror_ptr_clear (call_event.getArgSVal (0),
	                                     context.getState (), context,
	                                     call_event.getArgSourceRange (0)) ||
	    !this->_assert_code_in_domain (call_event.getArgSVal (1),
	                                   call_event.getArgSVal (2),
	                                   context.getState (), context,
	                                   call_event.getArgSourceRange (1),
	                                   call_event.getArgSourceRange (2))) {
		return NULL;
	}

	return context.getState ();
}

/**
 * Just after a g_set_error(error_ptr, …) call, change the state to:
 *  • Conjure a new heap memory region for a new GError.
 *  • Bind that to (*error_ptr).
 *  • Update the ErrorMap to mark (*error_ptr) as Set.
 */
ProgramStateRef
GErrorChecker::_handle_eval_g_set_error (CheckerContext &context,
                                         const CallExpr &call_expr) const
{
	ProgramStateRef state = context.getState ();

	/* Statically construct a new GError instance and bind it to the
	 * dereferenced GError** pointer. */
	DefinedSVal *allocated_sval = NULL;
	state = this->_gerror_new (&call_expr, false,
	                           &allocated_sval, state, context,
	                           call_expr.getSourceRange ());

	SVal ptr_error_location = state->getSVal (call_expr.getArg (0),
	                                          context.getLocationContext ());

	state = this->_set_gerror (ptr_error_location, *allocated_sval,
	                           state, context,
	                           call_expr.getArg (0)->getSourceRange ());

	delete allocated_sval;

	return state;
}

/**
 * Just before a g_error_new(domain, code, format, …) call, check
 * that:
 *     code ϵ error_codes(domain)
 */
ProgramStateRef
GErrorChecker::_handle_pre_g_error_new (CheckerContext &context,
                                        const CallEvent &call_event) const
{
	if (!this->_assert_code_in_domain (call_event.getArgSVal (0),
	                                   call_event.getArgSVal (1),
	                                   context.getState (), context,
	                                   call_event.getArgSourceRange (0),
	                                   call_event.getArgSourceRange (1))) {
		return NULL;
	}

	return context.getState ();
}

/**
 * Just after a g_error_new(…) call, change the state to:
 *  • Conjure a new heap memory region for a new GError.
 *  • Bind it to the call’s return value.
 */
ProgramStateRef
GErrorChecker::_handle_eval_g_error_new (CheckerContext &context,
                                         const CallExpr &call_expr) const
{
	return this->_gerror_new (&call_expr, true, NULL,
	                          context.getState (), context,
	                          call_expr.getSourceRange ());
}

/**
 * Just before a g_error_free(error) call, check
 * that:
 *     error ≠ NULL ∧ valid_allocation(error)
 */
ProgramStateRef
GErrorChecker::_handle_pre_g_error_free (CheckerContext &context,
                                         const CallEvent &call_event) const
{
	SVal error_location = call_event.getArgSVal (0);

	if (!this->_assert_gerror_set (error_location, false,
	                               context.getState (), context,
	                               call_event.getArgSourceRange (0))) {
		return NULL;
	}

	return context.getState ();
}

/**
 * Just after a g_error_free(error) call, change the state to:
 *  • Update the ErrorMap to mark error as Free.
 *  • Update the MemRegion for (*error) to fill it with undefined values.
 */
ProgramStateRef
GErrorChecker::_handle_eval_g_error_free (CheckerContext &context,
                                          const CallExpr &call_expr) const
{
	ProgramStateRef state = context.getState ();

	SVal error_location = state->getSVal (call_expr.getArg (0),
	                                      context.getLocationContext ());

	DEBUG_DUMPABLE ("Handle post-g_error_free:", error_location);

	state = this->_gerror_free (error_location, context.getState (),
	                            context,
	                            call_expr.getArg (0)->getSourceRange ());

	return state;
}

/**
 * Just before a g_clear_error(error_ptr) call, check that:
 *     error_ptr = NULL ∨ (*error_ptr) = NULL ∨ valid_allocation(*error_ptr)
 */
ProgramStateRef
GErrorChecker::_handle_pre_g_clear_error (CheckerContext &context,
                                          const CallEvent &call_event) const
{
	ProgramStateRef state;
	ProgramStateRef not_null_state, null_state;  /* for GError* */
	ProgramStateRef ptr_not_null_state, ptr_null_state;  /* for GError** */

	state = context.getState ();

	SVal _ptr_error_location = call_event.getArgSVal (0);
	if (!_ptr_error_location.getAs<DefinedOrUnknownSVal> ()) {
		return state;
	}

	DefinedOrUnknownSVal ptr_error_location =
		_ptr_error_location.castAs<DefinedOrUnknownSVal> ();

	/* Branch on whether the GError** is NULL. If it is, we have nothing to
	 * do. */
	std::tie (ptr_not_null_state, ptr_null_state) =
		state->assume (ptr_error_location);
	if (ptr_null_state && !ptr_not_null_state) {
		/* Definitely NULL. */
		return state;
	}

	SVal error_location =
		this->_error_from_error_ptr (call_event.getArgSVal (0),
		                             context);

	/* Check whether the GError* is free. */
	if (!this->_assert_gerror_set (error_location, true, state, context,
	                               call_event.getArgSourceRange (0))) {
		return NULL;
	}

	return state;
}

/**
 * Just after a g_clear_error(error_ptr) call, change the state to:
 *  • Update the ErrorMap to mark (*error_ptr) as Clear.
 *  • Update the MemRegion for (**error_ptr) to fill it with undefined values.
 *  • Bind (*error_ptr) to NULL.
 */
ProgramStateRef
GErrorChecker::_handle_eval_g_clear_error (CheckerContext &context,
                                           const CallExpr &call_expr) const
{
	ProgramStateRef state = context.getState ();

	SVal ptr_error_location = state->getSVal (call_expr.getArg (0),
	                                          context.getLocationContext ());
	SVal error_location =
		this->_error_from_error_ptr (ptr_error_location, context);

	DEBUG_DUMPABLE ("Handle post-g_clear_error:", error_location);

	/* Free the GError*. */
	state = this->_gerror_free (error_location, state, context,
	                            call_expr.getArg (0)->getSourceRange ());

	if (state == NULL) {
		return state;
	}

	/* Set it to NULL. */
	state = this->_clear_gerror (ptr_error_location, state, context,
	                             call_expr.getArg (0)->getSourceRange ());

	return state;
}

/**
 * Just before a g_propagate_error(dest_error_ptr, src_error) call, check that:
 *     src_error ≠ NULL ∧ valid_allocation(src_error)
 *     dest_error_ptr = NULL ∨ (*dest_error_ptr) = NULL
 */
ProgramStateRef
GErrorChecker::_handle_pre_g_propagate_error (CheckerContext &context,
                                              const CallEvent &call_event) const
{
	SVal dest_ptr_location = call_event.getArgSVal (0);
	SVal src_location = call_event.getArgSVal (1);

	if (!this->_assert_gerror_ptr_clear (dest_ptr_location,
	                                     context.getState (), context,
	                                     call_event.getArgSourceRange (0)) ||
	    !this->_assert_gerror_set (src_location, false, context.getState (),
	                               context,
	                               call_event.getArgSourceRange (1))) {
		return NULL;
	}

	return context.getState ();
}

/**
 * Just after a g_propagate_error(dest_error_ptr, src_error) call, change the
 * state to:
 *  • If (dest_error_ptr = NULL), update the ErrorMap to mark src_error as Free
 *    and update the MemRegion for (*src_error) to fill it with undefined
 *    values.
 *  • If (dest_error_ptr ≠ NULL), bind (*dest_error_ptr) to src_error.
 */
ProgramStateRef
GErrorChecker::_handle_eval_g_propagate_error (CheckerContext &context,
                                               const CallExpr &call_expr) const
{
	ProgramStateRef state = context.getState ();

	SVal dest_ptr_location = state->getSVal (call_expr.getArg (0),
	                                         context.getLocationContext ());
	SVal dest_location =
		this->_error_from_error_ptr (dest_ptr_location, context);
	SVal src_location = state->getSVal (call_expr.getArg (1),
	                                    context.getLocationContext ());

	if (!src_location.getAs<DefinedSVal> ()) {
		DEBUG ("Cannot get src location as DefinedSVal.");
		return state;
	}

	DEBUG_DUMPABLE ("Handle post-g_propagate_error: dest_location:",
	                dest_location);
	DEBUG_DUMPABLE ("Handle post-g_propagate_error: src_location:",
	                src_location);

	/* Branch on whether the GError** is NULL. If it is, the src error
	 * should be freed. */
	if (!dest_ptr_location.getAs<DefinedOrUnknownSVal> ()) {
		return state;
	}

	ProgramStateRef not_null_state, null_state;
	std::tie (not_null_state, null_state) =
		state->assume (dest_ptr_location.castAs<DefinedOrUnknownSVal> ());

	if (null_state != NULL) {
		/* Potentially NULL, so free the @src error. */
		null_state = this->_gerror_free (src_location, null_state,
		                                 context,
		                                 call_expr.getArg (1)->getSourceRange ());
	}

	if (not_null_state != NULL) {
		/* Set the @dest error. Don’t know why we have to use @dest_ptr_location
		 * here, but it seems to work. Obviously my understanding of SVals is
		 * worse than I thought. */
		not_null_state = this->_set_gerror (dest_ptr_location,
		                                    src_location.castAs<DefinedSVal> (),
		                                    not_null_state,
		                                    context,
		                                    call_expr.getArg (0)->getSourceRange ());
	}

	if (not_null_state != NULL && null_state != NULL) {
		context.addTransition (null_state);
	}

	return (not_null_state != NULL) ? not_null_state : null_state;
}

/* Dispatch pre-call events to the different per-function handlers. */
void
GErrorChecker::checkPreCall (const CallEvent &call,
                             CheckerContext &context) const
{
	if (!call.isGlobalCFunction ()) {
		return;
	}

	ASTContext &ast_context = context.getASTContext ();
	this->_initialise_identifiers (ast_context);

	const IdentifierInfo *call_ident = call.getCalleeIdentifier ();

	ProgramStateRef new_state;

	if (call_ident == this->_identifier_g_set_error ||
	    call_ident == this->_identifier_g_set_error_literal) {
		new_state = this->_handle_pre_g_set_error (context, call);
	} else if (call_ident == this->_identifier_g_error_new ||
	           call_ident == this->_identifier_g_error_new_literal ||
	           call_ident == this->_identifier_g_error_new_valist) {
		new_state = this->_handle_pre_g_error_new (context, call);
	} else if (call_ident == this->_identifier_g_error_free) {
		new_state = this->_handle_pre_g_error_free (context, call);
	} else if (call_ident == this->_identifier_g_clear_error) {
		new_state = this->_handle_pre_g_clear_error (context, call);
	} else if (call_ident == this->_identifier_g_propagate_error ||
	           call_ident == this->_identifier_g_propagate_prefixed_error) {
		new_state = this->_handle_pre_g_propagate_error (context, call);
	} else {
		new_state = NULL;
	}

	if (new_state != NULL) {
		context.addTransition (new_state);
	}
}

/* Dispatch call-evaluation events to the different per-function handlers.
 * Return true iff the call was evaluated. */
bool
GErrorChecker::evalCall (const CallExpr *call,
                         CheckerContext &context) const
{
	const FunctionDecl *func_decl = context.getCalleeDecl (call);

	if (func_decl == NULL ||
	    func_decl->getKind() != Decl::Function ||
	    !CheckerContext::isCLibraryFunction (func_decl)) {
		return false;
	}

	ASTContext &ast_context = context.getASTContext ();
	this->_initialise_identifiers (ast_context);

	const IdentifierInfo *call_ident = func_decl->getIdentifier ();

	ProgramStateRef new_state;

	if (call_ident == this->_identifier_g_set_error ||
	    call_ident == this->_identifier_g_set_error_literal) {
		new_state = this->_handle_eval_g_set_error (context, *call);
	} else if (call_ident == this->_identifier_g_error_new ||
	           call_ident == this->_identifier_g_error_new_literal ||
	           call_ident == this->_identifier_g_error_new_valist) {
		new_state = this->_handle_eval_g_error_new (context, *call);
	} else if (call_ident == this->_identifier_g_error_free) {
		new_state = this->_handle_eval_g_error_free (context, *call);
	} else if (call_ident == this->_identifier_g_clear_error) {
		new_state = this->_handle_eval_g_clear_error (context, *call);
	} else if (call_ident == this->_identifier_g_propagate_error ||
	           call_ident == this->_identifier_g_propagate_prefixed_error) {
		new_state = this->_handle_eval_g_propagate_error (context,
		                                                  *call);
	} else {
		new_state = NULL;
	}

	if (new_state != NULL) {
		context.addTransition (new_state);
	}

	return (new_state != NULL);
}

/**
 * Just before a value binding of (loc = val), check that:
 *     val = NULL ∨ (val ≠ NULL ∧ valid_allocation(val))
 *     loc = NULL ∨ ¬valid_allocation(loc)
 */
void
GErrorChecker::checkBind (SVal loc, SVal val, const Stmt *stmt,
                          CheckerContext &context) const
{
	ProgramStateRef new_state;

	/* We’re only interested in stores into GError*s. */
	const TypedValueRegion *region =
		dyn_cast_or_null<TypedValueRegion> (loc.getAsRegion ());
	if (region == NULL) {
		return;
	}

	const ASTContext &ast_context = context.getASTContext ();
	this->_initialise_identifiers (ast_context);

	QualType error_type = ast_context.getPointerType (this->_gerror_type);
	QualType value_type = region->getValueType ();
	if (!context.getASTContext ().hasSameType (error_type, value_type)) {
		return;
	}

	/* Check the preconditions on loc and val. */
	ProgramStateRef state = context.getState ();
	SVal loc_region = state->getSVal (region);

	if (!this->_assert_gerror_unset (loc_region, true, context.getState (), context,
	                                 stmt->getSourceRange ()) ||
	    !this->_assert_gerror_set (val, true, context.getState (), context,
	                               stmt->getSourceRange ())) {
		return;
	}

	/* Update the binding. */
	ConditionTruthVal val_is_null = state->isNull (val);

	if (val_is_null.isConstrainedTrue ()) {
		DEBUG_DUMPABLE ("Check bind: clearing GError*:", loc);

		new_state = this->_clear_gerror (loc, state, context,
		                                 stmt->getSourceRange ());
	} else {
		DEBUG_DUMPABLE ("Check bind: setting GError*:", loc);

		new_state = this->_set_gerror (loc, val.castAs<DefinedSVal> (),
		                               state, context,
		                               stmt->getSourceRange ());
	}

	if (new_state != NULL) {
		context.addTransition (new_state);
	}
}

void
GErrorChecker::checkDeadSymbols (SymbolReaper &symbol_reaper,
                                 CheckerContext &context) const
{
	if (!symbol_reaper.hasDeadSymbols ()) {
		return;
	}

	ProgramStateRef state = context.getState ();

	/* Iterate through the ErrorMap and find any error symbols which are
	 * dead. */
	ErrorMapTy error_map = state->get<ErrorMap> ();

	for (ErrorMapTy::iterator i = error_map.begin (), e = error_map.end ();
	     i != e; ++i) {
		if (symbol_reaper.isDead (i->first)) {
			state = _error_map_remove (state, i->first);
		}
	}
}

/* Conjure a new symbol to represent a newly allocated GError*.
 * @call_expr may be %NULL if no expression corresponds to the allocation.
 * If non-%NULL, @allocated_sval_out will be filled with the address of an
 * allocated DefinedSVal, which the caller must delete. */
ProgramStateRef
GErrorChecker::_gerror_new (const Expr *call_expr,
                            bool bind_to_call,
                            DefinedSVal **allocated_sval_out,
                            ProgramStateRef state,
                            CheckerContext &context,
                            const SourceRange &source_range) const
{
	DEBUG ("Conjuring new GError* symbol.");

	/* Bind the return value to the symbolic value from the heap region. */
	unsigned int count = context.blockCount ();
	SValBuilder &sval_builder = context.getSValBuilder ();
	const LocationContext *location_context =
		context.getPredecessor ()->getLocationContext ();
	const ASTContext &ast_context = context.getASTContext ();
	SymbolManager &symbol_manager = sval_builder.getSymbolManager ();
	MemRegionManager &memory_manager = sval_builder.getRegionManager ();

	QualType error_type = ast_context.getPointerType (this->_gerror_type);
	assert (Loc::isLocType (error_type));
	assert (SymbolManager::canSymbolicate (error_type));

	SymbolRef allocated_symbol =
		symbol_manager.conjureSymbol (call_expr, location_context,
		                              error_type, count);
	DefinedSVal *allocated_sval =
		new loc::MemRegionVal (memory_manager.getSymbolicHeapRegion (allocated_symbol));

	/* Sanity check: the SVal needs to be usable as a key in the
	 * ErrorMap. */
	assert (allocated_sval->getAsSymbol () != NULL);

	if (bind_to_call) {
		state = state->BindExpr (call_expr,
		                         context.getLocationContext (),
		                         *allocated_sval);
		assert (state != NULL);
	}

	/* Fill the region with the initialization value. */
	state = state->bindDefault (*allocated_sval, UndefinedVal ());

	const MemRegion *allocated_region = allocated_sval->getAsRegion ();
	assert (allocated_region);

	/* Set the region’s extent to sizeof(GError). */
	const SymbolicRegion *symbolic_allocated_region =
		dyn_cast_or_null<SymbolicRegion> (allocated_region);

	if (symbolic_allocated_region != NULL) {
		this->_initialise_identifiers (ast_context);

		DefinedOrUnknownSVal extent =
			symbolic_allocated_region->getExtent (sval_builder);
		const uint64_t _gerror_size =
			ast_context.getTypeSize (this->_gerror_type);
		DefinedOrUnknownSVal gerror_size =
			sval_builder.makeIntVal (_gerror_size,
			                         ast_context.getSizeType ());
		DefinedOrUnknownSVal extent_constraint =
			sval_builder.evalEQ (state, extent, gerror_size);

		state = state->assume (extent_constraint, true);
		assert (state);
	}

	/* Mark the GError* as Set. */
	SymbolRef allocated_sym = allocated_sval->getAsSymbol ();
	assert (allocated_sym);

	state = _error_map_set (state, allocated_sym,
	                        ErrorState::getSet (source_range));

	/* Clean up. */
	if (allocated_sval_out != NULL) {
		*allocated_sval_out = allocated_sval;
	} else {
		delete allocated_sval;
	}

	return state;
}

/* Mark a GError* as freed (but still non-NULL). */
ProgramStateRef
GErrorChecker::_gerror_free (SVal error_location, ProgramStateRef state,
                             CheckerContext &context,
                             const SourceRange &source_range) const
{
	/* Fill the MemRegion with rubbish. */
	if (error_location.getAs<Loc> ()) {
		state = state->bindLoc (error_location.castAs<Loc> (),
		                        UndefinedVal ());
		assert (state != NULL);
	}

	/* Set the region’s state to Freed. */
	SymbolRef error_sym = error_location.getAsSymbol ();
	if (error_sym == NULL) {
		return state;
	}

	return _error_map_set (state, error_sym,
	                       ErrorState::getFreed (source_range));
}

/**
 * Check a GError* is non-NULL and allocated before freeing it.
 *
 * Formally, this checks the conditions:
 *    null_allowed = (error_location = NULL ∨
 *                    (error_location ≠ NULL ∧ valid_allocation(error_location)))
 *    ¬null_allowed = (error_location ≠ NULL ∧ valid_allocation(error_location))
 *
 * Returns: false on a bug, true otherwise
 */
bool
GErrorChecker::_assert_gerror_set (SVal error_location,
                                   bool null_allowed,
                                   ProgramStateRef state,
                                   CheckerContext &context,
                                   const SourceRange &source_range) const
{
	if (error_location.getAs<UndefinedVal> ()) {
		ExplodedNode *error_node = context.generateSink (state);

		this->_initialise_bug_reports ();
		BugReport *R = new BugReport (*this->_use_uninitialised,
		                              "Using uninitialized GError",
		                              error_node);
		#if 0
		bugreporter::trackNullOrUndefValue (error_node, stmt, *R);
		#endif
		R->addRange (source_range);
		context.emitReport (R);

		return false;
	} else if (!error_location.getAs<DefinedOrUnknownSVal> ()) {
		return true;
	}

	ProgramStateRef not_null_state, null_state;

	/* Branch on whether the GError* is NULL. If it is, we have nothing to
	 * do. If it isn’t, it must be a valid allocation. */
	std::tie (not_null_state, null_state) =
		state->assume (error_location.castAs<DefinedOrUnknownSVal> ());
	if (null_state && !not_null_state && !null_allowed) {
		/* Definitely NULL. */
		ExplodedNode *error_node = context.generateSink (state);

		this->_initialise_bug_reports ();
		BugReport *R = new BugReport (*this->_free_cleared,
		                              "Freeing non-set GError",
		                              error_node);
		#if 0
		bugreporter::trackNullOrUndefValue (error_node, stmt, *R);
		#endif
		R->addRange (source_range);
		context.emitReport (R);

		return false;
	} else if (null_state && !not_null_state) {
		/* Definitely NULL. */
		return true;
	}

	/* Check it’s a valid allocation. */
	SymbolRef error_sym = error_location.getAsSymbol ();
	DEBUG ("Asserting GError* is set: SymbolRef:" << error_sym);

	if (error_sym == NULL) {
		return true;
	}

	const ErrorState *error_state = _error_map_get (state, error_sym);

	if (error_state != NULL && error_state->isFreed ()) {
		ExplodedNode *error_node = context.generateSink (state);

		this->_initialise_bug_reports ();
		BugReport *R = new BugReport (*this->_double_free,
		                              "Freeing already-freed GError",
		                              error_node);
		R->addRange (source_range);
		R->addRange (error_state->S);
		context.emitReport (R);

		return false;
	} else if (error_state != NULL && !error_state->isSet ()) {
		ExplodedNode *error_node = context.generateSink (state);

		this->_initialise_bug_reports ();
		BugReport *R = new BugReport (*this->_free_cleared,
		                              "Freeing non-set GError",
		                              error_node);
		R->addRange (source_range);
		R->addRange (error_state->S);
		context.emitReport (R);

		return false;
	}

	return true;
}

/**
 * Check a GError** is clear before overwriting it.
 *
 * Formally, this checks the condition:
 *     ptr_error_location = NULL ∨ (*ptr_error_location) = NULL
 *
 * Returns: false on a bug, true otherwise
 */
bool
GErrorChecker::_assert_gerror_ptr_clear (SVal ptr_error_location,
                                         ProgramStateRef state,
                                         CheckerContext &context,
                                         const SourceRange &source_range) const
{
	ProgramStateRef not_null_state, null_state;  /* for GError* */
	ProgramStateRef ptr_not_null_state, ptr_null_state;  /* for GError** */

	state = context.getState ();

	if (!ptr_error_location.getAs<DefinedOrUnknownSVal> ()) {
		return true;
	}

	DefinedOrUnknownSVal _ptr_error_location =
		ptr_error_location.castAs<DefinedOrUnknownSVal> ();

	/* Branch on whether the GError** is NULL. If it is, we have nothing to
	 * do. */
	std::tie (ptr_not_null_state, ptr_null_state) =
		state->assume (_ptr_error_location);
	if (ptr_null_state && !ptr_not_null_state) {
		/* Definitely NULL. */
		return true;
	}

	/* Check the GError*. */
	SVal error_location =
		this->_error_from_error_ptr (ptr_error_location, context);

	return this->_assert_gerror_unset (error_location, false, state,
	                                   context, source_range);
}

/**
 * Check a GError* is NULL (clear) or unset before overwriting it.
 *
 * Formally, this checks the condition:
 *     undef_allowed = (error_location = NULL ∨
 *                      (error_location ≠ NULL ∧ ¬valid_allocation(error_location)))
 *     ¬undef_allowed = (error_location = NULL)
 *
 * Returns: false on a bug, true otherwise
 */
bool
GErrorChecker::_assert_gerror_unset (SVal error_location,
                                     bool undef_allowed,
                                     ProgramStateRef state,
                                     CheckerContext &context,
                                     const SourceRange &source_range) const
{
	ProgramStateRef not_null_state, null_state;

	/* Branch on whether the GError* is NULL. If it isn’t NULL, there’s a
	 * bug. */
	if (error_location.getAs<UndefinedVal> () && !undef_allowed) {
		ExplodedNode *error_node = context.generateSink (state);

		this->_initialise_bug_reports ();
		BugReport *R = new BugReport (*this->_use_uninitialised,
		                              "Using uninitialized GError",
		                              error_node);
		#if 0
		bugreporter::trackNullOrUndefValue (error_node, stmt, *R);
		#endif
		R->addRange (source_range);
		context.emitReport (R);

		return false;
	} else if (!error_location.getAs<DefinedOrUnknownSVal> ()) {
		return true;
	}

	DefinedOrUnknownSVal _error_location =
		error_location.castAs<DefinedOrUnknownSVal> ();
	std::tie (not_null_state, null_state) = state->assume (_error_location);
	if (null_state && !not_null_state) {
		/* Definitely NULL. */
		return true;
	}

	SymbolRef error_sym = error_location.getAsSymbol ();
	DEBUG ("Asserting GError* is clear: SymbolRef:" << error_sym);

	if (error_sym == NULL) {
		return true;
	}

	const ErrorState *error_state = _error_map_get (state, error_sym);

	if (error_state != NULL && error_state->isSet ()) {
		ExplodedNode *error_node = context.generateSink (state);

		this->_initialise_bug_reports ();
		BugReport *R = new BugReport (*this->_overwrite_set,
		                              "Overwriting already-set GError",
		                              error_node);
		R->addRange (source_range);
		R->addRange (error_state->S);
		context.emitReport (R);

		return false;
	} else if (error_state != NULL && error_state->isFreed () &&
	           !undef_allowed) {
		ExplodedNode *error_node = context.generateSink (state);

		this->_initialise_bug_reports ();
		BugReport *R = new BugReport (*this->_overwrite_freed,
		                              "Overwriting already-freed GError",
		                              error_node);
		R->addRange (source_range);
		R->addRange (error_state->S);
		context.emitReport (R);

		return false;
	}

	return true;
}

/**
 * Check the given error code is a member of a specific error domain.
 *
 * Formally, this checks the condition:
 *     code ϵ error_codes(domain)
 *
 * Returns: false on a bug, true otherwise
 */
bool
GErrorChecker::_assert_code_in_domain (SVal domain,
                                       SVal code,
                                       ProgramStateRef state,
                                       CheckerContext &context,
                                       const SourceRange &domain_source_range,
                                       const SourceRange &code_source_range) const
{
	/* FIXME: Implement. */
	return true;
}

/* Set a GError* to a non-NULL value. */
ProgramStateRef
GErrorChecker::_set_gerror (SVal error_location,
                            DefinedSVal new_error,
                            ProgramStateRef state,
                            CheckerContext &context,
                            const SourceRange &source_range) const
{
	/* Bind the error location to the new error. */
	state = state->bindLoc (error_location, new_error);
	assert (state != NULL);

	/* Constrain the GError* location (lvalue) and rvalue to be non-NULL. */
	SValBuilder &sval_builder = context.getSValBuilder ();

	DefinedOrUnknownSVal error_rvalue_null =
		sval_builder.evalEQ (state,
		                     new_error,
		                     sval_builder.makeNull ());
	state = state->assume (error_rvalue_null, false);
	assert (state != NULL);

	/* Set the error. */
	SymbolRef error_sym = error_location.getAsSymbol ();
	DEBUG ("Setting GError* mapping: SymbolRef: " << error_sym);

	if (error_sym == NULL) {
		return state;
	}

	return _error_map_set (state, error_sym,
	                       ErrorState::getSet (source_range));
}

/* Set a GError* to a NULL value. Note: This does _not_ mark the MemRegion
 * storing the actual GError as freed. Use _gerror_free() for that. */
ProgramStateRef
GErrorChecker::_clear_gerror (SVal error_location,
                              ProgramStateRef state,
                              CheckerContext &context,
                              const SourceRange &source_range) const
{
	/* Bind the GError* to NULL. */
	SValBuilder &sval_builder = context.getSValBuilder ();

	state = state->bindLoc (error_location, sval_builder.makeNull ());
	assert (state != NULL);

	/* Constrain the GError* location (lvalue) to be NULL. */
	if (error_location.getAs<DefinedOrUnknownSVal> ()) {
		DefinedOrUnknownSVal error_null =
			sval_builder.evalEQ (state,
			                     error_location.castAs<DefinedOrUnknownSVal> (),
			                     sval_builder.makeNull ());
		ProgramStateRef new_state = state->assume (error_null, true);
		if (new_state != NULL) {
			state = new_state;
		}
	} else {
		DEBUG ("Couldn’t get DefinedOrUnknownSVal for error.");
	}

	/* Clear the error. */
	DEBUG ("Clearing GError* mapping.");

	SymbolRef error_sym = error_location.getAsSymbol ();
	if (error_sym == NULL) {
		return state;
	}

	return _error_map_set (state, error_sym,
	                       ErrorState::getClear (source_range));
}

void
GErrorChecker::_initialise_identifiers (const ASTContext &context) const
{
	if (!this->_gerror_type.isNull ()) {
		return;
	}

	TypeManager manager = TypeManager (context);

	/* Types. */
	this->_gerror_type = manager.find_type_by_name ("GError");
	assert (!this->_gerror_type.isNull ());

	/* Functions. */
	this->_identifier_g_set_error =
		&context.Idents.get ("g_set_error");
	this->_identifier_g_set_error_literal =
		&context.Idents.get ("g_set_error_literal");
	this->_identifier_g_error_new =
		&context.Idents.get ("g_error_new");
	this->_identifier_g_error_new_literal =
		&context.Idents.get ("g_error_new_literal");
	this->_identifier_g_error_new_valist =
		&context.Idents.get ("g_error_new_valist");
	this->_identifier_g_error_free =
		&context.Idents.get ("g_error_free");
	this->_identifier_g_clear_error =
		&context.Idents.get ("g_clear_error");
	this->_identifier_g_propagate_error =
		&context.Idents.get ("g_propagate_error");
	this->_identifier_g_propagate_prefixed_error =
		&context.Idents.get ("g_propagate_prefixed_error");
}

void
GErrorChecker::_initialise_bug_reports () const
{
	if (this->_overwrite_set) {
		return;
	}

	this->_overwrite_set.reset (
		new BuiltinBug (this->filter.check_name_overwrite_set,
		                Debug::Categories::GError,
		                "Try to assign over the top of an existing "
		                "GError. Causes loss of error information and "
		                "a memory leak."));
	this->_overwrite_freed.reset (
		new BuiltinBug (this->filter.check_name_overwrite_freed,
		                Debug::Categories::GError,
		                "Try to assign over the top of an existing "
		                "GError which has been freed but not cleared "
		                "to NULL. g_set_error(!NULL) is not allowed."));
	this->_double_free.reset (
		new BuiltinBug (this->filter.check_name_double_free,
		                Debug::Categories::GError,
		                "Try to free a GError which has already been "
		                "freed. Causes heap corruption."));
	this->_free_cleared.reset (
		new BuiltinBug (this->filter.check_name_free_cleared,
		                Debug::Categories::GError,
		                "Try to free a GError which has been cleared to"
		                "NULL. g_error_free(NULL) is not allowed."));
	this->_use_uninitialised.reset (
		new BuiltinBug (this->filter.check_name_use_uninitialised,
		                Debug::Categories::GError,
		                "Try to use a GError which has not been "
		                "initialized to NULL. Causes spurious "
		                "error reports."));
	this->_memory_leak.reset (
		new BuiltinBug (this->filter.check_name_memory_leak,
		                Debug::Categories::GError,
		                "Fail to free a GError before it goes out of "
		                "scope."));
}

} /* namespace tartan */
