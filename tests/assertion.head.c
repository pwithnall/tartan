#include <stdio.h>

#include <glib.h>
#include <glib-object.h>

static void
assertion_func (const gchar *some_str, guint some_int, GObject *some_obj)
{
