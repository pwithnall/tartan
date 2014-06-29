/* Template: gvariant */

/*
 * No error
 */
{
	gchar *str1 = NULL, *str2 = NULL;
	g_variant_get (existing_variant, "(ss)", &str1, &str2);
}

/*
 * No error
 */
{
	gchar *str1 = NULL, *str2 = NULL;
	g_variant_get (existing_variant, "(ss)", NULL, NULL);
}

/*
 * No error
 */
{
	const gchar *str1 = NULL, *str2 = NULL;
	g_variant_get (existing_variant, "(&s&s)", &str1, &str2);
}

/*
 *  Expected a GVariant variadic argument of type 'const char **' but saw one of type 'gchar **' (aka 'char **').
 *         g_variant_get (existing_variant, "(&s&s)", &str1, &str2);
 *                                                    ^
 */
{
	// Outbound pointers must be const if ‘&’ is used.
	gchar *str1 = NULL, *str2 = NULL;
	g_variant_get (existing_variant, "(&s&s)", &str1, &str2);
}

/*
 * Expected a GVariant variadic argument of type 'gint32 *' (aka 'int *') but there wasn’t one.
 *         g_variant_get (existing_variant, "invalid");
 *                        ^
 */
{
	g_variant_get (existing_variant, "invalid");
}

/*
 * No error
 */
{
	gchar **some_str_array;
	g_variant_get (existing_variant, "^as", &some_str_array);
}

/*
 * Expected a GVariant variadic argument of type 'char ***' but saw one of type 'const gchar ***' (aka 'const char ***').
 *         g_variant_get (existing_variant, "^as", &some_str_array);
 *                                                 ^
 */
{
	const gchar **some_str_array;
	g_variant_get (existing_variant, "^as", &some_str_array);
}

/*
 * No error
 */
{
	const gchar **some_str_array;
	g_variant_get (existing_variant, "^a&s", &some_str_array);
}

/*
 *  Expected a GVariant variadic argument of type 'const char ***' but saw one of type 'gchar ***' (aka 'char ***').
 *         g_variant_get (existing_variant, "^a&s", &some_str_array);
 *                                                  ^
 */
{
	gchar **some_str_array;
	g_variant_get (existing_variant, "^a&s", &some_str_array);
}

/*
 * No error
 */
{
	gchar **some_str_array;
	g_variant_get (existing_variant, "^ao", &some_str_array);
}

/*
 * No error
 */
{
	const gchar **some_str_array;
	g_variant_get (existing_variant, "^a&o", &some_str_array);
}

/*
 * No error
 */
{
	gchar *some_byte_str;
	g_variant_get (existing_variant, "^ay", &some_byte_str);
}

/*
 * Expected a GVariant variadic argument of type 'char **' but saw one of type 'const gchar **' (aka 'const char **').
 *         g_variant_get (existing_variant, "^ay", &some_byte_str);
 *                                                 ^
 */
{
	const gchar *some_byte_str;
	g_variant_get (existing_variant, "^ay", &some_byte_str);
}

/*
 * No error
 */
{
	const gchar *some_byte_str;
	g_variant_get (existing_variant, "^&ay", &some_byte_str);
}

/*
 * Expected a GVariant variadic argument of type 'const char **' but saw one of type 'gchar **' (aka 'char **').
 *         g_variant_get (existing_variant, "^&ay", &some_byte_str);
 *                                                  ^
 */
{
	gchar *some_byte_str;
	g_variant_get (existing_variant, "^&ay", &some_byte_str);
}

/*
 * No error
 */
{
	gchar **some_bytes_array;
	g_variant_get (existing_variant, "^aay", &some_bytes_array);
}

/*
 * Expected a GVariant variadic argument of type 'char ***' but saw one of type 'const gchar ***' (aka 'const char ***').
 *         g_variant_get (existing_variant, "^aay", &some_bytes_array);
 *                                                  ^
 */
{
	const gchar **some_bytes_array;
	g_variant_get (existing_variant, "^aay", &some_bytes_array);
}

/*
 * No error
 */
{
	const gchar **some_bytes_array;
	g_variant_get (existing_variant, "^a&ay", &some_bytes_array);
}

/*
 * Expected a GVariant variadic argument of type 'const char ***' but saw one of type 'gchar ***' (aka 'char ***').
 *         g_variant_get (existing_variant, "^a&ay", &some_bytes_array);
 *                                                   ^
 */
{
	gchar **some_bytes_array;
	g_variant_get (existing_variant, "^a&ay", &some_bytes_array);
}

/*
 * No error
 */
{
	g_variant_get (existing_variant, "@s", NULL);
}

/*
 * Expected a GVariant variadic argument of type 'GVariant **' (aka 'struct _GVariant **') but saw one of type 'gchar **' (aka 'char **').
 *         g_variant_get (existing_variant, "@s", &some_str);
 *                                                ^
 */
{
	gchar *some_str;
	g_variant_get (existing_variant, "@s", &some_str);
}

/*
 * No error
 */
{
	GVariant *some_variant;
	g_variant_get (existing_variant, "@s", &some_variant);
}

/*
 *  Expected a GVariant variadic argument of type 'GVariant **' (aka 'struct _GVariant **') but saw one of type 'const GVariant **' (aka 'const struct _GVariant **').
 *         g_variant_get (existing_variant, "@s", &some_variant);
 *                                                ^
 */
{
	const GVariant *some_variant;
	g_variant_get (existing_variant, "@s", &some_variant);
}

/*
 * No error
 */
{
	GVariant *some_variant1, *some_variant2;
	gchar *some_str;
	g_variant_get (existing_variant, "(@s@ss)", &some_variant1, &some_variant2, &some_str);
}


/*
 * Unexpected GVariant format strings ‘i’ with unpaired arguments. If using multiple format strings, they should be enclosed in brackets to create a tuple (e.g. ‘(si)’).
 *         g_variant_get (existing_variant, "si", &some_str, &some_int);
 *                                          ^
 */
{
	// Missing tuple brackets
	gchar *some_str;
	gint32 some_int;
	g_variant_get (existing_variant, "si", &some_str, &some_int);
}

/*
 * No error
 */
{
	// Simple tuple
	gchar *some_str;
	gint32 some_int;
	g_variant_get (existing_variant, "(si)", &some_str, &some_int);
}

/*
 * No error
 */
{
	// Nested tuples
	gchar *str1, *str2;
	gint32 int1;
	g_variant_get (existing_variant, "((s)(si))", &str1, &str2, &int1);
}

/*
 * No error
 */
{
	// Empty tuple
	g_variant_get (existing_variant, "()");
}

/*
 * Invalid GVariant format string: tuple did not end with ‘)’.
 *         g_variant_get (existing_variant, "(si", &str1, &int1);
 *                                          ^
 */
{
	// Unbalanced brackets
	gchar *str1;
	gint32 int1;
	g_variant_get (existing_variant, "(si", &str1, &int1);
}

/*
 * No error
 */
{
	GVariant *some_var;
	g_variant_get (existing_variant, "v", &some_var);
}

/*
 * Expected a GVariant variadic argument of type 'GVariant **' (aka 'struct _GVariant **') but saw one of type 'const GVariant **' (aka 'const struct _GVariant **').
 *         g_variant_get (existing_variant, "v", &some_var);
 *                                               ^
 */
{
	const GVariant *some_var;
	g_variant_get (existing_variant, "v", &some_var);
}

/*
 * No error
 */
{
	g_variant_get (existing_variant, "v", NULL);
}

/*
 * Expected a GVariant variadic argument of type 'GVariant **' (aka 'struct _GVariant **') but saw one of type 'char *'.
 *         g_variant_get (existing_variant, "v", "nope");
 *                                               ^
 */
{
	g_variant_get (existing_variant, "v", "nope");
}

/*
 * No error
 */
{
	gchar *some_str;
	g_variant_get (existing_variant, "ms", &some_str);
}

/*
 * No error
 */
{
	g_variant_get (existing_variant, "ms", NULL);
}

/*
 * No error
 */
{
	g_variant_get (existing_variant, "m&s", NULL);
}

/*
 * No error
 */
{
	const gchar *some_str;
	g_variant_get (existing_variant, "m&s", &some_str);
}

/*
 * Expected a GVariant variadic argument of type 'const char **' but saw one of type 'gchar **' (aka 'char **').
 *         g_variant_get (existing_variant, "m&s", &some_str);
 *                                                 ^
 */
{
	gchar *some_str;
	g_variant_get (existing_variant, "m&s", &some_str);
}


/*
 * No error
 */
{
	GVariantIter *some_iter;
	g_variant_get (existing_variant, "as", &some_iter);
	g_variant_iter_free (some_iter);
}

/*
 * Expected a GVariant variadic argument of type 'GVariantIter **' (aka 'struct _GVariantIter **') but saw one of type 'GVariantIter *' (aka 'struct _GVariantIter *').
 *         g_variant_get (existing_variant, "as", some_iter);
 *                                                ^
 */
{
	// Incorrectly don’t pass @some_iter by reference.
	GVariantIter *some_iter;
	g_variant_get (existing_variant, "as", some_iter);
	g_variant_iter_free (some_iter);
}

/*
 * No error
 */
{
	g_variant_get (existing_variant, "as", NULL);
}

/*
 * No error
 */
{
	g_variant_get (existing_variant, "a?", NULL);
}

/*
 * No error
 */
{
	GVariantIter *some_iter;
	g_variant_get (existing_variant, "a(sss)", &some_iter);
	g_variant_iter_free (some_iter);
}

/*
 * No error
 */
{
	GVariantIter *some_iter;
	g_variant_get (existing_variant, "a{?*}", &some_iter);
	g_variant_iter_free (some_iter);
}

/*
 * Expected a GVariant variadic argument of type 'GVariantIter **' (aka 'struct _GVariantIter **') but saw one of type 'char *'.
 *         g_variant_get (existing_variant, "au", "nope");
 *                                                ^
 */
{
	g_variant_get (existing_variant, "au", "nope");
}

/*
 * No error
 */
{
	gboolean some_bool;
	g_variant_get (existing_variant, "b", &some_bool);
}

/*
 *  Expected a GVariant variadic argument of type 'int *' but saw one of type 'gchar *' (aka 'char *').
 *         g_variant_get (existing_variant, "b", &some_not_bool);
 *                                               ^
 */
{
	gchar some_not_bool;
	g_variant_get (existing_variant, "b", &some_not_bool);
}

/*
 * No error
 */
{
	guchar some_uchar;
	g_variant_get (existing_variant, "y", &some_uchar);
}

/*
 * Expected a GVariant variadic argument of type 'unsigned char *' but saw one of type 'guint *' (aka 'unsigned int *').
 *         g_variant_get (existing_variant, "y", &some_uchar);
 *                                               ^
 */
{
	// Type promotion doesn’t occur with g_variant_get().
	guint some_uchar;
	g_variant_get (existing_variant, "y", &some_uchar);
}

/*
 * No error
 */
{
	guint some_uchar;
	g_variant_get (existing_variant, "y", (guchar *) &some_uchar);
}

/*
 * No error
 */
{
	gdouble some_double;
	g_variant_get (existing_variant, "d", &some_double);
}

/*
 * Expected a GVariant variadic argument of type 'double *' but saw one of type 'gfloat *' (aka 'float *').
 *         g_variant_get (existing_variant, "d", &some_float);
 *                                               ^
 */
{
	gfloat some_float;
	g_variant_get (existing_variant, "d", &some_float);
}

/*
 * Expected a GVariant variadic argument of type 'gint64 *' (aka 'long *') but saw one of type 'gint32 *' (aka 'int *').
 *         g_variant_get (existing_variant, "x", &some_int);
 *                                               ^
 */
{
	gint32 some_int;
	g_variant_get (existing_variant, "x", &some_int);
}

/*
 * No error
 */
{
	gint64 some_int;
	g_variant_get (existing_variant, "x", &some_int);
}

/*
 * Expected a GVariant variadic argument of type 'gint64 *' (aka 'long *') but saw one of type 'guint64 *' (aka 'unsigned long *').
 *         g_variant_get (existing_variant, "x", &some_uint);
 *                                               ^
 */
{
	guint64 some_uint;
	g_variant_get (existing_variant, "x", &some_uint);
}

/*
 * No error
 */
{
	va_list some_va_list;
	g_variant_get_va (existing_variant, "(sss)", NULL, &some_va_list);
}

/*
 * Unexpected GVariant format strings ‘nvalid’ with unpaired arguments. If using multiple format strings, they should be enclosed in brackets to create a tuple (e.g. ‘(invalid)’).
 *         g_variant_get_va (existing_variant, "invalid", NULL, &some_va_list);
 *                                             ^
 */
{
	va_list some_va_list;
	g_variant_get_va (existing_variant, "invalid", NULL, &some_va_list);
}
