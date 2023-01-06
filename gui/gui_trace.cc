/*
   Copyright (C) 1998,1999,2000,2001
   T. Scott Dattalo and Ralf Forsberg

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "../config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include <stdio.h>

#include "../src/gpsim_time.h"
#include "../src/trace.h"
#include "../src/xref.h"
#include "gui.h"
#include "gui_processor.h"
#include "gui_trace.h"

#define MAXTRACES  100
#define TRACE_STRING 100

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_ADD_WATCH,
} menu_id;

// gui trace flags:
#define GTF_ENABLE_XREF_UPDATES    (1<<0)

enum {CYCLE_COLUMN, TRACE_COLUMN, N_COLUMNS};

//========================================================================

class TraceXREF : public CrossReferenceToGUI
{
public:

  /*****************************************************************
   * Update
   *
   * This is called by the simulator when it has been determined that
   * that the trace buffer has changed and needs to be updated
   */

  void Update(int ) override
  {
    Trace_Window *tw  = static_cast<Trace_Window *>(parent_window);

    if (!tw  || !tw->enabled)
      return;

    if (!tw->gp || !tw->gp->cpu)
      {
        puts("Warning gp or gp->cpu == NULL in TraceWindow_update");
        return;
      }

    // If we're not allowing xref updates then exit
    if ( !(tw->trace_flags & GTF_ENABLE_XREF_UPDATES))
      return;

    if (get_trace().string_buffer[0] && (get_trace().string_cycle >= tw->last_cycle)) {
      tw->last_cycle = get_trace().string_cycle;
      tw->trace_map[tw->trace_map_index].cycle = get_trace().string_cycle;
      tw->trace_map[tw->trace_map_index].simulation_trace_index = get_trace().string_index;

      // Advance the trace_map_index using rollover arithmetic
      if (++tw->trace_map_index >= MAXTRACES)
        tw->trace_map_index = 0;

      GtkListStore *list = tw->trace_list;
      GtkTreeIter iter;

      gtk_list_store_append(list, &iter);
      gtk_list_store_set(list, &iter,
        CYCLE_COLUMN, guint64(get_trace().string_cycle),
        TRACE_COLUMN, get_trace().string_buffer,
        -1);

      if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list), nullptr) > MAXTRACES) {
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list), &iter);
        gtk_list_store_remove(list, &iter);
      }

    }

  }

};

//========================================================================

/*****************************************************************
 * TraceWindow_update
 *
 * The purpose of this routine is to refresh the trace window with
 * the latest trace information. The current pic simulation cycle (should
 * this be change to real time???) is examined and compared to what
 * is currently displayed in the trace window. If the info in the
 * trace window is really old, this the entire window is deleted and
 * the trace is redrawn with the latest. If the trace window is rather
 * recent then the older trace info is deleted and the new is appended
 * to the end.
 *
 * INPUTS: *tw a pointer to a Trace_Window structure.
 */

void Trace_Window::Update()
{
  //guint64 cycle;

  if (!enabled)
    return;

  if (!gp || !gp->cpu)
  {
    g_print("Warning gp or gp->cpu == NULL in TraceWindow_update");
    return;
  }

  trace_flags |= GTF_ENABLE_XREF_UPDATES;
  if (get_cycles().get() - last_cycle >= MAXTRACES)
    // redraw the whole thing
    get_trace().dump(MAXTRACES, 0);
  else
    get_trace().dump(get_cycles().get() - last_cycle, 0);


  trace_flags &= ~GTF_ENABLE_XREF_UPDATES;
  last_cycle = get_cycles().get();
}


/*****************************************************************
 * TraceWindow_new_processor
 *
 *
 */

void Trace_Window::NewProcessor(GUI_Processor *)
{
#define NAME_SIZE 32

  TraceXREF *cross_reference;

  if (!gp)
    return;

  if (!enabled)
    return;

  cross_reference = new TraceXREF();
  cross_reference->parent_window = (gpointer) this;
  cross_reference->data = 0;
  if (get_trace().xref)
    get_trace().xref->_add((gpointer) cross_reference);
}

void Trace_Window::cycle_cell_data_function(
  GtkTreeViewColumn *,
  GtkCellRenderer *renderer,
  GtkTreeModel *model,
  GtkTreeIter *iter,
  gpointer )
{
  guint64 cycle;
  gchar buf[TRACE_STRING];

  gtk_tree_model_get(model, iter, gint(CYCLE_COLUMN), &cycle, -1);
  g_snprintf(buf, sizeof(buf),"0x%016" PRINTF_GINT64_MODIFIER "x", cycle);
  g_object_set(renderer, "text", buf, nullptr);
}

void Trace_Window::Build()
{
  if (bIsBuilt)
    return;

  GtkWidget *main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(window), main_vbox);

  gtk_window_set_title(GTK_WINDOW(window), "trace viewer");

  // Trace list
  trace_list = gtk_list_store_new(N_COLUMNS, G_TYPE_UINT64, G_TYPE_STRING);
  GtkWidget *trace_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(trace_list));
  g_object_unref(trace_list);

  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes("Cycle", renderer,
    "text", CYCLE_COLUMN,
    nullptr);
  gtk_tree_view_column_set_cell_data_func(col, renderer,
    Trace_Window::cycle_cell_data_function, nullptr, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(trace_view), col);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes("Trace", renderer,
    "text", gint(TRACE_COLUMN),
    nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(trace_view), col);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(0, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(trace_view));

  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);

  gtk_widget_show_all(window);

  if (!trace_map) {
    trace_map = new TraceMapping[MAXTRACES];
    trace_map_index = 0;
  }

  enabled = 1;
  bIsBuilt = true;
  last_cycle = 0;

  NewProcessor(gp);

  Update();
  UpdateMenuItem();
}

//------------------------------------------------------------------------
//
//
//

Trace_Window::Trace_Window(GUI_Processor *_gp)
  : GUI_Object("trace"),
    trace_flags(0), trace_map(nullptr)
{
  menu = "/menu/Windows/Trace";

  gp = _gp;

  if (enabled)
      Build();
}

#endif // HAVE_GUI
