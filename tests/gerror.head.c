#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

static void
some_failing_func (GError **error)
{
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_PENDING, "Pending error");
}

static void
some_failable_func (gboolean some_cond, GError **error)
{
