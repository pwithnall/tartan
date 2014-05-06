	/* End of assertion_func(). */
}

int
main (void)
{
	GObject *obj = g_malloc (5);

	/* Various NULL and non-NULL calls to the function. */
	assertion_func ("str", 0, obj);
	assertion_func (NULL, 1, obj);
	assertion_func ("str", 2, NULL);
	assertion_func (NULL, 3, NULL);

	g_free (obj);
}
