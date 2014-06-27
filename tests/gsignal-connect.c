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
 * No error
 */
{
	// As long as object_notify_missing_parameter_cb is using a safe calling
	// convention, this is actually fine.
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

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	g_signal_connect_swapped (settings, "changed",
	                          (GCallback) settings_changed_swapped_cb,
	                          NULL);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	gint some_thing[] = { 0, 1, 2 }; // subtype of gpointer, so valid
	g_signal_connect_swapped (settings, "changed",
	                          (GCallback) settings_changed_swapped_cb,
	                          some_thing);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	gint some_thing[] = { 0, 1, 2 }; // subtype of gpointer, so valid
	g_signal_connect_swapped (settings, "changed",
	                          (GCallback) settings_changed_swapped_cb,
	                          some_thing);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	gint some_thing[] = { 0, 1, 2 }; // not a GInputStream; FIXME: might be marked as invalid in future
	g_signal_connect_swapped (settings, "changed",
	                          (GCallback) settings_changed_swapped_stream_cb,
	                          some_thing);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	GFilterInputStream *stream = g_malloc (5);  // GInputStream subtype; valid
	g_signal_connect_swapped (settings, "changed",
	                          (GCallback) settings_changed_swapped_stream_cb,
	                          stream);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	GObject *stream = g_malloc (5);  // GInputStream supertype; FIXME: might be marked as invalid in future
	g_signal_connect_swapped (settings, "changed",
	                          (GCallback) settings_changed_swapped_stream_cb,
	                          stream);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	gint some_thing[] = { 0, 1, 2 }; // subtype of gpointer, so valid
	g_signal_connect_data (settings, "changed",
	                       (GCallback) settings_changed_swapped_cb,
	                       some_thing, NULL, G_CONNECT_SWAPPED);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	gint some_thing[] = { 0, 1, 2 }; // subtype of gpointer, so valid
	g_signal_connect_data (settings, "changed",
	                       (GCallback) settings_changed_swapped_cb,
	                       some_thing, NULL,
	                       G_CONNECT_SWAPPED | G_CONNECT_AFTER);
}

/*
 * No error
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	GApplication *app = g_malloc (5);  // only checking the type
	g_signal_connect_swapped (app, "activate",
	                          G_CALLBACK (application_activate_swapped_cb),
	                          settings);
}

/*
 * Incorrect number of arguments in signal handler for signal ‘GApplication::activate’. Expected 2 but saw 4.
 *                                   G_CALLBACK (application_activate_swapped_excess_arguments_cb),
 *                                               ^
 * note: expanded from macro 'G_CALLBACK'
 * #define G_CALLBACK(f)                    ((GCallback) (f))
 *                                                        ^
 * note: expanded from macro 'g_signal_connect_swapped'
 *     g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, G_CONNECT_SWAPPED)
 *                                                            ^
 */
{
	GSettings *settings = g_malloc (5);  // only checking the type
	GApplication *app = g_malloc (5);  // only checking the type
	g_signal_connect_swapped (app, "activate",
	                          G_CALLBACK (application_activate_swapped_excess_arguments_cb),
	                          settings);
}

/*
 * No error
 */
{
	// A more typical use of G_CONNECT_SWAPPED, where we use an existing
	// function and swap the arguments to avoid writing a wrapper function.
	GApplication *app = g_malloc (5);  // only checking the type
	GObject *some_object = g_malloc (5);  // only checking the type
	g_signal_connect_swapped (app, "activate",
	                          G_CALLBACK (g_object_run_dispose), some_object);
}

/*
 * No error
 */
{
	// Connecting to a signal defined on an interface.
	GDBusObject *obj = g_malloc (5);  // only checking the type
	g_signal_connect (obj, "interface-added",
	                  (GCallback) dbus_object_interface_added_cb, NULL);
}

/*
 * No error
 */
{
	// Connecting to a signal defined on an interface implemented by the
	// object.
	GDBusObjectProxy *proxy = g_malloc (5);  // only checking the type
	g_signal_connect (proxy, "interface-added",
	                  (GCallback) dbus_object_interface_added_cb, NULL);
}
