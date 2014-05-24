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
 * GSignalVisitor:
 *
 * This is a checker for GObject signal connection calls. For calls to functions
 * such as g_signal_connect(), it validates that:
 *  • The signal name exists on the given GObject.
 *  • The type of the callback function matches the signal declaration.
 * It requires the signal name to be a string literal, and will only work if the
 * GObject parameter (the first parameter to g_signal_connect()) has the most
 * specific type possible, so it can look up the signals for that GObject
 * subclass.
 *
 * The type of the user_data is not validated, but could be checked by a
 * separate plugin for closure types.
 *
 * The GObject type is resolved and the signal name is looked up on it. If both
 * these operations succeed, the type of the callback function is checked
 * against the signal declaration in the GIR file. A warning is emitted if the
 * callback function can’t be resolved (i.e. if a variable is passed instead of
 * a function pointer), or if the function declaration is old-style (without
 * arguments).
 *
 * Formally, given a connection call with types representing variables:
 *     g_signal_connect (A, "O::signal-name", callback, U_1)
 * and a callback defined as:
 *     R callback (B, …, U_2)
 * the following type relationships must hold:
 *     U_1 <: U_2 or U_2 = gpointer
 *     A <: O    or it won’t have the signal
 *     A <: B    or the callback may call invalid methods
 *     B <: O    or it won’t have the signal
 */

#include <cstring>

#include <clang/AST/Attr.h>

#include <glib.h>

#include <girepository.h>

#include "debug.h"
#include "gsignal-checker.h"

namespace tartan {

/* Information about the GSignal functions we’re interested in. If you want to
 * add support for a new GSignal function, it may be enough to add a new
 * element here. */
typedef struct {
	/* C name of the function */
	const char *func_name;
	/* Zero-based index of the GObject instance parameter. */
	unsigned int gobject_param_index;
	/* Zero-based index of the signal name parameter. */
	unsigned int signal_name_param_index;
	/* Zero-based index of the callback function pointer parameter. */
	unsigned int callback_param_index;
} SignalFuncInfo;

static const SignalFuncInfo gsignal_connect_funcs[] = {
	{ "g_signal_connect", 0, 1, 2 },
	{ "g_signal_connect_after", 0, 1, 2 },
	{ "g_signal_connect_swapped", 0, 1, 2 },
	{ "g_signal_connect_object", 0, 1, 2 },
	{ "g_signal_connect_data", 0, 1, 2 },
/* FIXME add support for these:
	{ "g_signal_connect_closure", 0, 1, _ },
	{ "g_signal_connect_closure_by_id", 0, _, _ },
*/
};


static const SignalFuncInfo *
_func_is_gsignal_connect (const FunctionDecl& func)
{
	const char *func_name = func.getNameAsString ().c_str ();
	guint i;

	/* Fast path elimination of irrelevant functions. */
	if (*func_name != 'g')
		return NULL;

	for (i = 0; i < G_N_ELEMENTS (gsignal_connect_funcs); i++) {
		if (strcmp (func_name, gsignal_connect_funcs[i].func_name) == 0) {
			return &gsignal_connect_funcs[i];
		}
	}

	return NULL;
}

/* If an expression is a reference to a GObject (or subclass), return the most
 * specific type information we can for that object. This must be freed with
 * g_base_info_unref().
 *
 * If the expression is not a GObject, return NULL. */
static GIObjectInfo*
_expr_to_gobject_type (const Expr *expr,
                       const ASTContext &context,
                       const GirManager &gir_manager)
{
	QualType gobject_type = expr->getType ();

	while (gobject_type->isPointerType ()) {
		gobject_type = gobject_type->getPointeeType ();
	}

	/* We have the GObject pointee type, so try and resolve it. */
	std::string gobject_type_str =
		gobject_type.getUnqualifiedType ().getAsString ();
	return gir_manager.find_object_info (gobject_type_str);
}

/* Look up a named signal in a GIObjectInfo. The return value must be freed
 * using g_base_info_unref(). The GIObjectInfo for the GObject subclass which
 * actually defines that signal will be returned in @static_gobject_info iff the
 * return value is non-NULL. Any value returned in @static_gobject_info must be
 * freed using g_base_info_unref(). */
static GISignalInfo *
_gobject_look_up_signal (GIObjectInfo *dynamic_gobject_info,
                         GIObjectInfo **static_gobject_info,
                         const gchar *signal_name)
{
	GISignalInfo *signal_info;
	gint n_signals = g_object_info_get_n_signals (dynamic_gobject_info);

	for (gint i = 0; i < n_signals; i++) {
		signal_info = g_object_info_get_signal (dynamic_gobject_info,
		                                        i);

		if (strcmp (signal_name,
		            g_base_info_get_name (signal_info)) == 0) {
			/* Found the signal. */
			*static_gobject_info =
				g_base_info_ref (dynamic_gobject_info);
			return signal_info;
		}

		g_base_info_unref (signal_info);
	}

	/* If the object has a parent class, try that. */
	dynamic_gobject_info = g_object_info_get_parent (dynamic_gobject_info);
	if (dynamic_gobject_info == NULL) {
		*static_gobject_info = NULL;
		return NULL;
	}

	signal_info = _gobject_look_up_signal (dynamic_gobject_info,
	                                       static_gobject_info,
	                                       signal_name);
	g_base_info_unref (dynamic_gobject_info);

	return signal_info;
}

/* Look up the #QualType representing the type in @type_info, which must be
 * a %GI_TYPE_TAG_INTERFACE info. If type lookup fails, a null type is
 * returned. */
static QualType
_type_interface_info_to_type (GITypeInfo *type_info,
                              const ASTContext &context,
                              const GirManager &gir_manager,
                              TypeManager &type_manager)
{
	GIBaseInfo *interface_info;
	QualType retval;

	interface_info = g_type_info_get_interface (type_info);
	assert (interface_info != NULL);

	switch (g_base_info_get_type (interface_info)) {
	case GI_INFO_TYPE_CALLBACK:
	case GI_INFO_TYPE_STRUCT:
	case GI_INFO_TYPE_BOXED:
	case GI_INFO_TYPE_ENUM:
	case GI_INFO_TYPE_FLAGS:
	case GI_INFO_TYPE_OBJECT:
	case GI_INFO_TYPE_INTERFACE:
	case GI_INFO_TYPE_UNION: {
		std::string c_type (gir_manager.get_c_name_for_type (interface_info));
		retval = type_manager.find_pointer_type_by_name (c_type);
		break;
	}
	case GI_INFO_TYPE_FUNCTION:
	case GI_INFO_TYPE_CONSTANT:
	case GI_INFO_TYPE_VALUE:
	case GI_INFO_TYPE_SIGNAL:
	case GI_INFO_TYPE_VFUNC:
	case GI_INFO_TYPE_PROPERTY:
	case GI_INFO_TYPE_FIELD:
	case GI_INFO_TYPE_ARG:
	case GI_INFO_TYPE_TYPE:
		/* Never expect these to be argument types. */
	case GI_INFO_TYPE_INVALID_0:
	case GI_INFO_TYPE_UNRESOLVED:
	case GI_INFO_TYPE_INVALID:
		/* These are all invalid */
	default:
		llvm::errs () << "Warning: Unexpected base info type " <<
			g_base_info_get_type (interface_info) <<
			" for base info " <<
			g_base_info_get_name (interface_info) << ".\n";
	}

	g_base_info_unref (interface_info);

	if (g_type_info_is_pointer (type_info)) {
		retval = context.getPointerType (retval);
	}

	return retval;
}

static QualType
_type_info_to_type (GITypeInfo *type_info,
                    const ASTContext &context,
                    const GirManager &gir_manager,
                    TypeManager &type_manager);

/* Look up the #QualType representing the type in @array_info, which must be
 * a %GI_TYPE_TAG_ARRAY info. If type lookup fails, a null type is returned. */
static QualType
_type_array_info_to_type (GITypeInfo *array_info,
                          const ASTContext &context,
                          const GirManager &gir_manager,
                          TypeManager &type_manager)
{
	switch (g_type_info_get_array_type (array_info)) {
	case GI_ARRAY_TYPE_C: {
		/* FIXME: Really not sure if this is correct. */
		GITypeInfo *param_type = g_type_info_get_param_type (array_info,
		                                                     0);
		QualType element_type = _type_info_to_type (param_type, context,
		                                            gir_manager,
		                                            type_manager);
		g_base_info_unref (param_type);

		if (element_type.isNull ()) {
			return QualType ();
		}

		/* Handle the array length. */
		gint fixed_size = g_type_info_get_array_fixed_size (array_info);
		/* FIXME: Probably can’t do anything with
		 * g_type_info_get_array_length() because it requires an Expr
		 * for the length, which I don’t think we can retrieve if we’re
		 * examining a callback type (as opposed to a call itself). */

		if (fixed_size > -1) {
			return context.getConstantArrayType (element_type,
			                                     llvm::APInt (32, fixed_size),
			                                     ArrayType::ArraySizeModifier::Static,
			                                     0);
		} else {
			return context.getIncompleteArrayType (element_type,
			                                       ArrayType::ArraySizeModifier::Static,
			                                       0);
		}
	}
	case GI_ARRAY_TYPE_ARRAY:
		return type_manager.find_pointer_type_by_name ("GArray");
	case GI_ARRAY_TYPE_PTR_ARRAY:
		return type_manager.find_pointer_type_by_name ("GPtrArray");
	case GI_ARRAY_TYPE_BYTE_ARRAY:
		return type_manager.find_pointer_type_by_name ("GByteArray");
	default:
		llvm::errs () << "Warning: Unexpected array type " <<
			g_type_info_get_array_type (array_info) <<
			" for base info " <<
			g_base_info_get_name (array_info) << ".\n";
		return QualType ();
	}
}

/* Look up the #QualType representing the type in @type_info, which can have any
 * type tag. If type lookup fails, a null type is returned. */
static QualType
_type_info_to_type (GITypeInfo *type_info,
                    const ASTContext &context,
                    const GirManager &gir_manager,
                    TypeManager &type_manager)
{
	switch (g_type_info_get_tag (type_info)) {
	/* Basic types. */
	case GI_TYPE_TAG_VOID:
		return context.VoidTy;
	case GI_TYPE_TAG_BOOLEAN:
		return context.IntTy;
	case GI_TYPE_TAG_INT8:
		return context.getIntTypeForBitwidth (8, true);
	case GI_TYPE_TAG_UINT8:
		return context.getIntTypeForBitwidth (8, false);
	case GI_TYPE_TAG_INT16:
		return context.getIntTypeForBitwidth (16, true);
	case GI_TYPE_TAG_UINT16:
		return context.getIntTypeForBitwidth (16, false);
	case GI_TYPE_TAG_INT32:
		return context.getIntTypeForBitwidth (32, true);
	case GI_TYPE_TAG_UINT32:
		return context.getIntTypeForBitwidth (32, false);
	case GI_TYPE_TAG_INT64:
		return context.getIntTypeForBitwidth (64, true);
	case GI_TYPE_TAG_UINT64:
		return context.getIntTypeForBitwidth (64, false);
	case GI_TYPE_TAG_FLOAT:
		return context.FloatTy;
	case GI_TYPE_TAG_DOUBLE:
		return context.DoubleTy;
	case GI_TYPE_TAG_GTYPE:
		/* FIXME: The type of GType can differ on different platforms
		 * and under different languages. */
		return context.getSizeType ();
	case GI_TYPE_TAG_UTF8:
	case GI_TYPE_TAG_FILENAME:
		return context.getPointerType (context.CharTy);
	case GI_TYPE_TAG_UNICHAR:
		return context.getIntTypeForBitwidth (32, false);
	/* Non-basic types */
	case GI_TYPE_TAG_ARRAY:
		return _type_array_info_to_type (type_info, context,
		                                 gir_manager, type_manager);
	case GI_TYPE_TAG_INTERFACE:
		return _type_interface_info_to_type (type_info, context,
		                                     gir_manager, type_manager);
	case GI_TYPE_TAG_GLIST:
		return type_manager.find_pointer_type_by_name ("GList");
	case GI_TYPE_TAG_GSLIST:
		return type_manager.find_pointer_type_by_name ("GSList");
	case GI_TYPE_TAG_GHASH:
		return type_manager.find_pointer_type_by_name ("GHashTable");
	case GI_TYPE_TAG_ERROR: {
		QualType qt = type_manager.find_pointer_type_by_name ("GError");

		if (!qt.isNull ()) {
			return context.getPointerType (qt);
		}

		return QualType ();
	}
	default:
		llvm::errs () << "Warning: Unexpected base info type " <<
			g_base_info_get_type (type_info) <<
			" for base info " <<
			g_base_info_get_name (type_info) << ".\n";
		return QualType ();
	}
}

/* Returns true iff @a is equal to, or a subclass of, @b. */
static bool
_is_gobject_subclass (GIBaseInfo *a, GIBaseInfo *b)
{
	assert (g_base_info_get_type (a) == GI_INFO_TYPE_OBJECT);
	assert (g_base_info_get_type (b) == GI_INFO_TYPE_OBJECT);

	if (g_base_info_equal (a, b)) {
		return true;
	}

	GIObjectInfo *ap = g_object_info_get_parent ((GIObjectInfo *) a);

	if (ap == NULL) {
		return false;
	}

	bool retval = _is_gobject_subclass (ap, b);
	g_base_info_unref (ap);

	return retval;
}

/* Check the type of the callback in @expr (which is assumed to be a function
 * pointer or cast of a function pointer), asserting that it matches the type
 * of @signal_info.
 *
 * @dynamic_gobject_info is information about the GObject subclass being passed
 * to g_signal_connect(). @static_gobject_info is information about the GObject
 * subclass which the signal is defined on. @dynamic_gobject_info should be a
 * (non-strict) subclass of @static_gobject_info.
 *
 * Returns true if the callback has the correct type. Emits an error and returns
 * false otherwise. */
static bool
_check_signal_callback_type (const Expr *expr,
                             GIBaseInfo *dynamic_gobject_info,
                             GIBaseInfo *static_gobject_info,
                             GISignalInfo *signal_info,
                             CompilerInstance &compiler,
                             const ASTContext &context,
                             const GirManager &gir_manager,
                             TypeManager &type_manager)
{
	const FunctionProtoType *callback_type = NULL;
	SourceRange decl_range;  /* for the callback definition */

	/* We can’t just use expr->getType() here because we’ll typically get
	 * GCallback as the type, which is not helpful. */
	switch (expr->getStmtClass ()) {
	case Stmt::StmtClass::DeclRefExprClass: {
		/* A reference to a function. Check the variable is a pointer
		 * and look it up in the GIR namespace. */
		const DeclRefExpr *decl_ref_expr = cast<DeclRefExpr> (expr);
		const ValueDecl *value_decl = decl_ref_expr->getDecl ();
		QualType value_type = value_decl->getType ();

		if (value_type->isFunctionNoProtoType ()) {
			/* Warning. */

			/* TODO: Emit expected type of signal callback? */
			Debug::emit_warning ("Could not check type of handler "
			                     "for signal ‘%0::%1’. Callback "
			                     "function declaration does not "
			                     "contain parameter types.",
			                     compiler, expr->getLocStart ())
			<< gir_manager.get_c_name_for_type (static_gobject_info)
			<< g_base_info_get_name (signal_info)
			<< decl_range;

			return false;
		} else if (!value_type->isFunctionProtoType ()) {
			/* Error. */
			WARN_EXPR (__func__ << "() can’t handle value "
			           "declarations of type ‘" <<
			           value_type.getAsString () << "’.",
			           *expr);
			return false;
		}

		callback_type = cast<FunctionProtoType> (value_type);
		decl_range = cast<FunctionDecl> (value_decl)->getCanonicalDecl ()->getSourceRange ();

		break;
	}
	case Stmt::StmtClass::ParenExprClass: {
		/* A parenthesised expression. */
		const ParenExpr *paren_expr = cast<ParenExpr> (expr);

		return _check_signal_callback_type (paren_expr->getSubExpr (),
		                                    dynamic_gobject_info,
		                                    static_gobject_info,
		                                    signal_info, compiler,
		                                    context, gir_manager,
		                                    type_manager);
	}
	case Stmt::StmtClass::ImplicitCastExprClass:
	case Stmt::StmtClass::CStyleCastExprClass: {
		/* A cast (explicit or C-style). */
		const CastExpr *cast_expr = cast<CastExpr> (expr);

		return _check_signal_callback_type (cast_expr->getSubExprAsWritten (),
		                                    dynamic_gobject_info,
		                                    static_gobject_info,
		                                    signal_info, compiler,
		                                    context, gir_manager,
		                                    type_manager);
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN_EXPR (__func__ << "() can’t handle expressions of type " <<
		           expr->getStmtClassName (), *expr);
		return false;
	}

	/* Check the function type against the signal info. Add 2 to n_args
	 * because GIR omits the ‘self’ and ‘user_data’ arguments. */
	GICallableInfo *callable_info = signal_info;
	guint n_args = g_callable_info_get_n_args (callable_info) + 2;
	GITypeInfo expected_type_info;
	QualType actual_type, expected_type;

	if (n_args != callback_type->getNumArgs ()) {
		/* Error. */

		/* TODO: Emit expected type of signal callback? */
		Debug::emit_error ("Incorrect number of arguments in signal "
		                   "handler for signal ‘%0::%1’. Expected %2 "
		                   "but saw %3.",
		                   compiler, expr->getLocStart ())
		<< gir_manager.get_c_name_for_type (static_gobject_info)
		<< g_base_info_get_name (signal_info)
		<< n_args
		<< callback_type->getNumArgs ()
		<< decl_range;

		return false;
	}

	/* Check all arguments */
	for (guint i = 0; i < n_args; i++) {
		const gchar *arg_name;
		bool type_error;

		actual_type = callback_type->getArgType (i);

		if (i == 0) {
			/* First argument is always a pointer to the GObject
			 * instance which the signal is defined on. */
			std::string c_type (gir_manager.get_c_name_for_type (static_gobject_info));
			expected_type = type_manager.find_pointer_type_by_name (c_type);
			arg_name = "self";

			QualType atp = actual_type;
			while (atp->isPointerType ()) {
				atp = atp->getPointeeType ();
			}

			std::string actual_type_str =
				atp.getUnqualifiedType ().getAsString ();
			GIBaseInfo *actual_type_info =
				gir_manager.find_object_info (actual_type_str);

			if (actual_type_info == NULL) {
				/* Error. */

				/* TODO: Emit expected type of signal callback? */
				Debug::emit_warning ("Failed to resolve type "
				                     "of argument ‘%0’ in "
				                     "signal handler for "
				                     "signal ‘%1::%2’. Cannot "
				                     "find type with name "
				                     "‘%3’.", compiler,
				                     expr->getLocStart ())
				<< arg_name
				<< c_type
				<< g_base_info_get_name (signal_info)
				<< actual_type_str
				<< decl_range;

				continue;
			}

			DEBUG ("Checking expected subclass relationships ‘" <<
			       g_base_info_get_name (dynamic_gobject_info) <<
			       "’ <: ‘" <<
			       g_base_info_get_name (actual_type_info) <<
			       "’ <: ‘" <<
			       g_base_info_get_name (static_gobject_info) <<
			       "’.");

			/* See the documentation at the top of the file for an
			 * explanation of the (non-trivial) GObject type
			 * checking for the first parameter. */
			type_error = (actual_type_info == NULL ||
			              atp.isConstQualified () ||
			              !_is_gobject_subclass (dynamic_gobject_info,
			                                     actual_type_info) ||
			              !_is_gobject_subclass (actual_type_info,
			                                     static_gobject_info));

			g_base_info_unref (actual_type_info);
		} else if (i == n_args - 1) {
			/* Final argument is always a gpointer user_data. */
			expected_type = context.getPointerType (context.VoidTy);
			arg_name = "user_data";

			DEBUG ("Comparing expected ‘" <<
			       expected_type.getAsString () << "’ with actual "
			       "‘" << actual_type.getAsString () << "’.");

			/* Although technically the callback function should
			 * take a gpointer user_data argument, ignore cases 
			 * where it takes a more specific *pointer* type, since
			 * it’s a common practice which causes no problems. This
			 * eliminates a huge number of false positives. */
			type_error = !(context.hasSameType (actual_type,
			                                    expected_type) ||
			               actual_type->isPointerType ());
		} else {
			/* All other arguments. */
			GIArgInfo arg_info;

			g_callable_info_load_arg (callable_info, i - 1,
			                          &arg_info);
			g_arg_info_load_type (&arg_info, &expected_type_info);

			arg_name = g_base_info_get_name (&arg_info);

			expected_type = _type_info_to_type (&expected_type_info,
			                                    context,
			                                    gir_manager,
			                                    type_manager);

			if (expected_type.isNull ()) {
				/* Error. */

				/* TODO: Emit expected type of signal callback? */
				Debug::emit_warning ("Failed to resolve type "
				                     "of argument ‘%0’ in "
				                     "signal handler for "
				                     "signal ‘%1::%2’. Cannot "
				                     "find type with name "
				                     "‘%3’.", compiler,
				                     expr->getLocStart ())
				<< arg_name
				<< gir_manager.get_c_name_for_type (static_gobject_info)
				<< g_base_info_get_name (signal_info)
				<< g_base_info_get_name (&expected_type_info)
				<< decl_range;

				continue;
			}

			DEBUG ("Comparing expected ‘" <<
			       expected_type.getAsString () << "’ with actual "
			       "‘" << actual_type.getAsString () << "’.");

			/* Perform the check. */
			type_error = (expected_type.isNull () ||
			              !context.hasSameType (actual_type,
			                                    expected_type));
		}

		/* Return as soon as the first error is encountered, since it’s
		 * likely the user’s used completely the wrong callback type,
		 * so further errors would just be noise. */
		if (type_error) {
			/* Error. */

			/* TODO: Emit expected type of signal callback? */
			Debug::emit_error ("Incorrect type for argument ‘%0’ "
			                   "in signal handler for signal "
			                   "‘%1::%2’. Expected ‘%3’ but saw "
			                   "‘%4’.", compiler,
			                   expr->getLocStart ())
			<< arg_name
			<< gir_manager.get_c_name_for_type (static_gobject_info)
			<< g_base_info_get_name (signal_info)
			<< expected_type.getAsString ()
			<< actual_type.getAsString ()
			<< decl_range;

			return false;
		}
	}

	/* Return type. */
	g_callable_info_load_return_type (callable_info, &expected_type_info);
	actual_type = callback_type->getResultType ();
	expected_type = _type_info_to_type (&expected_type_info, context,
	                                    gir_manager, type_manager);
	if (expected_type.isNull ()) {
		/* Error. */

		/* TODO: Emit expected type of signal callback? */
		Debug::emit_warning ("Failed to resolve return type in signal "
		                     "handler for signal ‘%0::%1’. Cannot find "
		                     "type with name ‘%2’.", compiler,
		                     expr->getLocStart ())
		<< gir_manager.get_c_name_for_type (static_gobject_info)
		<< g_base_info_get_name (signal_info)
		<< g_base_info_get_name (&expected_type_info)
		<< decl_range;

		return false;
	}

	if (!context.hasSameType (actual_type, expected_type)) {
		/* Error. */

		/* TODO: Emit expected type of signal callback? */
		Debug::emit_error ("Incorrect return type from signal handler "
		                   "for signal ‘%0::%1’. Expected ‘%2’ but saw "
		                   "‘%3’.", compiler, expr->getLocStart ())
		<< gir_manager.get_c_name_for_type (static_gobject_info)
		<< g_base_info_get_name (signal_info)
		<< expected_type.getAsString ()
		<< actual_type.getAsString ()
		<< decl_range;

		return false;
	}

	return true;
}

/* Check the type of the function pointer passed to a g_signal_connect() call,
 * and ensure that its declaration matches the signal definition.
 *
 * If the signal name string is not a string literal, or if the concrete type
 * of the GObject is not known, we can’t check anything. */
static bool
_check_gsignal_callback_type (const CallExpr &call,
                              const FunctionDecl &func,
                              const SignalFuncInfo *func_info,
                              CompilerInstance &compiler,
                              const ASTContext &context,
                              const GirManager &gir_manager,
                              TypeManager &type_manager)
{
	const Expr *callback_arg, *gobject_arg, *signal_name_arg;

	callback_arg = call.getArg (func_info->callback_param_index);
	gobject_arg = call.getArg (func_info->gobject_param_index);
	signal_name_arg = call.getArg (func_info->signal_name_param_index);

	/* Check if the signal name is a string literal. If not, we can’t check
	 * it. */
	const StringLiteral *signal_name_str =
		dyn_cast<StringLiteral> (signal_name_arg->IgnoreParenImpCasts ());
	if (signal_name_str == NULL) {
		/* Warning. */
		Debug::emit_warning ("Non-string literal passed to signal "
		                     "name parameter. This is not an error "
		                     "but is highly unusual.",
		                     compiler, signal_name_arg->getLocStart ());

		return false;
	}

	/* Sort out the signal name, splitting off the detail if necessary. */
	StringRef signal_name_str_ref = signal_name_str->getString ();
	const std::string signal_name_and_detail = signal_name_str_ref.str ();

	std::string::size_type d = signal_name_and_detail.find ("::");
	std::string signal_name;

	if (d != std::string::npos) {
		/* Strip off the detail string.
		 *
		 * FIXME: In future we could validate this. e.g. For the
		 * ‘notify’ signal, validate it against the object’s
		 * properties. */
		signal_name = signal_name_and_detail.substr (0, d);
	} else {
		signal_name = signal_name_and_detail;
	}

	DEBUG ("Using signal name ‘" << signal_name << "’.");

	/* Try and grab the GObject parameter’s type. This is the type of the
	 * variable passed into g_signal_connect(). The @static_gobject_info is
	 * the type of the GObject subclass which defines the signal. */
	GIObjectInfo *dynamic_gobject_info, *static_gobject_info = NULL;

	dynamic_gobject_info = _expr_to_gobject_type (gobject_arg->IgnoreParenImpCasts (),
	                                              context, gir_manager);
	if (dynamic_gobject_info == NULL) {
		/* Warning. */
		Debug::emit_warning ("Could not find GObject subclass for "
		                     "expression when connecting to signal "
		                     "‘%0’. To improve static analysis, add a "
		                     "typecast to the GObject parameter of "
		                     "%1().", compiler, call.getLocStart ())
		<< signal_name
		<< func_info->func_name
		<< gobject_arg->getSourceRange ()
		<< signal_name_arg->getSourceRange ();

		return false;
	}

	DEBUG ("Using GIObjectInfo ‘" <<
	       g_base_info_get_name ((GIBaseInfo *) dynamic_gobject_info) <<
	       "’ from namespace ‘" <<
	       g_base_info_get_namespace ((GIBaseInfo *) dynamic_gobject_info) <<
	       "’.");

	/* Find the signal in the GObject. */
	GISignalInfo *signal_info;

	signal_info = _gobject_look_up_signal (dynamic_gobject_info,
	                                       &static_gobject_info,
	                                       signal_name.c_str ());
	if (signal_info == NULL) {
		/* Warning. */
		Debug::emit_warning ("No signal named ‘%0’ in GObject class "
		                     "‘%1’. To improve static analysis, add a "
		                     "typecast to the GObject parameter of "
		                     "%2().", compiler, call.getLocStart ())
		<< signal_name
		<< gir_manager.get_c_name_for_type (dynamic_gobject_info)
		<< func_info->func_name
		<< gobject_arg->getSourceRange ()
		<< signal_name_arg->getSourceRange ();

		g_base_info_unref (dynamic_gobject_info);

		return false;
	}

	DEBUG ("Using GISignalInfo ‘" <<
	       g_base_info_get_name ((GIBaseInfo *) signal_info) <<
	       "’ from namespace ‘" <<
	       g_base_info_get_namespace ((GIBaseInfo *) signal_info) <<
	       "’.");

	/* Check the callback’s type. */
	if (!_check_signal_callback_type (callback_arg->IgnoreParenImpCasts (),
	                                  dynamic_gobject_info,
	                                  static_gobject_info, signal_info,
	                                  compiler, context, gir_manager,
	                                  type_manager)) {
		/* A diagnostic has already been emitted by
		 * _check_signal_callback_type(). */
		g_base_info_unref (signal_info);
		g_base_info_unref (dynamic_gobject_info);
		g_base_info_unref (static_gobject_info);

		return false;
	}

	g_base_info_unref (signal_info);
	g_base_info_unref (dynamic_gobject_info);
	g_base_info_unref (static_gobject_info);

	return true;
}

void
GSignalConsumer::HandleTranslationUnit (ASTContext& context)
{
	/* Run away if the plugin is disabled. */
	if (!this->is_enabled ()) {
		return;
	}

	this->_visitor.TraverseDecl (context.getTranslationUnitDecl ());
}

/* Note: Specifically overriding the Traverse* method here to re-implement
 * recursion to child nodes. */
bool
GSignalVisitor::VisitCallExpr (CallExpr* expr)
{
	const SignalFuncInfo *func_info;

	/* Can only handle direct function calls (i.e. not calling dereferenced
	 * function pointers). */
	const FunctionDecl *func = expr->getDirectCallee ();
	if (func == NULL)
		return true;

	/* We’re only interested in functions which connect signals. */
	func_info = _func_is_gsignal_connect (*func);
	if (func_info == NULL)
		return true;

	/* Check the callback type. */
	const GirManager *gir_manager = this->_gir_manager.get ();
	_check_gsignal_callback_type (*expr, *func, func_info, this->_compiler,
	                              func->getASTContext (),
	                              *gir_manager, this->_type_manager);

	return true;
}

} /* namespace tartan */
