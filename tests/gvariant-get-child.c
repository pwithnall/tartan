/* Template: gvariant */

/*
 * No error
 */
{
	gchar *str1 = NULL, *str2 = NULL;
	g_variant_get_child (existing_variant, 0, "(ss)", &str1, &str2);
}

/*
 * No error
 */
{
	gchar *str1 = NULL, *str2 = NULL;
	g_variant_get_child (existing_variant, 0, "(ss)", NULL, NULL);
}

/*
 * No error
 */
{
	const gchar *str1 = NULL, *str2 = NULL;
	g_variant_get_child (existing_variant, 0, "(&s&s)", &str1, &str2);
}

/*
 * No error
 */
{
	g_variant_get_child (existing_variant, 0, "()");
}

/*
 * Expected a GVariant variadic argument of type ‘const char **’ but saw one of type ‘guint *’.
 *         g_variant_get_child (existing_variant, 0, "&s", &not_a_string);
 *                                                         ^
 */
{
	guint not_a_string;
	g_variant_get_child (existing_variant, 0, "&s", &not_a_string);
}
