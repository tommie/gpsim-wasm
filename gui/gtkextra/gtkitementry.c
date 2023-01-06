/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "../config.h"
#ifdef HAVE_GUI

#include <stddef.h>

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gtkitementry.h"

G_DEFINE_TYPE(GtkItemEntry, gtk_item_entry, GTK_TYPE_ENTRY)

static void
gtk_item_entry_class_init(GtkItemEntryClass *class)
{
}

static void
gtk_item_entry_init(GtkItemEntry *entry)
{
  entry->justification = GTK_JUSTIFY_LEFT;
  entry->text_max_size = 0;
  gtk_entry_set_has_frame(GTK_ENTRY(entry), FALSE);
}

/* Public API
 */

GtkWidget*
gtk_item_entry_new(void)
{
  GtkWidget *widget;
  widget = GTK_WIDGET(g_object_new(GTK_TYPE_ITEM_ENTRY, NULL));
  return widget;
}

void
gtk_item_entry_set_text(GtkItemEntry    *entry,
                        const gchar *text,
                        GtkJustification justification)
{
  g_return_if_fail(GTK_IS_ITEM_ENTRY(entry));
  g_return_if_fail(text != NULL);
  entry->justification = justification;

  /* Call the parent */
  gtk_entry_set_text(GTK_ENTRY(entry), text);
}

void
gtk_item_entry_set_justification(GtkItemEntry *entry, GtkJustification just)
{
  g_return_if_fail(GTK_IS_ITEM_ENTRY(entry));
  entry->justification = just;
}

#endif // HAVE_GUI

