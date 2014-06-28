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
 * The type of the user_data is not validated (other than requiring it to be a
 * pointer type, such as gpointer), but could be checked by a separate plugin
 * for closure types.
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
 *
 * If %G_CONNECT_SWAPPED is specified in the g_signal_connect() flags, the same
 * relationships hold but using the connection call definition:
 *     g_signal_connect (A, "O::signal-name", callback, U_1)
 * and a callback defined as:
 *     R callback (U_2, …, B)
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
	/* Zero-based index of the flags parameter, or -1 if there is none. */
	int flags_param_index;
	/* Zero-based index of the user_data parameter. */
	unsigned int user_data_param_index;
} SignalFuncInfo;

static const SignalFuncInfo gsignal_connect_funcs[] = {
	{ "g_signal_connect", 0, 1, 2, -1, 3 },
	{ "g_signal_connect_after", 0, 1, 2, -1, 3 },
	{ "g_signal_connect_swapped", 0, 1, 2, -1, 3 },
	{ "g_signal_connect_object", 0, 1, 2, 4, 3 },
	{ "g_signal_connect_data", 0, 1, 2, 5, 3 },
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

/* If an expression is a reference to a GObject (or subclass, or a GInterface),
 * return the most specific type information we can for that object (or
 * interface). This must be freed with g_base_info_unref().
 *
 * If the expression is not a GObject, return NULL. */
static GIObjectInfo*
_expr_to_gtype (const Expr *expr,
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

/* Look up a named signal in a #GIObjectInfo or #GIInterfaceInfo,
 * @dynamic_instance_info.
 *
 * If no definition for the signal can be found, %NULL will be returned.
 *
 * If the signal is defined on a #GObject, the #GIObjectInfo for that object
 * will be returned in @static_instance_info. In this case,
 * @static_instance_info is guaranteed to be the same as, or a superclass of,
 * @dynamic_instance_info.
 *
 * If the signal is defined on a #GInterface, the #GIInterfaceInfo for that
 * interface will be returned in @static_instance_info. In this case,
 * @dynamic_instance_info could have been the interface, or any class which
 * implements it.
 *
 * Any value returned in @static_instance_info must be freed using
 * g_base_info_unref(). The returned #GISignalInfo must be freed using
 * g_base_info_unref() as well. */
static GISignalInfo *
_gtype_look_up_signal (GIRegisteredTypeInfo *dynamic_instance_info,
                       GIRegisteredTypeInfo **static_instance_info,
                       const gchar *signal_name)
{
	GISignalInfo *signal_info;
	gint n_signals;

	if (GI_IS_OBJECT_INFO (dynamic_instance_info)) {
		n_signals = g_object_info_get_n_signals (dynamic_instance_info);
	} else if (GI_IS_INTERFACE_INFO (dynamic_instance_info)) {
		n_signals = g_interface_info_get_n_signals (dynamic_instance_info);
	} else {
		g_assert_not_reached ();
	}

	for (gint i = 0; i < n_signals; i++) {
		if (GI_IS_OBJECT_INFO (dynamic_instance_info)) {
			signal_info = g_object_info_get_signal (dynamic_instance_info, i);
		} else if (GI_IS_INTERFACE_INFO (dynamic_instance_info)) {
			signal_info = g_interface_info_get_signal (dynamic_instance_info, i);
		} else {
			g_assert_not_reached ();
		}

		if (strcmp (signal_name,
		            g_base_info_get_name (signal_info)) == 0) {
			/* Found the signal. */
			*static_instance_info =
				g_base_info_ref (dynamic_instance_info);
			return signal_info;
		}

		g_base_info_unref (signal_info);
	}

	if (GI_IS_OBJECT_INFO (dynamic_instance_info)) {
		/* If the object implements any interfaces, try those. */
		for (gint i = 0;
		     i < g_object_info_get_n_interfaces (dynamic_instance_info);
		     i++) {
			GIInterfaceInfo *b;

			b = g_object_info_get_interface (dynamic_instance_info, i);
			signal_info = _gtype_look_up_signal (b,
			                                     static_instance_info,
			                                     signal_name);
			g_base_info_unref (b);

			if (signal_info != NULL) {
				return signal_info;
			}
		}

		/* If the object has a parent class, try that. */
		dynamic_instance_info = g_object_info_get_parent (dynamic_instance_info);
		if (dynamic_instance_info != NULL) {
			signal_info = _gtype_look_up_signal (dynamic_instance_info,
			                                     static_instance_info,
			                                     signal_name);
			g_base_info_unref (dynamic_instance_info);

			return signal_info;
		}
	}

	/* Found nothing. */
	*static_instance_info = NULL;
	return NULL;
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
		return context.getPointerType (context.getConstType (context.CharTy));
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
	case GI_TYPE_TAG_ERROR:
		return type_manager.find_pointer_type_by_name ("GError");
	default:
		llvm::errs () << "Warning: Unexpected base info type " <<
			g_base_info_get_type (type_info) <<
			" for base info " <<
			g_base_info_get_name (type_info) << ".\n";
		return QualType ();
	}
}

/* Returns true iff
 *  • @a is a GObject, @b is a GObject, and @a is equal to or a subclass of @b;
 *  • @a is a GInterface, @b is a GInterface, and @a is equal to @b;
 *  • @a is a GObject, @b is a GInterface, and @a or one of its superclasses
 *    implements @b.
 */
static bool
_is_gtype_subclass (GIBaseInfo *a, GIBaseInfo *b)
{
	assert (g_base_info_get_type (a) == GI_INFO_TYPE_OBJECT ||
	        g_base_info_get_type (a) == GI_INFO_TYPE_INTERFACE);
	assert (g_base_info_get_type (b) == GI_INFO_TYPE_OBJECT ||
	        g_base_info_get_type (b) == GI_INFO_TYPE_INTERFACE);

	/* The case where @a and @b are equal. */
	if (g_base_info_equal (a, b)) {
		return true;
	}

	/* The case where @a implements @b. */
	if (g_base_info_get_type (b) == GI_INFO_TYPE_INTERFACE) {
		for (gint i = 0; i < g_object_info_get_n_interfaces (a); i++) {
			GIInterfaceInfo *iface;

			iface = g_object_info_get_interface (a, i);
			bool eq = g_base_info_equal (iface, b);
			g_base_info_unref (iface);

			if (eq) {
				return true;
			}
		}
	}

	/* The case where @a is a subclass of @b, or a subclass of a class which
	 * implements @b. */
	GIObjectInfo *ap = g_object_info_get_parent ((GIObjectInfo *) a);

	if (ap == NULL) {
		return false;
	}

	bool retval = _is_gtype_subclass (ap, b);
	g_base_info_unref (ap);

	return retval;
}

/* A safe calling convention is any convention which is caller-cleanup and where
 * the callee can access its actual parameters left-to-right without calculating
 * offsets (e.g. if the actual parameters are pushed on to the stack in
 * right-to-left order by the caller, leaving the left-most actual parameter the
 * first one to be popped off by the callee). This allows us to safely pass
 * actual parameters in excess of the number of formal parameters expected by
 * the function, without risking corrupting the stack.
 *
 * An unsafe calling convention is any other convention.
 *
 * Known safe calling conventions:
 *  - x86[1]:
 *   • cdecl: Caller clean-up, parameters pushed right-to-left.
 *   • syscall: Same.
 *   • optlink: Same.
 *   • GCC thiscall: Same.
 *   • Microsoft x64: Caller clean-up (of parameters, not stack frame),
 *     parameters pushed right-to-left[10].
 *   • System V AMD64 ABI: Same[11, §3.2.3].
 *  - PowerPC (32-bit): Caller clean-up (of parameters, not stack frame),
 *    parameters pushed right-to-left[2].
 *  - PowerPC (64-bit): Same[3].
 *  - ARM: Caller clean-up, parameters pushed right-to-left[4, §5.3].
 *  - MIPS:
 *   • O32: Caller clean-up (of parameters, not stack frame), parameters pushed
 *     right-to-left[6].
 *   • O64: Same[8].
 *   • N32: Unclear, but variadic arguments are supported[7].
 *   • N64: Same[7].
 *   • EABI32: Caller clean-up, parameters pushed right-to-left[5].
 *   • EABI64: Same[5].
 *  - SPARC: Callee clean-up, but the caller’s stack pointer value is saved in
 *    its register window, so the callee doesn’t need to know the stack frame
 *    size to restore the old stack pointer; parameters pushed right-to-left[9].
 *  - PNaCl: Equivalent to cdecl[12].
 *
 * Known unsafe calling conventions:
 *  - x86[1]:
 *   • pascal: Callee clean-up, parameters pushed left-to-right.
 *   • stdcall: Callee clean-up, parameters pushed right-to-left.
 *     Note: This is used for Windows API calls, so we cannot use Windows API
 *     for callbacks with swapped parameters.
 *   • register/Borland fastcall: Same problems as pascal.
 *   • Microsoft/GCC fastcall: Same problems as stdcall.
 *   • Watcom fastcall: Same.
 *   • safecall: Same.
 *   • Microsoft thiscall: Same.
 *     Note: This is used for C++ non-static member function calls (when using
 *     MSVC++), so we cannot use C++ methods for callbacks with swapped
 *     parameters.
 *
 * References:
 *  [1]: http://en.wikipedia.org/wiki/X86_calling_conventions
 *  [2]: https://developer.apple.com/library/mac/documentation/
 *       DeveloperTools/Conceptual/LowLevelABI/
 *       100-32-bit_PowerPC_Function_Calling_Conventions/
 *       32bitPowerPC.html
 *  [3]: https://developer.apple.com/library/mac/documentation/
 *       DeveloperTools/Conceptual/LowLevelABI/
 *       110-64-bit_PowerPC_Function_Calling_Conventions/
 *       64bitPowerPC.html
 *  [4]: http://infocenter.arm.com/help/topic/com.arm.doc.ihi0042e/
 *       IHI0042E_aapcs.pdf
 *  [5]: http://www.cygwin.com/ml/binutils/2003-06/msg00436.html
 *  [6]: http://web.archive.org/web/20040930224745/http://
 *       www.caldera.com/developers/devspecs/mipsabi.pdf
 *  [7]: http://techpubs.sgi.com/library/manuals/2000/007-2816-005/pdf/
 *       007-2816-005.pdf
 *  [8]: https://gcc.gnu.org/projects/mipso64-abi.html
 *  [9]: http://math-atlas.sourceforge.net/devel/assembly/abi_sysV_sparc.pdf
 *  [10]: http://msdn.microsoft.com/en-us/library/ms235286.aspx
 *  [11]: http://x86-64.org/documentation/abi.pdf
 *  [12]: https://developer.chrome.com/native-client/reference/
 *        pnacl-bitcode-abi#calling-conventions
 */
static bool
calling_convention_is_safe (CallingConv conv)
{
	switch (conv) {
	case CC_C:  /* cdecl */
	case CC_X86_64Win64:  /* x86-64 */
	case CC_X86_64SysV:  /* x86-64 */
	case CC_AAPCS:  /* ARM */
	case CC_AAPCS_VFP:  /* ARM with VFP registers */
	case CC_PnaclCall:  /* Chromium PNC — equivalent to cdecl */
		return true;
	case CC_X86StdCall:
	case CC_X86FastCall:
	case CC_X86ThisCall:
	case CC_X86Pascal:
		return false;
	case CC_IntelOclBicc:
		/* Intel OpenCL Built-Ins. I can’t find any documentation about
		 * this, so let’s consider it unsafe. */
	default:
		return false;
	}
}

/* Check the type of the callback in @expr (which is assumed to be a function
 * pointer or cast of a function pointer), asserting that it matches the type
 * of @signal_info.
 *
 * @dynamic_instance_info is information about the GObject subclass being passed
 * to g_signal_connect(). @static_instance_info is information about the GObject
 * subclass which the signal is defined on. @dynamic_instance_info should be a
 * (non-strict) subclass of @static_instance_info.
 *
 * @data_type is the qualified type of the user data parameter passed to
 * g_signal_connect(). It is checked against the first parameter of the callback
 * type iff %G_CONNECT_SWAPPED was specified at connection time (iff @is_swapped
 * is true).
 *
 * Returns true if the callback has the correct type. Emits an error and returns
 * false otherwise. */
static bool
_check_signal_callback_type (const Expr *expr,
                             GIBaseInfo *dynamic_instance_info,
                             GIBaseInfo *static_instance_info,
                             const QualType data_type,
                             bool is_swapped,
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
			<< gir_manager.get_c_name_for_type (static_instance_info)
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
		                                    dynamic_instance_info,
		                                    static_instance_info,
		                                    data_type, is_swapped,
		                                    signal_info, compiler,
		                                    context, gir_manager,
		                                    type_manager);
	}
	case Stmt::StmtClass::ImplicitCastExprClass:
	case Stmt::StmtClass::CStyleCastExprClass: {
		/* A cast (explicit or C-style). */
		const CastExpr *cast_expr = cast<CastExpr> (expr);

		return _check_signal_callback_type (cast_expr->getSubExprAsWritten (),
		                                    dynamic_instance_info,
		                                    static_instance_info,
		                                    data_type, is_swapped,
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
	 * because GIR omits the ‘self’ and ‘user_data’ arguments.
	 *
	 * Should we allow the callback type to have fewer parameters than the
	 * signal prototype specifies? This is a complex one. It is undefined
	 * behaviour by the C standard (§6.5.2.2¶6), but is defined behaviour in
	 * most common calling conventions, so we can define ‘safe’ and ‘unsafe’
	 * calling conventions for passing actual parameters in excess of a
	 * function’s number of formal parameters.
	 *
	 * In the interest of being practical, only emit an error on an
	 * actual–formal parameter count mismatch if the user specifies a
	 * callback with an unsafe calling convention. If the user specifies a
	 * callback with a safe calling convention, emit a portability warning
	 * instead, which should be low-importance and disableable.
	 *
	 * See the documentation for calling_convention_is_safe() for an
	 * analysis.
	 */
	GICallableInfo *callable_info = signal_info;
	guint n_signal_args = g_callable_info_get_n_args (callable_info) + 2;
	guint n_callback_args = callback_type->getNumArgs ();

	GITypeInfo expected_type_info;
	QualType actual_type, expected_type;

	if ((!calling_convention_is_safe (callback_type->getCallConv ()) &&
	     n_signal_args != n_callback_args) ||
	    n_signal_args < n_callback_args) {
		/* Error. */

		/* TODO: Emit expected type of signal callback? */
		Debug::emit_error ("Incorrect number of arguments in signal "
		                   "handler for signal ‘%0::%1’. Expected %2 "
		                   "but saw %3.",
		                   compiler, expr->getLocStart ())
		<< gir_manager.get_c_name_for_type (static_instance_info)
		<< g_base_info_get_name (signal_info)
		<< n_signal_args
		<< n_callback_args
		<< decl_range;

		return false;
	}

	/* Check all arguments */
	for (guint i = 0; i < n_callback_args; i++) {
		const gchar *arg_name;
		bool type_error;

		actual_type = callback_type->getArgType (i);

		if ((i == 0 && !is_swapped) ||
		    (i == n_signal_args - 1 && is_swapped)) {
			/* First argument is always a pointer to the GObject
			 * instance which the signal is defined on; unless the
			 * %G_CONNECT_SWAPPED flag has been passed, in which
			 * case it’s the user_data, which is handled in the
			 * ‘else’ block below. */
			std::string c_type (gir_manager.get_c_name_for_type (static_instance_info));
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
			       g_base_info_get_name (dynamic_instance_info) <<
			       "’ <: ‘" <<
			       g_base_info_get_name (actual_type_info) <<
			       "’ <: ‘" <<
			       g_base_info_get_name (static_instance_info) <<
			       "’.");

			/* See the documentation at the top of the file for an
			 * explanation of the (non-trivial) GObject type
			 * checking for the first parameter. */
			type_error = (actual_type_info == NULL ||
			              atp.isConstQualified () ||
			              !_is_gtype_subclass (dynamic_instance_info,
			                                   actual_type_info) ||
			              !_is_gtype_subclass (actual_type_info,
			                                   static_instance_info));

			g_base_info_unref (actual_type_info);
		} else if ((i == n_signal_args - 1 && !is_swapped) ||
		           (i == 0 && is_swapped)) {
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
			 * eliminates a huge number of false positives.
			 *
			 * FIXME: In future, we might want to compare that the
			 * @data_type (the type of the @user_data expression
			 * passed to g_signal_connect()) is compatible with
			 * @actual_type (the type of the formal @user_data
			 * parameter of the callback). However, I think that can
			 * be better implemented as a separate checker which
			 * checks that closure parameters type check with the
			 * callbacks they are used with, i.e. so it will also
			 * check things like g_list_find_custom(). */
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
				<< gir_manager.get_c_name_for_type (static_instance_info)
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
		if (type_error && is_swapped) {
			/* Error. */

			/* TODO: Emit expected type of signal callback? */
			Debug::emit_error ("Incorrect type for argument ‘%0’ "
			                   "in swapped signal handler for "
			                   "signal ‘%1::%2’. Expected ‘%3’ but "
			                   "saw ‘%4’.", compiler,
			                   expr->getLocStart ())
			<< arg_name
			<< gir_manager.get_c_name_for_type (static_instance_info)
			<< g_base_info_get_name (signal_info)
			<< expected_type.getAsString ()
			<< actual_type.getAsString ()
			<< decl_range;

			return false;
		} else if (type_error) {
			/* Error. */

			/* TODO: Emit expected type of signal callback? */
			Debug::emit_error ("Incorrect type for argument ‘%0’ "
			                   "in signal handler for signal "
			                   "‘%1::%2’. Expected ‘%3’ but saw "
			                   "‘%4’.", compiler,
			                   expr->getLocStart ())
			<< arg_name
			<< gir_manager.get_c_name_for_type (static_instance_info)
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
		<< gir_manager.get_c_name_for_type (static_instance_info)
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
		<< gir_manager.get_c_name_for_type (static_instance_info)
		<< g_base_info_get_name (signal_info)
		<< expected_type.getAsString ()
		<< actual_type.getAsString ()
		<< decl_range;

		return false;
	}

	return true;
}

static bool
_signal_flags_is_swapped (const Expr *flags_expr,
                          const std::string &signal_name,
                          CompilerInstance &compiler,
                          const ASTContext &context)
{
	switch (flags_expr->getStmtClass ()) {
	case Stmt::StmtClass::DeclRefExprClass: {
		/* A reference to an enum, presumably. */
		const DeclRefExpr *decl_ref_expr = cast<DeclRefExpr> (flags_expr);
		const ValueDecl *value_decl = decl_ref_expr->getDecl ();
		const EnumConstantDecl *enum_decl = dyn_cast<EnumConstantDecl> (value_decl);

		if (enum_decl == NULL) {
			/* Error. */
			WARN_EXPR (__func__ << "() can’t handle values "
			           "of type ‘" <<
			           value_decl->getType ().getAsString () <<
			           "’.",
			           *flags_expr);
			return false;
		}

		return (enum_decl->getNameAsString () == "G_CONNECT_SWAPPED");
	}
	case Stmt::StmtClass::IntegerLiteralClass: {
		const IntegerLiteral *literal_expr = cast<IntegerLiteral> (flags_expr);
		const llvm::APInt i = literal_expr->getValue ();

		/* FIXME: Ugly as sin. */
		return ((i.getLimitedValue ((1 << 8) - 1) & G_CONNECT_SWAPPED) != 0);
	}
	case Stmt::StmtClass::BinaryOperatorClass: {
		/* A binary operation, probably bitwise OR. */
		const BinaryOperator *op_expr = cast<BinaryOperator> (flags_expr);

		if (op_expr->getOpcode () != BO_Or) {
			/* Error. */
			WARN_EXPR (__func__ << "() can’t handle binary "
			           "operators other than bitwise OR.",
			           *flags_expr);
			return false;
		}

		bool lhs_is_swapped = _signal_flags_is_swapped (op_expr->getLHS ()->IgnoreParenImpCasts (),
		                                                signal_name,
		                                                compiler,
		                                                context);
		bool rhs_is_swapped = _signal_flags_is_swapped (op_expr->getRHS ()->IgnoreParenImpCasts (),
		                                                signal_name,
		                                                compiler,
		                                                context);

		return lhs_is_swapped || rhs_is_swapped;
	}
	case Stmt::StmtClass::ParenExprClass: {
		/* A parenthesised expression. */
		const ParenExpr *paren_expr = cast<ParenExpr> (flags_expr);

		return _signal_flags_is_swapped (paren_expr->getSubExpr (),
		                                 signal_name, compiler,
		                                 context);
	}
	case Stmt::StmtClass::ImplicitCastExprClass:
	case Stmt::StmtClass::CStyleCastExprClass: {
		/* A cast (explicit or C-style). */
		const CastExpr *cast_expr = cast<CastExpr> (flags_expr);

		return _signal_flags_is_swapped (cast_expr->getSubExprAsWritten (),
		                                 signal_name, compiler,
		                                 context);
	}
	case Stmt::StmtClass::NoStmtClass:
	default:
		WARN_EXPR (__func__ << "() can’t handle expressions of type " <<
		           flags_expr->getStmtClassName (), *flags_expr);
		return false;
	}
}

/* Parse the signal name out of a user-provided string. It could be in the
 * format:
 *  • signal-name
 *  • signal_name
 *  • signal-name::some-detail
 * We need to return the signal name with hyphens. */
static std::string
_parse_signal_name (const std::string &in)
{
	std::string::size_type d = in.find ("::");
	std::string signal_name;

	if (d != std::string::npos) {
		/* Strip off the detail string.
		 *
		 * FIXME: In future we could validate this. e.g. For the
		 * ‘notify’ signal, validate it against the object’s
		 * properties. */
		signal_name = in.substr (0, d);
	} else {
		signal_name = in;
	}

	/* Normalise the string. */
	std::replace (signal_name.begin (), signal_name.end (), '_', '-');

	assert (!signal_name.empty ());

	return signal_name;
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
	const Expr *user_data_arg;
	const Expr *flags_arg = NULL;

	callback_arg = call.getArg (func_info->callback_param_index);
	gobject_arg = call.getArg (func_info->gobject_param_index);
	signal_name_arg = call.getArg (func_info->signal_name_param_index);
	user_data_arg = call.getArg (func_info->user_data_param_index);

	if (func_info->flags_param_index >= 0) {
		flags_arg = call.getArg (func_info->flags_param_index);
	}

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
	std::string signal_name = _parse_signal_name (signal_name_str_ref.str ());

	DEBUG ("Using signal name ‘" << signal_name << "’.");

	/* Work out whether the instance and data have been swapped for extra
	 * complication funtimes. */
	bool is_swapped = false;

	if (flags_arg != NULL) {
		is_swapped = _signal_flags_is_swapped (flags_arg->IgnoreParenImpCasts (),
		                                       signal_name, compiler,
		                                       context);
	}

	/* Try and grab the GObject parameter’s type. This is the type of the
	 * variable passed into g_signal_connect(). The @static_instance_info is
	 * the type of the GObject subclass which defines the signal. */
	GIObjectInfo *dynamic_instance_info, *static_instance_info = NULL;

	dynamic_instance_info = _expr_to_gtype (gobject_arg->IgnoreParenImpCasts (),
	                                        context, gir_manager);
	if (dynamic_instance_info == NULL) {
		/* Emit a remark rather than a warning because the user may not
		 * easily be able to add a GIR file containing the signal
		 * information.. */
		Debug::emit_remark ("Could not find GObject subclass for "
		                    "expression when connecting to signal "
		                    "‘%0’. To improve static analysis, add a "
		                    "typecast to the GObject parameter of "
		                    "%1() to the specific class defining the "
		                    "signal. Ensure a GIR file defining that "
		                    "class is loaded.", compiler,
		                    call.getLocStart ())
		<< signal_name
		<< func_info->func_name
		<< gobject_arg->getSourceRange ()
		<< signal_name_arg->getSourceRange ();

		return false;
	}

	DEBUG ("Using GIObjectInfo ‘" <<
	       g_base_info_get_name ((GIBaseInfo *) dynamic_instance_info) <<
	       "’ from namespace ‘" <<
	       g_base_info_get_namespace ((GIBaseInfo *) dynamic_instance_info) <<
	       "’.");

	/* Find the signal in the GObject. */
	GISignalInfo *signal_info;

	signal_info = _gtype_look_up_signal (dynamic_instance_info,
	                                     &static_instance_info,
	                                     signal_name.c_str ());
	if (signal_info == NULL) {
		/* Remark on the fact the signal information cannot be found.
		 * We can’t really make this a warning, since the user may not
		 * be able to easily add a GIR file containing the signal
		 * information. */
		Debug::emit_remark ("No signal named ‘%0’ in GObject class "
		                    "‘%1’. To improve static analysis, add a "
		                    "typecast to the GObject parameter of "
		                    "%2() to the specific class defining the "
		                    "signal. Ensure a GIR file defining that "
		                    "class is loaded.", compiler, call.getLocStart ())
		<< signal_name
		<< gir_manager.get_c_name_for_type (dynamic_instance_info)
		<< func_info->func_name
		<< gobject_arg->getSourceRange ()
		<< signal_name_arg->getSourceRange ();

		g_base_info_unref (dynamic_instance_info);

		return false;
	}

	DEBUG ("Using GISignalInfo ‘" <<
	       g_base_info_get_name ((GIBaseInfo *) signal_info) <<
	       "’ from namespace ‘" <<
	       g_base_info_get_namespace ((GIBaseInfo *) signal_info) <<
	       "’.");

	/* Check the callback’s type. */
	if (!_check_signal_callback_type (callback_arg->IgnoreParenImpCasts (),
	                                  dynamic_instance_info,
	                                  static_instance_info,
	                                  user_data_arg->getType (), is_swapped,
	                                  signal_info,
	                                  compiler, context, gir_manager,
	                                  type_manager)) {
		/* A diagnostic has already been emitted by
		 * _check_signal_callback_type(). */
		g_base_info_unref (signal_info);
		g_base_info_unref (dynamic_instance_info);
		g_base_info_unref (static_instance_info);

		return false;
	}

	g_base_info_unref (signal_info);
	g_base_info_unref (dynamic_instance_info);
	g_base_info_unref (static_instance_info);

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
