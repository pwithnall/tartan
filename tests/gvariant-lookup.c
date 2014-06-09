/* Template: gvariant */

/*
 * No error
 */
{
	// FIXME: Dynamically check that @existing_variant is a dictionary.
	gchar *str1 = NULL, *str2 = NULL;
	g_variant_lookup (existing_variant, "key", "(ss)", &str1, &str2);
}

/*
 * No error
 */
{
	gchar *str1 = NULL, *str2 = NULL;
	g_variant_lookup (existing_variant, "key", "(ss)", NULL, NULL);
}

/*
 * No error
 */
{
	const gchar *str1 = NULL, *str2 = NULL;
	g_variant_lookup (existing_variant, "key", "(&s&s)", &str1, &str2);
}

/*
 * No error
 */
{
	// The standard a{?*} example.
	GVariant *value;
	g_variant_lookup (existing_variant, "key", "*", &value);
}

/*
 * No error
 */
{
	// The standard a{sv} example.
	GVariant *value;
	g_variant_lookup (existing_variant, "key", "v", &value);
}

/*
 * Expected a GVariant variadic argument of type 'char **' but saw one of type 'guint *' (aka 'unsigned int *').
 *         g_variant_lookup (existing_variant, "key", "s", &not_a_string);
 *                                                         ^
 */
{
	guint not_a_string;
	g_variant_lookup (existing_variant, "key", "s", &not_a_string);
}

/*
 * Unexpected GVariant variadic argument of type 'char *'. Either it should be removed, or a ‘s’ (or other valid) GVariant format string should be added to the format argument to use it.
 *                           "server", "&s", &server_param);
 *                           ^
 * Unexpected GVariant variadic argument of type 'char *'. Either it should be removed, or a ‘s’ (or other valid) GVariant format string should be added to the format argument to use it.
 *                           "server", "&s", &server_param);
 *                                     ^
 * Unexpected GVariant variadic argument of type 'const gchar **' (aka 'const char **'). Either it should be removed, or a ‘^as’ (or other valid) GVariant format string should be added to the format argument to use it.
 *                           "server", "&s", &server_param);
 *                                           ^
 */
{
	/* Incorrect usage of g_variant_lookup() found in the wild in tp-glib.
	 * This is the first real bug Tartan found!
	 * (g_variant_lookup() can only look up one key, not multiple.) */
	const gchar *account_param, *server_param;
	g_variant_lookup (existing_variant,
	                  "account", "&s", &account_param,
	                  "server", "&s", &server_param);
}
