#include <stdio.h>

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

static void
object_notify_cb (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	/* Something */
}

extern void
object_notify_proto_cb (GObject *gobject, GParamSpec *pspec, gpointer user_data);

extern void
object_notify_no_proto_cb ();

static gboolean
object_notify_invalid_return_cb (GObject *gobject, GParamSpec *pspec,
                                 gpointer user_data)
{
	/* Something */
	return FALSE;
}

static void
object_notify_invalid_parameter_subclass_cb (GIOStream *gobject,
                                             GParamSpec *pspec,
                                             gpointer user_data)
{
	/* Something */
}

static void
object_notify_invalid_parameter_const_cb (const GObject *gobject,
                                          GParamSpec *pspec,
                                          gpointer user_data)
{
	/* Something */
}

static void
object_notify_invalid_parameter_middle_cb (GObject *gobject,
                                           void *pspec,
                                           gpointer user_data)
{
	/* Something */
}

static void
object_notify_invalid_parameter_user_data_cb (GObject *gobject,
                                              GParamSpec *pspec,
                                              guint user_data)
{
	/* Something */
}

static void
object_notify_invalid_parameter_user_data_ptr_cb (GObject *gobject,
                                                  GParamSpec *pspec,
                                                  GObject *user_data)
{
	/* Something */
}

static void
object_notify_missing_parameter_cb (GObject *gobject, GParamSpec *pspec)
{
	/* Something */
}

static void
object_notify_extra_parameter_cb (GObject *gobject, GParamSpec *pspec,
                                  gpointer user_data, gpointer user_data2)
{
	/* Something */
}

static void
settings_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
	/* Something */
}

static void
settings_changed_const_cb (GSettings *settings, const gchar *key,
                           gpointer user_data)
{
	/* Something */
}

static void
settings_changed_swapped_cb (gpointer user_data, const gchar *key,
                             GSettings *settings)
{
	/* Something */
}

static void
settings_changed_swapped_stream_cb (GInputStream *stream, const gchar *key,
                                    GSettings *settings)
{
	/* Something */
}

static void
application_activate_swapped_cb (GSettings *self, GApplication *app)
{
	/* Done */
}

static void
application_activate_swapped_gpointer_cb (GSettings *self, GApplication *app)
{
	/* Done */
}

static void
application_activate_swapped_incorrect_cb (GSettings *self, const gchar *incorrect)
{
	/* Done */
}

static void
application_activate_swapped_excess_arguments_cb (GSettings *self,
                                                  const gchar *some_random_arg,
                                                  guint another_random_arg,
                                                  gpointer user_data)
{
	/* Done */
}

static void
dbus_object_interface_added_cb (GDBusObject *obj, GDBusInterface *iface,
                                gpointer user_data)
{
	/* Done */
}

static void
dbus_object_proxy_interface_added_cb (GDBusObjectProxy *obj,
                                      GDBusInterface *iface,
                                      gpointer user_data)
{
	/* Done */
}

static void
object_interface_added_cb (GObject *obj, GDBusInterface *iface,
                           gpointer user_data)
{
	/* Done */
}

static void
dbus_connection_closed_cb (GDBusConnection *conn,
                           gboolean remote_peer_vanished,
                           GError *error,
                           gpointer user_data)
{
	/* Done */
}

static gboolean
tls_connection_accept_certificate_cb (GTlsConnection *conn,
                                      GTlsCertificate *peer_certificate,
                                      GTlsCertificateFlags errors,
                                      gpointer user_data)
{
	/* Done */
	return FALSE;
}

int
main (void)
{
