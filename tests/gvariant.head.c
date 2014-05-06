#include <stdio.h>

#include <glib.h>

int
main (void)
{
	GVariant *floating_variant = NULL, *existing_variant;
	existing_variant = g_variant_new_boolean (FALSE);  /* arbitrary */
