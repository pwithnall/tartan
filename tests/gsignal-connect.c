/* Template: gsignal */

/*
 * No error
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify", (GCallback) object_notify_cb,
	                  NULL);
}

/*
 * No error
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type

	// Detailed signal
	// FIXME: Check the detail is a valid property name too?
	g_signal_connect (some_object, "notify::something",
	                  (GCallback) object_notify_cb, NULL);
}

/*
 * No error
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_proto_cb,
	                  NULL);
}

/*
 * Could not check type of handler for signal ‘GObject::notify’. Callback function declaration does not contain parameter types.
 *                           (GCallback) object_notify_no_proto_cb,
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_no_proto_cb,
	                  NULL);
}

/*
 * Incorrect return type from signal handler for signal ‘GObject::notify’. Expected ‘void’ but saw ‘gboolean’.
 *                           (GCallback) object_notify_invalid_return_cb, NULL);
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_invalid_return_cb, NULL);
}

/*
 * Incorrect type for argument ‘self’ in signal handler for signal ‘GObject::notify’. Expected ‘GObject *’ but saw ‘GIOStream *’.
 *                           (GCallback) object_notify_invalid_parameter_subclass_cb,
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_invalid_parameter_subclass_cb,
	                  NULL);
}

/*
 * Incorrect type for argument ‘self’ in signal handler for signal ‘GObject::notify’. Expected ‘GObject *’ but saw ‘const GObject *’.
 *                           (GCallback) object_notify_invalid_parameter_const_cb,
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_invalid_parameter_const_cb,
	                  NULL);
}

/*
 * Incorrect type for argument ‘pspec’ in signal handler for signal ‘GObject::notify’. Expected ‘GParamSpec *’ but saw ‘void *’.
 *                           (GCallback) object_notify_invalid_parameter_middle_cb,
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_invalid_parameter_middle_cb,
	                  NULL);
}

/*
 * Incorrect type for argument ‘user_data’ in signal handler for signal ‘GObject::notify’. Expected ‘void *’ but saw ‘guint’.
 *                           (GCallback) object_notify_invalid_parameter_user_data_cb,
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_invalid_parameter_user_data_cb,
	                  NULL);
}

/*
 * No error
 */
{
	// Non-void* pointer types for user_data are allowed.
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_invalid_parameter_user_data_ptr_cb,
	                  NULL);
}

/*
 * Incorrect number of arguments in signal handler for signal ‘GObject::notify’. Expected 3 but saw 2.
 *                           (GCallback) object_notify_missing_parameter_cb, NULL);
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_missing_parameter_cb, NULL);
}

/*
 * Incorrect number of arguments in signal handler for signal ‘GObject::notify’. Expected 3 but saw 4.
 *                           (GCallback) object_notify_extra_parameter_cb, NULL);
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "notify",
	                  (GCallback) object_notify_extra_parameter_cb, NULL);
}

/*
 * No signal named ‘invalid-signal’ in GObject class ‘GObject’. To improve static analysis, add a typecast to the GObject parameter of g_signal_connect_data().
 *         g_signal_connect (some_object, "invalid-signal",
 *         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *     ^                      ~~~~~~~~~~  ~~~~~~~~~~~~~~~~~
 */
{
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect (some_object, "invalid-signal",
	                  (GCallback) object_notify_cb, NULL);
}

/*
 * Incorrect type for argument ‘key’ in signal handler for signal ‘GSettings::changed’. Expected ‘const char *’ but saw ‘gchar *’.
 *                           (GCallback) settings_changed_cb, NULL);
 *                                       ^
 * note: expanded from macro 'g_signal_connect'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
 *                                                            ^
 */
{
	// Signal handler doesn't mark the key as 'const'.
	GSettings *settings = g_malloc (5);  // only checking the type
	g_signal_connect (settings, "changed",
	                  (GCallback) settings_changed_cb, NULL);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	g_signal_connect (settings, "changed",
	                  (GCallback) settings_changed_const_cb, NULL);
}
