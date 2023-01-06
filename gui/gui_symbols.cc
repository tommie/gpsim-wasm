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

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <typeinfo>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib-object.h>

#include "../src/gpsim_object.h"
#include "../src/pic-instructions.h"
#include "../src/registers.h"
#include "../src/symbol.h"
#include "../src/ui.h"
#include "../src/value.h"

#include "gui.h"
#include "gui_regwin.h"
#include "gui_symbols.h"
#include "gui_watch.h"
#include "gui_processor.h"
#include "gui_src.h"

// TODO: Use a GtkTreeModelSort so that the view can return back to an unsorted
// view. Use our own Cell Renderer to display Value. This would have less data
// to store and variable data can then use the most upto date values.

typedef enum {
  MENU_ADD_WATCH,
} menu_id;


typedef struct _menu_item {
  const char *name;
  menu_id id;
  GtkWidget *item;
} menu_item;


static menu_item menu_items[] = {
  {"Add to watch window", MENU_ADD_WATCH, nullptr},
};


// Used only in popup menus
Symbol_Window *popup_sw;

static void update_menus(Symbol_Window *sw)
{
  gboolean r = gtk_tree_selection_get_selected(
                 gtk_tree_view_get_selection(GTK_TREE_VIEW(sw->symbol_view)), nullptr, nullptr);

  for (size_t i = 0; i < (sizeof(menu_items) / sizeof(menu_items[0])) ; ++i) {
    GtkWidget *item = menu_items[i].item;

    if (r) {
      gtk_widget_set_sensitive(item, TRUE);

    } else {
      gtk_widget_set_sensitive(item, FALSE);
    }
  }
}


// called when user has selected a menu item
static void popup_activated(GtkWidget *, gpointer data)
{
  menu_item *item = (menu_item *)data;
  GtkTreeIter iter;
  GtkTreeSelection *sel
    = gtk_tree_view_get_selection(GTK_TREE_VIEW(popup_sw->symbol_view));

  if (!gtk_tree_selection_get_selected(sel, nullptr, &iter)) {
    return;
  }

  Value *entry;
  gtk_tree_model_get(GTK_TREE_MODEL(popup_sw->symbol_list), &iter, 3, &entry, -1);

  if (!entry) {
    return;
  }

  switch (item->id) {
  case MENU_ADD_WATCH: {
    popup_sw->gp->watch_window->Add(entry);
  }
  break;

  default:
    puts("Unhandled menuitem?");
    break;
  }
}


// helper function, called from do_popup
GtkWidget *Symbol_Window::build_menu(GtkWidget *)
{
  popup_sw = this;
  GtkWidget *menu = gtk_menu_new();

  for (size_t i = 0; i < G_N_ELEMENTS(menu_items); ++i) {
    GtkWidget *item = gtk_menu_item_new_with_label(menu_items[i].name);
    menu_items[i].item = item ;
    g_signal_connect(item, "activate",
                     G_CALLBACK(popup_activated),
                     &menu_items[i]);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  return menu;
}


// button press handler
gboolean Symbol_Window::do_popup(GtkWidget *, GdkEventButton *event, Symbol_Window *sw)
{
  GtkWidget *popup = sw->popup_menu;

  if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
    update_menus(sw);
    gtk_menu_popup(GTK_MENU(popup), nullptr, nullptr, nullptr, nullptr,
                   3, event->time);
    return TRUE;
  }

  return FALSE;
}


static Symbol_Window *lpSW = nullptr;
static std::string table;

void updateOneSymbol(const SymbolEntry_t &sym)
{
  Value *pVal = dynamic_cast<Value *>(sym.second);

  if (lpSW && pVal) {
    Register *pReg = dynamic_cast<Register *>(pVal);

    if ((typeid(*pVal) == typeid(LineNumberSymbol)) |
        (lpSW->filter_addresses && (typeid(*pVal) == typeid(AddressSymbol)))  ||
        (lpSW->filter_constants && (typeid(*pVal) == typeid(Integer))) ||
        (lpSW->filter_constants && (typeid(*pVal) == typeid(Boolean))) ||
        (lpSW->filter_registers && (pReg))) {
      return;
    }

#define SYM_LEN 32
    std::string symbol_name;
    GtkTreeIter iter;
    char value[SYM_LEN];

    if (table != "__global__") {
      symbol_name = table + "." + pVal->name();

    } else {
      symbol_name = pVal->name();
    }

    if (pReg) {
      g_snprintf(value, sizeof(value), "%02x / %d (0x%02x)",
                 pReg->getAddress(), pReg->get_value(), pReg->get_value());

    } else {
      pVal->get(value, sizeof(value));
    }

    char *pLF = strchr(value, '\n');

    if (pLF) {
      *pLF = 0;
    }

    gtk_list_store_append(lpSW->symbol_list, &iter);
    gtk_list_store_set(lpSW->symbol_list, &iter,
                       0, symbol_name.c_str(),
                       1, pVal->showType().c_str(),
                       2, value,
                       3, pVal,
                       -1);
  }
}


static void updateSymbolTables(const SymbolTableEntry_t &st)
{
  if (verbose) {
    std::cout << " gui Symbol Window: " << st.first << '\n';
  }

  table = st.first;
  (st.second)->ForEachSymbolTable(updateOneSymbol);
}


void Symbol_Window::Update()
{
  load_symbols = 1;

  if (!enabled) {
    return;
  }

  gtk_list_store_clear(symbol_list);
  lpSW = this;
  globalSymbolTable().ForEachModule(updateSymbolTables);
  lpSW = nullptr;
}


void Symbol_Window::do_symbol_select(Value *e)
{
  if (!gp) {
    return;
  }

  // Do what is to be done when a symbol is selected.
  // Except for selecting the symbol row in the symbol_clist

  if (typeid(*e) == typeid(LineNumberSymbol) ||
      typeid(*e) == typeid(AddressSymbol)) {
    if (gp->source_browser) {
      gp->source_browser->SelectAddress(e);
    }

    if (gp->program_memory) {
      gp->program_memory->SelectAddress(e);
    }

  } else if (typeid(*e) == typeid(Register))
    if (gp->regwin_ram) {
      gp->regwin_ram->SelectRegister(e);
    }
}


void Symbol_Window::symbol_list_row_selected(GtkTreeSelection *treeselection,
    gpointer user_data)
{
  Symbol_Window *sw = static_cast<Symbol_Window *>(user_data);
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected(treeselection, nullptr, &iter)) {
    return;
  }

  Value *e;
  gtk_tree_model_get(GTK_TREE_MODEL(sw->symbol_list), &iter, 3, &e, -1);
  sw->do_symbol_select(e);
}


void Symbol_Window::NewSymbols()
{
  Update();
}


void Symbol_Window::toggle_addresses(GtkToggleButton *, Symbol_Window *sw)
{
  sw->filter_addresses = !sw->filter_addresses;
  config_set_variable(sw->name(), "filter_addresses", sw->filter_addresses);
  sw->Update();
}


void Symbol_Window::toggle_constants(GtkToggleButton *, Symbol_Window *sw)
{
  sw->filter_constants = !sw->filter_constants;
  config_set_variable(sw->name(), "filter_constants", sw->filter_constants);
  sw->Update();
}


void Symbol_Window::toggle_registers(GtkToggleButton *, Symbol_Window *sw)
{
  sw->filter_registers = !sw->filter_registers;
  config_set_variable(sw->name(), "filter_registers", sw->filter_registers);
  sw->Update();
}


//------------------------------------------------------------------------
// Build
//

void Symbol_Window::Build()
{
  if (bIsBuilt) {
    return;
  }

  GtkWidget *vbox;
  GtkWidget *scrolled_window;
  GtkWidget *hbox;
  gtk_window_set_title(GTK_WINDOW(window), "Symbol Viewer");
  symbol_list = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_POINTER);
  symbol_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(symbol_list));
  g_object_unref(symbol_list);
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Name",
           renderer, "text", 0, nullptr);
  gtk_tree_view_column_set_sort_indicator(column, TRUE);
  gtk_tree_view_column_set_sort_column_id(column, 0);
  gtk_tree_view_append_column(GTK_TREE_VIEW(symbol_view), column);
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Type",
           renderer, "text", 1, nullptr);
  gtk_tree_view_column_set_sort_indicator(column, TRUE);
  gtk_tree_view_column_set_sort_column_id(column, 1);
  gtk_tree_view_append_column(GTK_TREE_VIEW(symbol_view), column);
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Address/Value",
           renderer, "text", 2, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(symbol_view), column);
  g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(symbol_view)),
                   "changed", G_CALLBACK(symbol_list_row_selected), this);
  g_signal_connect(symbol_view,
                   "button_press_event",
                   G_CALLBACK(do_popup),
                   this);
  scrolled_window = gtk_scrolled_window_new(0, 0);
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(scrolled_window), symbol_view);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
  addressesbutton = gtk_check_button_new_with_label("addresses");
  gtk_box_pack_start(GTK_BOX(hbox), addressesbutton, TRUE, TRUE, 5);

  if (filter_addresses) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(addressesbutton), FALSE);

  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(addressesbutton), TRUE);
  }

  g_signal_connect(addressesbutton, "toggled",
                   G_CALLBACK(toggle_addresses), (gpointer)this);
  constantsbutton = gtk_check_button_new_with_label("constants");
  gtk_box_pack_start(GTK_BOX(hbox), constantsbutton, TRUE, TRUE, 5);

  if (filter_constants) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(constantsbutton), FALSE);

  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(constantsbutton), TRUE);
  }

  g_signal_connect(constantsbutton, "toggled",
                   G_CALLBACK(toggle_constants), (gpointer)this);
  registersbutton = gtk_check_button_new_with_label("registers");
  gtk_box_pack_start(GTK_BOX(hbox), registersbutton, TRUE, TRUE, 5);

  if (filter_registers) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(registersbutton), FALSE);

  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(registersbutton), TRUE);
  }

  g_signal_connect(registersbutton, "toggled",
                   G_CALLBACK(toggle_registers), (gpointer)this);
  gtk_widget_show_all(window);
  bIsBuilt = true;

  if (load_symbols) {
    Update();
  }

  UpdateMenuItem();
  popup_menu = build_menu(window);
}


Symbol_Window::Symbol_Window(GUI_Processor *_gp)
  : GUI_Object("symbol_viewer"),
    filter_addresses(0), filter_constants(1), filter_registers(0),
    load_symbols(0)
{
  menu = "/menu/Windows/Symbols";
  gp = _gp;
  config_get_variable(name(), "filter_addresses", &filter_addresses);
  config_get_variable(name(), "filter_constants", &filter_constants);
  config_get_variable(name(), "filter_registers", &filter_registers);

  if (enabled) {
    Build();
  }
}

#endif // HAVE_GUI
