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

int
main (void)
{
