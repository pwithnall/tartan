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
 * Expected a GVariant variadic argument of type ‘char **’ but saw one of type ‘guint *’.
 *         g_variant_lookup (existing_variant, "key", "s", &not_a_string);
 *                                                         ^
 */
{
	guint not_a_string;
	g_variant_lookup (existing_variant, "key", "s", &not_a_string);
}
