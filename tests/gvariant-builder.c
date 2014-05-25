/* Template: gvariant */

/*
 * No error
 */
{
	GVariantBuilder *builder;
	int i;

	builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
	for (i = 0; i < 16; i++) {
		gchar buf[3];

		sprintf (buf, "%d", i);
		g_variant_builder_add (builder, "{is}", i, buf);
	}

	g_variant_builder_end (builder);
}

/*
 * Expected a GVariant variadic argument of type 'char *' but saw one of type 'int'.
 *         g_variant_builder_add (builder, "{ss}", "hi", 15);
 *                                                       ^
 */
{
	GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add (builder, "{ss}", "hi", 15);
	g_variant_builder_end (builder);
}

/*
 * Expected a GVariant variadic argument of type 'char *' but saw NULL instead.
 *         g_variant_builder_add (builder, "s", FALSE);
 *                                              ^
 */
{
	GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add (builder, "s", "hi");
	g_variant_builder_add (builder, "s", "hi");
	g_variant_builder_add (builder, "s", FALSE);
	g_variant_builder_end (builder);
}
