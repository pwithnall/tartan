#include <stdio.h>

#include <glib.h>
#include <glib-object.h>

static gpointer
assertion_val_func (const gchar *some_str, guint some_int, GObject *some_obj)
{
