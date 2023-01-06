/* GtkItemEntry - widget for gtk+
 * Copyright (C) 1999-2001 Adrian E. Feiguin <adrian@ifir.ifir.edu.ar>
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkItemEntry widget by Adrian E. Feiguin
 * Based on GtkEntry widget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef gtkextra_GTK_ITEM_ENTRY_H_
#define gtkextra_GTK_ITEM_ENTRY_H_

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_TYPE_ITEM_ENTRY            (gtk_item_entry_get_type ())
#define GTK_ITEM_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_item_entry_get_type (), GtkItemEntry))
#define GTK_ITEM_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, gtk_item_entry_get_type (), GtkItemEntryClass))
#define GTK_IS_ITEM_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_item_entry_get_type ()))
#define GTK_IS_ITEM_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ENTRY))

typedef struct {
  GtkEntry parent;

  gint text_max_size;

  GtkJustification justification;
} GtkItemEntry;

typedef struct {
  GtkEntryClass parent_class;
} GtkItemEntryClass;

GType gtk_item_entry_get_type(void);
GtkWidget* gtk_item_entry_new(void);
void gtk_item_entry_set_text(GtkItemEntry *item_entry,
                             const gchar *text,
                             GtkJustification justification);

void gtk_item_entry_set_justification(GtkItemEntry *item_entry,
                                      GtkJustification justification);

#ifdef __cplusplus
}
#endif

#endif /* gtkextra_GTK_ITEM_ENTRY_H_ */
