/* Template: gvariant */

/*
 * No error
 */
{
	GVariant *dictionary;
	GVariantIter iter;
	GVariant *value;
	gchar *key;

	g_variant_iter_init (&iter, dictionary);
	while (g_variant_iter_loop (&iter, "{sv}", &key, &value)) {
		/* Something */
	}
}

/*
 *  Expected a GVariant variadic argument of type ‘GVariant **’ but saw one of type ‘gchar **’.
 *         while (g_variant_iter_loop (&iter, "{sv}", &key, &not_a_value)) {
 *                                                          ^
 */
{
	GVariant *dictionary;
	GVariantIter iter;
	gchar *not_a_value;
	gchar *key;

	g_variant_iter_init (&iter, dictionary);
	while (g_variant_iter_loop (&iter, "{sv}", &key, &not_a_value)) {
		/* Something */
	}
}

/*
 * No error
 */
{
	GVariant *dictionary;
	GVariantIter iter;
	GVariant *value;
	gchar *key;

	g_variant_iter_init (&iter, dictionary);
	while (g_variant_iter_next (&iter, "{sv}", &key, &value)) {
		/* Something */

		g_variant_unref (value);
		g_free (key);
	}
}

/*
 * Expected a GVariant variadic argument of type ‘GVariant **’ but saw one of type ‘gchar **’.
 *         while (g_variant_iter_next (&iter, "{sv}", &key, &not_a_value)) {
 *                                                          ^
 */
{
	GVariant *dictionary;
	GVariantIter iter;
	gchar *not_a_value;
	gchar *key;

	g_variant_iter_init (&iter, dictionary);
	while (g_variant_iter_next (&iter, "{sv}", &key, &not_a_value)) {
		/* Something */

		g_free (not_a_value);
		g_free (key);
	}
}
