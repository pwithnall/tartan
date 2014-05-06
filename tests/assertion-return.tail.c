	/* End of assertion_val_func(). */

	return NULL;
}

int
main (void)
{
	GObject *obj = g_malloc (5);

	/* Various NULL and non-NULL calls to the function. */
	assertion_val_func ("str", 0, obj);
	assertion_val_func (NULL, 1, obj);
	assertion_val_func ("str", 2, NULL);
	assertion_val_func (NULL, 3, NULL);

	g_free (obj);
}
