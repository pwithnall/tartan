/* Template: assertion-return */

/*
 * No error
 */
{
	// Nothing to see here, so all parameters are nullable.
}

/*
 * warning: null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 1, obj);
 *                             ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                             ~~~~         ^
 */
{
	g_return_val_if_fail (some_str != NULL, NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func ("str", 2, NULL);
 *                                       ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                                      ~~~~^
 */
{
	g_return_val_if_fail (some_obj != NULL, NULL);
}

/*
 *  null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 1, obj);
 *                             ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func ("str", 2, NULL);
 *                                       ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                                      ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                             ~~~~         ^
 */
{
	g_return_val_if_fail (some_str != NULL, NULL);
	g_return_val_if_fail (some_obj != NULL, NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func ("str", 2, NULL);
 *                                       ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                                      ~~~~^
 */
{
	gpointer some_priv = (gpointer) some_obj;  // simulated setup of private data

	g_return_val_if_fail (some_obj != NULL, NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func ("str", 2, NULL);
 *                                       ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                                      ~~~~^
 */
{
	g_return_val_if_fail ("unrelated" == "assertion", NULL);
	g_return_val_if_fail (some_obj != NULL, NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 1, obj);
 *                             ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                             ~~~~         ^
 */
{
	// A non-explicit non-NULL check.
	g_return_val_if_fail (some_str, NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 1, obj);
 *                             ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                             ~~~~         ^
 */
{
	// A really weird non-NULL check.
	g_return_val_if_fail (some_str != 0, NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 1, obj);
 *                             ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func ("str", 2, NULL);
 *                                       ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                                      ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                             ~~~~         ^
 */
{
	// Dual non-NULL checks.
	g_return_val_if_fail (some_str && some_obj, NULL);
}

/*
 * No error
 */
{
	// Can’t statically analyse this at the moment.
	g_return_val_if_fail (some_str || some_obj, NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 1, obj);
 *                             ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_val_func (NULL, 3, NULL);
 *                             ~~~~         ^
 */
{
	GList *random_variable;

	g_return_val_if_fail (some_str, FALSE);
}
