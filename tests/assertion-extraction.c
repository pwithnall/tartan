/* Template: assertion */

/*
 * No error
 */
{
	// Nothing to see here, so all parameters are nullable.
}

/*
 * warning: null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 1, obj);
 *                         ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                         ~~~~         ^
 */
{
	g_return_if_fail (some_str != NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_func ("str", 2, NULL);
 *                                   ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                                  ~~~~^
 */
{
	g_return_if_fail (some_obj != NULL);
}

/*
 *  null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 1, obj);
 *                         ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_func ("str", 2, NULL);
 *                                   ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                                  ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                         ~~~~         ^
 */
{
	g_return_if_fail (some_str != NULL);
	g_return_if_fail (some_obj != NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_func ("str", 2, NULL);
 *                                   ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                                  ~~~~^
 */
{
	gpointer some_priv = (gpointer) some_obj;  // simulated setup of private data

	g_return_if_fail (some_obj != NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_func ("str", 2, NULL);
 *                                   ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                                  ~~~~^
 */
{
	g_return_if_fail ("unrelated" == "assertion");
	g_return_if_fail (some_obj != NULL);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 1, obj);
 *                         ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                         ~~~~         ^
 */
{
	// A non-explicit non-NULL check.
	g_return_if_fail (some_str);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 1, obj);
 *                         ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                         ~~~~         ^
 */
{
	// A really weird non-NULL check.
	g_return_if_fail (some_str != 0);
}

/*
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 1, obj);
 *                         ~~~~        ^
 * null passed to a callee which requires a non-null argument
 *         assertion_func ("str", 2, NULL);
 *                                   ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                                  ~~~~^
 * null passed to a callee which requires a non-null argument
 *         assertion_func (NULL, 3, NULL);
 *                         ~~~~         ^
 */
{
	// Dual non-NULL checks.
	g_return_if_fail (some_str && some_obj);
}

/*
 * No error
 */
{
	// Can’t statically analyse this at the moment.
	g_return_if_fail (some_str || some_obj);
}
