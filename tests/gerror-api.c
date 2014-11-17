/* Template: gerror */

/*
 * No error
 */
{
	GError *some_error = g_error_new (G_IO_ERROR,
	                                  G_IO_ERROR_FAILED, "bah!");
	g_error_free (some_error);
}

/*
 * No error
 */
{
	GError *some_error = g_error_new_literal (G_IO_ERROR,
	                                          G_IO_ERROR_FAILED, "bah!");
	g_error_free (some_error);
}

/*
 * No error
 */
{
	GError *some_error;

	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");

	if (some_error == NULL) {
		// Should never be reached, so we should never get an error
		// about a double-free.
		g_error_free (some_error);
	}

	g_error_free (some_error);
}

/*
 * warning: Freeing non-set GError
 *         g_error_free (NULL);
 *         ^~~~~~~~~~~~~~~~~~~
 */
{
	g_error_free (NULL);
}

/*
 * warning: Freeing non-set GError
 *         g_error_free (some_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error = NULL;
	g_error_free (some_error);
}

/*
 * warning: Using uninitialized GError
 *         g_error_free (some_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error;  // uninitialised
	g_error_free (some_error);
}

/*
 *  warning: Freeing non-set GError
 *         g_error_free (some_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error = NULL;

	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_clear_error (&some_error);

	g_error_free (some_error);
}

/*
 * No error
 */
{
	GError *some_error = NULL;

	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_error_free (some_error);

	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "new");
	g_error_free (some_error);
}

/*
 * warning: Overwriting already-freed GError
 *         g_set_error (&some_error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error = NULL;

	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_error_free (some_error);

	g_set_error (&some_error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
	g_clear_error (&some_error);
}

/*
 * No error
 */
{
	GError *some_error = NULL;
	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_error_free (some_error);
}

/*
 * warning: Freeing already-freed GError
 *         g_error_free (some_error);  // double free
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error = NULL;
	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_error_free (some_error);
	g_error_free (some_error);  // double free
}

/*
 * No error
 */
{
	GError *some_error;  // don’t explicitly initialise
	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_error_free (some_error);
}

/*
 * warning: Freeing non-set GError
 *         g_error_free (some_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error = NULL;
	if (some_cond) {
		// Bug: @some_error is not set on the FALSE path.
		some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	}
	g_error_free (some_error);
}

/*
 * warning: Overwriting already-set GError
 *         some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *         ~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error = NULL;
	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
}

/*
 * warning: Overwriting already-set GError
 *         some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *         ~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *some_error = NULL;
	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	some_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
	g_error_free (some_error);
}

/*
 * No error
 */
{
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
}

/*
 * No error
 */
{
	g_set_error (NULL, G_IO_ERROR, G_IO_ERROR_FAILED, "no-op");
}

/*
 * No error
 */
{
	GError *sub_error = NULL;

	g_set_error (&sub_error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");

	if (sub_error == NULL) {
		// Should never be reached, so we should never get an error
		// about double-freeing.
		g_error_free (sub_error);
	}

	g_error_free (sub_error);
}

/*
 * No error
 */
{
	GError *child_error = NULL;
	g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_FAILED, "no-op");
	g_error_free (child_error);
}

/*
 * warning: Using uninitialized GError
 *         g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_FAILED, "non-clear");
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *child_error;  // uninitialised
	g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_FAILED, "non-clear");
	g_error_free (child_error);
}

/*
 * warning: Overwriting already-set GError
 *         g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
}

/*
 * warning: Overwriting already-set GError
 *         g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
	g_clear_error (error);
}

/*
 * No error
 */
{
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_clear_error (error);
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah again!");
}

/*
 * warning: Overwriting already-set GError
 *         g_set_error (error, G_IO_ERROR, G_IO_ERROR_PENDING, "Pending error");
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	guint i;
	GError *child_error = NULL;

	for (i = 0; i < 5; i++) {
		some_failing_func (&child_error);
		// ^-- bug: can overwrite @child_error when i > 0
	}
}

/*
 * warning: Overwriting already-set GError
 *         g_propagate_error (error, sub_error2);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error1, *sub_error2;

	sub_error1 = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure 1");
	sub_error2 = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure 2");

	g_propagate_error (error, sub_error1);
	g_propagate_error (error, sub_error2);
}

/*
 * warning: Overwriting already-set GError
 *                 g_propagate_error (error, sub_error);
 *                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	guint i;

	for (i = 0; i < 5; i++) {
		GError *sub_error = NULL;

		sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
		                         "Failure");
		g_propagate_error (error, sub_error);
		// ^-- bug: will overwrite @error when i > 0
	}
}

/*
 * warning: Freeing already-freed GError
 *         g_error_free (main_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
	                         "Failure");
	g_propagate_error (error, sub_error);
	g_error_free (sub_error);
	// ^-- sort-of-bug: This is the first time @sub_error is freed, but it
	//     means a dangling pointer is returned to the caller in @error
	//     FIXME: The dangling pointer should be detected and warned about
	//     more specifically.
}

/*
 * warning: Freeing already-freed GError
 *         g_error_free (main_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_error (error, sub_error);
	g_clear_error (&sub_error);
	// ^-- sort-of-bug: This is the first time @sub_error is freed, but it
	//     means a dangling pointer is returned to the caller in @error
	//     FIXME: The dangling pointer should be detected and warned about
	//     more specifically.
}

/*
 * warning: Freeing already-freed GError
 *         g_clear_error (&sub_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_error (NULL, sub_error);
	g_clear_error (&sub_error);
	// ^-- bug: @sub_error has already effectively been freed
}

/*
 * warning: Overwriting already-set GError
 *         sub_error = NULL;
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_error (error, sub_error);
	// FIXME: false positive on this line; should be no error:
	sub_error = NULL;
	g_clear_error (&sub_error);
}

/*
 * No error
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_prefixed_error (error, sub_error, "prefix: ");
}

/*
 * warning: Freeing already-freed GError
 *         g_clear_error (&sub_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_prefixed_error (NULL, sub_error, "prefix: ");
	g_clear_error (&sub_error);
	// ^-- bug: @sub_error has already effectively been freed
}

/*
 * warning: Freeing non-set GError
 *         g_propagate_error (error, sub_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error = NULL;
	g_propagate_error (error, sub_error);
}

/*
 * warning: Freeing already-freed GError
 *         g_propagate_error (error, sub_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_error_free (sub_error);

	g_propagate_error (error, sub_error);
}

/*
 * No error
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_error (NULL, sub_error);
}

/*
 * warning: Using uninitialized GError
 *         g_propagate_error (&dest_error, sub_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error;
	GError *dest_error;  // uninitialised

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_error (&dest_error, sub_error);
}

/*
 * No error
 */
{
	GError *sub_error;
	GError *dest_error = NULL;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_error (&dest_error, sub_error);

	if (dest_error == NULL) {
		// Should never be reached, so this double-free should never be
		// a warning.
		g_error_free (sub_error);
	}
}

/*
 * No error
 */
{
	GError *sub_error;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "Failure");
	g_propagate_error (error, sub_error);

	if (error != NULL && *error == NULL) {
		// Should never be reached, so this double-free should never be
		// a warning.
		g_error_free (sub_error);
	}
}

/*
 * warning: Using uninitialized GError
 *         g_clear_error (&sub_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error;
	g_clear_error (&sub_error);
}

/*
 * No error
 */
{
	g_clear_error (NULL);
}

/*
 * No error
 */
{
	GError *sub_error = NULL;
	g_clear_error (&sub_error);
}

/*
 * warning: Freeing already-freed GError
 *         g_clear_error (&sub_error);
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error = NULL;

	g_set_error (&sub_error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_error_free (sub_error);
	g_clear_error (&sub_error);
}

/*
 * No error
 */
{
	GError *sub_error = NULL;

	g_set_error (&sub_error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_clear_error (&sub_error);

	if (sub_error != NULL) {
		// Should never be reached, so we should never get an error.
		g_error_free (sub_error);
	}
}

/*
 * No error
 */
{
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
}

/*
 * warning: Overwriting already-set GError
 *         g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "bah!");
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
}

/*
 * No error
 */
{
	GError *sub_error = NULL;

	if (rand ()) {
		g_set_error (&sub_error, G_IO_ERROR, G_IO_ERROR_FAILED, "blah");
	}

	g_clear_error (&sub_error);
}

/*
 * warning: Overwriting already-set GError
 *                 g_set_error (&sub_error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error = NULL;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "original");

	if (rand ()) {
		g_set_error (&sub_error, G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
	}

	g_clear_error (&sub_error);
}

/*
 * No error
 */
{
	GError *sub_error = NULL;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "original");

	if (rand ()) {
		g_propagate_error (error, sub_error);
	} else {
		g_error_free (sub_error);
	}
}

/*
 *  warning: Overwriting already-set GError
 *                 g_propagate_error (&sub_error, sub_error);
 *                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error = NULL;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "original");

	if (rand ()) {
		g_propagate_error (&sub_error, sub_error);
	} else {
		g_error_free (sub_error);
	}
}

/*
 * No error
 */
{
	GError *sub_error = NULL;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "original");

	if (rand ()) {
		g_clear_error (&sub_error);
	}

	g_clear_error (&sub_error);
}

/*
 * warning: Freeing already-freed GError
 *                 g_clear_error (&sub_error);
 *                 ^~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error = NULL;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "original");
	g_error_free (sub_error);

	if (rand ()) {
		g_clear_error (&sub_error);
	}
}

/*
 * No error
 */
{
	GError *sub_error = NULL;

	if (rand ()) {
		sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "1");
	} else {
		sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "2");
	}

	g_error_free (sub_error);
}

/*
 * warning: Overwriting already-set GError
 *                 sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
 *                 ~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
{
	GError *sub_error = NULL;

	sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "1");

	if (rand ()) {
		sub_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED, "dupe");
	}

	g_error_free (sub_error);
}
