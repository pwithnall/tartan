	/* End of some_failable_func(). */
}

int
main (void)
{
	GError *main_error = NULL;

	some_failable_func (rand (), &main_error);

	if (main_error != NULL) {
		g_error_free (main_error);
		return 1;
	}

	return 0;
}
