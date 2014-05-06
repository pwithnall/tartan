/*
 * No error
 */
{
	guint64 size = g_ascii_strtoull ("some-constant-string", NULL, 10);
}

/*
 * No error
 */
{
	const gchar *some_str = "foo";
	guint64 size = g_ascii_strtoull (some_str, NULL, 10);
}

/*
 * No error
 */
{
	const gchar *endptr = NULL;
	guint64 size = g_ascii_strtoull ("some-constant-string", (gchar **) &endptr, 10);
}

/*
 * null passed to a callee which requires a non-null argument
 *         guint64 size = g_ascii_strtoull (NULL, NULL, 10);
 *                                          ~~~~          ^
 */
{
	guint64 size = g_ascii_strtoull (NULL, NULL, 10);
}
