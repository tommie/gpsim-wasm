/*
   Copyright (C) 2000,2001
    Ralf Forsberg
   Copyright (c) 2013 Roy R Rankin

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

#include <cassert>
#include <cstring>
#include <string>

#include "../src/14bit-registers.h"
#include "../src/pic-processor.h"
#include "../src/processor.h"
#include "../src/ui.h"
#include "../src/value.h"

#include "gui_processor.h"
#include "gui_stack.h"
#include "gui_src.h"

enum {
  COLUMN_DEPTH,
  COLUMN_RETADDRESS,
  N_COLUMNS
};

#define COLUMNS 2

/*
 */
static void sigh_button_event(
  GtkTreeView *treeview,
  GtkTreePath *path,
  GtkTreeViewColumn *,
  Stack_Window *sw)
{
  assert(sw);

  if(!sw->gp || !sw->gp->cpu)
    return;

  GtkTreeModel *model = gtk_tree_view_get_model(treeview);
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gint retaddress;
    gtk_tree_model_get(model, &iter, gint(COLUMN_RETADDRESS), &retaddress, -1);
    sw->gp->cpu->pma->toggle_break_at_address(retaddress);
  }
}

static void stack_list_row_selected(GtkTreeSelection *selection, Stack_Window *sw)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gint retaddress;
    gtk_tree_model_get(model, &iter, gint(COLUMN_RETADDRESS), &retaddress, -1);

    sw->gp->source_browser->SelectAddress(retaddress);
    sw->gp->program_memory->SelectAddress(retaddress);
  }
}

// find name of label closest before 'address' and copy found data
// into name and offset
static int get_closest_label(Stack_Window *,
                              int address,
                              char *name,
                              int *offset)
{

  // this function assumes that address symbols are sorted

  Value *closest_symbol = nullptr;

#if 0
  int minimum_delta=0x2000000;
  int delta;

  Symbol_Table &st = CSimulationContext::GetContext()->GetSymbolTable();
  Symbol_Table::iterator symIt;
  Symbol_Table::iterator symItEnd = st.end();

  for(symIt=st.begin(); symIt != symItEnd; symIt++) {

    Value *s = *symIt;

    if(  (typeid(*s) == typeid(AddressSymbol)) /*||
         (typeid(*s) == typeid(line_number_symbol))*/) {
      int i;
      s->get(i);
      delta = abs( i - address);
      if(delta < minimum_delta) {
        minimum_delta = delta;
        closest_symbol = s;
      }
    }
  }
#endif
  if (verbose)
    g_print("FIXME gui_stack.cc get closest label\n");
  if (closest_symbol) {
    strcpy(name, closest_symbol->name().data());
    int i;
    closest_symbol->get(i);
    *offset = address - i;
    return 1;
  }

  return 0;
}

void Stack_Window::Update()
{
  int nrofentries;

  if (!gp || !enabled)
    return;

  pic_processor *pic = dynamic_cast<pic_processor *>(gp->cpu);
  if (!pic)
    return;

  // Enhanced 14bit processors use 0x1f(31) for empty index which is
  // also P18 max stack slot, so we mask with 0x1f
  nrofentries = pic->stack->pointer & 0x1f;
  if (nrofentries && ((nrofentries - 1) > (int)pic->stack->stack_mask))
    return;

  // Do if stack has shrunk
  for ( ; last_stacklen > nrofentries; --last_stacklen) {
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(stack_list), &iter)) {
      gtk_list_store_remove(stack_list, &iter);
    }
  }

  // Do if stack has grown
  for ( ; last_stacklen < nrofentries; ++last_stacklen) {
    GtkTreeIter iter;
    gint retaddress = pic->stack->contents[last_stacklen & pic->stack->stack_mask];

    gtk_list_store_prepend(stack_list, &iter);
    gtk_list_store_set(stack_list, &iter,
      gint(COLUMN_DEPTH), last_stacklen,
      gint(COLUMN_RETADDRESS), retaddress,
      -1);
  }
}

static void depth_cell_data_function(
  GtkTreeViewColumn *,
  GtkCellRenderer *renderer,
  GtkTreeModel *model,
  GtkTreeIter *iter,
  gpointer)
{
  gint depth;
  gchar buf[64];

  gtk_tree_model_get(model, iter, COLUMN_DEPTH, &depth, -1);
  g_snprintf(buf, sizeof(buf), "#%d", depth);
  g_object_set(renderer, "text", buf, nullptr);
}

static void retaddr_cell_data_function(
  GtkTreeViewColumn *,
  GtkCellRenderer *renderer,
  GtkTreeModel *model,
  GtkTreeIter *iter,
  gpointer user_data)
{
  gint addr;
  gchar buf[64];
  char labelname[64];
  int labeloffset;
  Stack_Window *sw;
  sw = static_cast<Stack_Window *>(user_data);

  gtk_tree_model_get(model, iter, COLUMN_RETADDRESS, &addr, -1);
  if (get_closest_label(sw, addr, labelname, &labeloffset))
    g_snprintf(buf, sizeof(buf), "0x%04x (%s+%d)", addr, labelname, labeloffset);
  else
    g_snprintf(buf, sizeof(buf), "0x%04x", addr);

  g_object_set(renderer, "text", buf, nullptr);
}

void Stack_Window::Build()
{
  if (bIsBuilt)
    return;

  GtkWidget *vbox;
  GtkWidget *scrolled_window;

  gtk_window_set_title(GTK_WINDOW(window), "Stack Viewer");

  g_signal_connect(window, "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &window);

  stack_list = gtk_list_store_new(gint(N_COLUMNS), G_TYPE_INT, G_TYPE_INT);
  sort_stack_list = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(stack_list));
  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sort_stack_list));
  g_object_unref(stack_list);
  g_object_unref(sort_stack_list);

  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "depth");
  gtk_tree_view_column_set_sort_indicator(col, TRUE);
  gtk_tree_view_column_set_sort_column_id(col, gint(COLUMN_DEPTH));
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func(col, renderer, depth_cell_data_function, nullptr, nullptr);

  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "return address");
  gtk_tree_view_column_set_sort_indicator(col, TRUE);
  gtk_tree_view_column_set_sort_column_id(col, gint(COLUMN_RETADDRESS));
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func(col, renderer, retaddr_cell_data_function, this, nullptr);

  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);

  g_signal_connect(selection, "changed", G_CALLBACK(stack_list_row_selected), this);
  g_signal_connect(tree, "row-activated", G_CALLBACK(sigh_button_event), this);

  scrolled_window = gtk_scrolled_window_new(0, 0);

  vbox = gtk_vbox_new(FALSE, 1);

  gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

  gtk_widget_show_all(window);


  bIsBuilt = true;

  UpdateMenuItem();

  Update();
}

//------------------------------------------------------------------------
// Create
//
//

Stack_Window::Stack_Window(GUI_Processor *_gp)
  : GUI_Object("stack_viewer"), last_stacklen(0)
{
  menu = "/menu/Windows/Stack";

  gp = _gp;

  if (enabled)
    Build();
}

#endif // HAVE_GUI
