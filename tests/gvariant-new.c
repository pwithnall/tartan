/* Template: gvariant */

/*
 * Expected a GVariant variadic argument of type ‘char *’ but saw one of type ‘guint’.
 *         floating_variant = g_variant_new ("(sss)", "hello", my_string, a_little_int_short_and_stout);
 *                                               ^                        ^
 */
{
	const gchar *my_string = "there";
	guint a_little_int_short_and_stout = 5;

	floating_variant = g_variant_new ("(sss)", "hello", my_string, a_little_int_short_and_stout);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("(ss)", "hi", "there");
}

/*
 * Expected a GVariant variadic argument of type ‘int’ but there wasn’t one.
 *         floating_variant = g_variant_new ("invalid");
 *                                            ^
 */
{
	floating_variant = g_variant_new ("invalid");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("x", (gint64) 56);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("b", TRUE);
}

/*
 * Expected a GVariant variadic argument of type ‘int’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("b", "nope");
 */
{
	floating_variant = g_variant_new ("b", "nope");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("y", (guchar) 'h');
}

/*
 * Expected a GVariant variadic argument of type ‘unsigned char’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("y", "nope");
 *                                            ^   ^
 */
{
	floating_variant = g_variant_new ("y", "nope");
}

/*
 * No error
 */
{
	gint16 some_var = 5;
	floating_variant = g_variant_new ("n", some_var);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("n", (gint16) -6);
}

/*
 * Expected a GVariant variadic argument of type ‘short’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("n", "nope");
 *                                                ^
 */
{
	floating_variant = g_variant_new ("n", "nope");
}

/*
 * No error
 */
{
	guint16 some_var = 5;
	floating_variant = g_variant_new ("q", some_var);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("q", (guint16) 6);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("q", 6);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("q", (gint32) 6);
}

/*
 * No error
 */
{
	// This is a subtle one, but we deliberately don’t currently warn about
	// it. ‘q’ will read a 32-bit signed integer, which -6 fits into.
	floating_variant = g_variant_new ("q", -6);
}

/*
 * Expected a GVariant variadic argument of type ‘int’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("i", "nope");
 *                                                ^
 */
{
	floating_variant = g_variant_new ("i", "nope");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("i", -16);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("i", 16);
}

/*
 * No error
 */
{
	gint32 some_var = -5;
	floating_variant = g_variant_new ("i", some_var);
}

/*
 * Expected a GVariant variadic argument of type ‘unsigned int’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("u", "nope");
 *                                                ^
 */
{
	floating_variant = g_variant_new ("u", "nope");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("u", G_MAXUINT);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("u", 16);
}

/*
 * No error
 */
{
	guint32 some_var = 5;
	floating_variant = g_variant_new ("u", some_var);
}

/*
 * No error
 */
{
	// This could be interpreted as a NULL pointer constant.
	floating_variant = g_variant_new ("u", 0);
}

/*
 *  Expected a GVariant variadic argument of type ‘long’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("x", "nope");
 *                                                ^
 */
{
	floating_variant = g_variant_new ("x", "nope");
}

/*
 * Expected a GVariant variadic argument of type ‘long’ but saw one of type ‘int’.
 *         floating_variant = g_variant_new ("x", -5);
 *                                                ^
 */
{
	floating_variant = g_variant_new ("x", -5);
}

/*
 * Expected a GVariant variadic argument of type ‘long’ but saw one of type ‘gint32’.
 *         floating_variant = g_variant_new ("x", some_var);
 *                                                ^
 */
{
	gint32 some_var = -5;  // deliberately not wide enough
	floating_variant = g_variant_new ("x", some_var);
}

/*
 * No error
 */
{
	gint64 some_var = -5;
	floating_variant = g_variant_new ("x", some_var);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("x", (gint64) 500);
}

/*
 *  Expected a GVariant variadic argument of type ‘signed long’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("t", "nada");
 *                                                ^
 */
{
	floating_variant = g_variant_new ("t", "nada");
}

/*
 * Expected a GVariant variadic argument of type ‘unsigned long’ but saw one of type ‘int’.
 *         floating_variant = g_variant_new ("t", 5);
 *                                                ^
 */
{
	floating_variant = g_variant_new ("t", 5);
}

/*
 * Expected a GVariant variadic argument of type ‘unsigned long’ but saw one of type ‘guint32’.
 *         floating_variant = g_variant_new ("t", some_var);
 *                                                ^
 */
{
	guint32 some_var = 5;  // deliberately not wide enough
	floating_variant = g_variant_new ("t", some_var);
}

/*
 * No error
 */
{
	guint64 some_var = 5;
	floating_variant = g_variant_new ("t", some_var);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("t", (guint64) 500);
}

/*
 * Expected a GVariant variadic argument of type ‘int’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("h", "nope");
 *                                                ^
 */
{
	floating_variant = g_variant_new ("h", "nope");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("h", -16);
}

/*
 * No error
 */
{
	gint32 some_var = -5;
	floating_variant = g_variant_new ("h", some_var);
}

/*
 * Expected a GVariant variadic argument of type ‘double’ but saw one of type ‘int’.
 *         floating_variant = g_variant_new ("d", 5);
 *                                                ^
 */
{
	floating_variant = g_variant_new ("d", 5);
}

/*
 * No error
 */
{
	gfloat some_var = 5.1;
	floating_variant = g_variant_new ("d", some_var);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("d", 51.07);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("d", (gdouble) 7);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("s", "some string");
}

/*
 * Expected a GVariant variadic argument of type ‘char *’ but saw one of type ‘int’.
 *         floating_variant = g_variant_new ("s", 152);
 *                                                ^
 */
{
	floating_variant = g_variant_new ("s", 152);
}

/*
 * No error
 */
{
	const gchar *some_var = "hello world";
	floating_variant = g_variant_new ("s", some_var);
}

/*
 * No error
 */
{
	const char *some_var = "hello world";  // try plain C types
	floating_variant = g_variant_new ("s", some_var);
}

/*
 * No error
 */
{
	gchar *some_var = g_strdup_printf ("%s %s", "hello", "world");
	floating_variant = g_variant_new ("s", some_var);
	g_free (some_var);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("o", "some string");
}

/*
 * Expected a GVariant variadic argument of type ‘char *’ but saw one of type ‘int’.
 *         floating_variant = g_variant_new ("o", 152);
 *                                                ^
 */
{
	floating_variant = g_variant_new ("o", 152);
}

/*
 * No error
 */
{
	const gchar *some_var = "hello world";
	floating_variant = g_variant_new ("o", some_var);
}

/*
 * No error
 */
{
	const char *some_var = "hello world";  // try plain C types
	floating_variant = g_variant_new ("o", some_var);
}

/*
 * No error
 */
{
	gchar *some_var = g_strdup_printf ("%s %s", "hello", "world");
	floating_variant = g_variant_new ("o", some_var);
	g_free (some_var);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("g", "some string");
}

/*
 * Expected a GVariant variadic argument of type ‘char *’ but saw one of type ‘int’.
 *         floating_variant = g_variant_new ("g", 152);
 *                                                ^
 */
{
	floating_variant = g_variant_new ("g", 152);
}

/*
 * No error
 */
{
	const gchar *some_var = "hello world";
	floating_variant = g_variant_new ("g", some_var);
}

/*
 * No error
 */
{
	const char *some_var = "hello world";  // try plain C types
	floating_variant = g_variant_new ("g", some_var);
}

/*
 * No error
 */
{
	gchar *some_var = g_strdup_printf ("%s %s", "hello", "world");
	floating_variant = g_variant_new ("g", some_var);
	g_free (some_var);
}

/*
 * Unexpected GVariant format strings ‘u’ with unpaired arguments. If using multiple format strings, they should be enclosed in brackets to create a tuple (e.g. ‘(su)’).
 *         floating_variant = g_variant_new ("su", "some", 56);
 *                                           ^
 */
{
	// Missing tuple brackets
	floating_variant = g_variant_new ("si", "some", 56);
}

/*
 * No error
 */
{
	// Simple tuple
	floating_variant = g_variant_new ("(si)", "some", 56);
}

/*
 * No error
 */
{
	// Nested tuples
	floating_variant = g_variant_new ("((s)(si))", "some", "other", 56);
}

/*
 * No error
 */
{
	// Empty tuple
	floating_variant = g_variant_new ("()");
}

/*
 * Invalid GVariant format string: tuple did not end with ‘)’.
 *         floating_variant = g_variant_new ("(si", "some", 56);
 *                                           ^
 */
{
	// Unbalanced brackets
	floating_variant = g_variant_new ("(si", "some", 56);
}

/*
 * No error
 */
{
	GVariant *some_var = g_variant_new_boolean (FALSE);
	floating_variant = g_variant_new ("v", some_var);
}

/*
 * No error
 */
{
	GVariant *some_var = g_variant_new_boolean (FALSE);
	floating_variant = g_variant_new ("v", (const GVariant *) some_var);
}

/*
 * Expected a GVariant variadic argument of type ‘GVariant *’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("v", "nope");
 *                                                ^
 */
{
	floating_variant = g_variant_new ("v", "nope");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("v", g_variant_new_boolean (FALSE));
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("ms", "some string");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("ms", NULL);
}

/*
 * Expected a GVariant variadic argument of type ‘char *’ but saw NULL instead.
 *         floating_variant = g_variant_new ("s", NULL);
 *                                           ^
 */
{
	// NULL strings are invalid.
	floating_variant = g_variant_new ("s", NULL);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("m(ss)", NULL, NULL);
}

/*
 * Expected a GVariant variadic argument of type ‘GVariant *’ but saw NULL instead.
 *         floating_variant = g_variant_new ("@s", NULL);
 *                                           ^
 */
{
	floating_variant = g_variant_new ("@s", NULL);
}

/*
 * Expected a GVariant variadic argument of type ‘GVariant *’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("@s", "some string");
 *                                                 ^
 */
{
	floating_variant = g_variant_new ("@s", "some string");
}

/*
 * No error
 */
{
	GVariant *some_variant = g_variant_new_string ("asd");
	floating_variant = g_variant_new ("@s", some_variant);
}

/*
 * No error
 */
{
	GVariant *some_variant1 = g_variant_new_string ("fasd");
	GVariant *some_variant2 = g_variant_new_string ("asdasd");
	floating_variant = g_variant_new ("(@s@ss)", some_variant1, some_variant2, "some string");
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("{ss}", "some string", "some string");
}

/*
 * Expected a GVariant variadic argument of type ‘char *’ but saw one of type ‘int’.
 *         floating_variant = g_variant_new ("{ss}", 5, FALSE);
 *                                                   ^
 */
{
	floating_variant = g_variant_new ("{ss}", 5, FALSE);
}

/*
 * Expected a GVariant basic type string but saw ‘m’.
 *         floating_variant = g_variant_new ("{msv}", "key", some_variant);
 *                                           ^
 */
{
	// Non-basic type as the key
	GVariant *some_variant = g_variant_new_string ("value");
	floating_variant = g_variant_new ("{msv}", "key", some_variant);
}

/*
 *  Expected a GVariant variadic argument of type ‘char *’ but saw NULL instead.
 *         floating_variant = g_variant_new ("{ss}", "key", NULL);
 *                                                          ^
 */
{
	floating_variant = g_variant_new ("{ss}", "key", NULL);
}

/*
 *  Expected a GVariant variadic argument of type ‘char *’ but saw NULL instead.
 *         floating_variant = g_variant_new ("{ss}", NULL, "key");
 *                                                   ^
 */
{
	floating_variant = g_variant_new ("{ss}", NULL, "value");
}

/*
 * Invalid GVariant format string: dict contains more than two elements.
 *         floating_variant = g_variant_new ("{sss}", "a", "b", "c");
 *                                           ^
 */
{
	// Too many elements
	floating_variant = g_variant_new ("{sss}", "a", "b", "c");
}

/*
 * Invalid GVariant format string: dict did not contain exactly two elements.
 *         floating_variant = g_variant_new ("{s}", "asd");
 *                                           ^
 */
{
	// Too few elements
	floating_variant = g_variant_new ("{s}", "asd");
}

/*
 * Invalid GVariant format string: dict did not contain exactly two elements.
 *         floating_variant = g_variant_new ("{}");
 *                                           ^
 */
{
	// Too few elements
	floating_variant = g_variant_new ("{}");
}

/*
 * No error
 */
{
	GVariant *some_variant = g_variant_new_boolean (FALSE);
	floating_variant = g_variant_new ("*", some_variant);
}

/*
 * No error
 */
{
	GVariant *some_variant = g_variant_new_boolean (FALSE);
	floating_variant = g_variant_new ("@*", some_variant);
}

/*
 * No error
 */
{
	GVariant *some_variant = g_variant_new_boolean (FALSE);
	floating_variant = g_variant_new ("?", some_variant);
}

/*
 * No error
 */
{
	GVariant *some_variant = g_variant_new_boolean (FALSE);
	floating_variant = g_variant_new ("@?", some_variant);
}

/*
 * No error
 */
{
	// FIXME: Check for non-basic types.
	GVariant *some_variant = g_variant_new_tuple (NULL, 0);
	floating_variant = g_variant_new ("@?", some_variant);
}

/*
 * No error
 */
{
	GVariant *some_variant = g_variant_new_tuple (NULL, 0);
	floating_variant = g_variant_new ("r", some_variant);
}

/*
 * No error
 */
{
	GVariant *some_variant = g_variant_new_tuple (NULL, 0);
	floating_variant = g_variant_new ("@r", some_variant);
}

/*
 * No error
 */
{
	// FIXME: Check for non-tuple types.
	GVariant *some_variant = g_variant_new_boolean (FALSE);
	floating_variant = g_variant_new ("@r", some_variant);
}

/*
 * No error
 */
{
	GVariantBuilder some_builder;
	g_variant_builder_init (&some_builder, G_VARIANT_TYPE_STRING);
	floating_variant = g_variant_new ("as", &some_builder);
}

/*
 * No error
 */
{
	GVariantBuilder *some_builder = g_variant_builder_new (G_VARIANT_TYPE_STRING);
	floating_variant = g_variant_new ("as", some_builder);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("as", NULL);
}

/*
 * No error
 */
{
	// FIXME: Check formats of GVariantBuilders.
	GVariantBuilder *some_builder = g_variant_builder_new (G_VARIANT_TYPE_BOOLEAN);
	floating_variant = g_variant_new ("as", some_builder);
}

/*
 * No error
 */
{
	// FIXME: Check formats of GVariantBuilders.
	GVariantBuilder some_builder;
	g_variant_builder_init (&some_builder, G_VARIANT_TYPE_BOOLEAN);
	floating_variant = g_variant_new ("as", &some_builder);
}

/*
 * No error
 */
{
	// FIXME: Disallow NULL for non-definite array element types.
	floating_variant = g_variant_new ("a?", NULL);
}

/*
 * No error
 */
{
	GVariantBuilder *some_builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
	floating_variant = g_variant_new ("a(sss)", some_builder);
}

/*
 * No error
 */
{
	GVariantBuilder *some_builder = g_variant_builder_new (G_VARIANT_TYPE_DICT_ENTRY);
	floating_variant = g_variant_new ("a{?*}", some_builder);
}

/*
 * Expected a GVariant variadic argument of type ‘GVariantBuilder *’ but saw one of type ‘char *’.
 *         floating_variant = g_variant_new ("au", "nope");
 *                                                 ^
 */
{
	floating_variant = g_variant_new ("au", "nope");
}

/*
 * No error
 */
{
	const gchar *str_array[] = { "a", "b", NULL };
	const gchar * const *some_str_array = (const gchar * const *) str_array;
	floating_variant = g_variant_new ("^as", some_str_array);
}

/*
 * No error
 */
{
	const gchar *str_array[] = { "a", "b", NULL };
	const gchar **some_str_array = (const gchar **) str_array;
	floating_variant = g_variant_new ("^as", some_str_array);
}

/*
 * No error
 */
{
	gchar **some_str_array = g_malloc (sizeof (gchar *) * 2);
	floating_variant = g_variant_new ("^as", some_str_array);
}

/*
 * No error
 */
{
	const gchar *some_str_array[] = { "a", "b", "c", NULL };
	floating_variant = g_variant_new ("^as", some_str_array);
}

/*
 *  Expected a GVariant variadic argument of type ‘char **’ but saw NULL instead.
 *         floating_variant = g_variant_new ("^as", NULL);
 *                                                  ^
 */
{
	// String arrays must be NULL-terminated.
	floating_variant = g_variant_new ("^as", NULL);
}

/*
 * No error
 */
{
	const gchar *str_array[] = { "a", "b", NULL };
	const gchar * const *some_str_array = (const gchar * const *) str_array;
	floating_variant = g_variant_new ("^a&s", some_str_array);
}

/*
 * No error
 */
{
	// FIXME: Validate that strings are object paths
	const gchar *some_str_array[] = { "/some/obj/path", "/some/object/path", NULL };
	floating_variant = g_variant_new ("^ao", some_str_array);
}

/*
 * No error
 */
{
	// FIXME: Validate that strings are object paths
	const gchar *some_str_array[] = { "/some/obj/path", "/some/object/path", NULL };
	floating_variant = g_variant_new ("^a&o", some_str_array);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("^ay", "some byte string");
}

/*
 * No error
 */
{
	const gchar *some_byte_string = "hello mum";
	floating_variant = g_variant_new ("^ay", some_byte_string);
}

/*
 * No error
 */
{
	const gchar some_byte_array[] = { 'a', 'b', 'c', '\0' };
	floating_variant = g_variant_new ("^ay", some_byte_array);
}

/*
 * No error
 */
{
	floating_variant = g_variant_new ("^&ay", (const char *) "some byte string");
}

/*
 * No error
 */
{
	const gchar *some_bytes_array[] = { "byte string", "more bytes", NULL };
	floating_variant = g_variant_new ("^aay", some_bytes_array);
}

/*
 * No error
 */
{
	const gchar *some_bytes_array[] = { "byte string", "more bytes", NULL };
	floating_variant = g_variant_new ("^a&ay", some_bytes_array);
}

/*
 * No error
 */
{
	va_list some_va_list;
	floating_variant = g_variant_new_va ("(sss)", NULL, &some_va_list);
}

/*
 * Unexpected GVariant format strings ‘nvalid’ with unpaired arguments. If using multiple format strings, they should be enclosed in brackets to create a tuple (e.g. ‘(invalid)’).
 *         floating_variant = g_variant_new_va ("invalid", NULL, &some_va_list);
 *                                              ^
 */
{
	va_list some_va_list;
	floating_variant = g_variant_new_va ("invalid", NULL, &some_va_list);
}

/*
 * Non-literal GVariant format string in call to g_variant_new(). Cannot check format string correctness. Instead of a non-literal format string, use GVariantBuilder.
 *         floating_variant = g_variant_new (some_format, "hi", "there");
 *                                           ^
 */
{
	// Check that non-const format strings are ignored.
	gchar *some_format = g_malloc (5);
	floating_variant = g_variant_new (some_format, "hi", "there");
}
