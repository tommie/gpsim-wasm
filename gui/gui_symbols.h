/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
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

#ifndef GUI_GUI_SYMBOLS_H_
#define GUI_GUI_SYMBOLS_H_

#include "gui_object.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

class GUI_Processor;
class Value;

//
// The symbol window
//
class Symbol_Window : public GUI_Object {
public:
  GtkWidget *symbol_view;
  GtkListStore *symbol_list;

  GtkWidget *popup_menu;

  int filter_addresses;
  int filter_constants;
  int filter_registers;

  GtkWidget *addressesbutton;
  GtkWidget *constantsbutton;
  GtkWidget *registersbutton;

  int load_symbols;

  explicit Symbol_Window(GUI_Processor *gp);
  void Build() override;
  void Update() override;
  void NewSymbols();

protected:
  // Signals
  static void toggle_addresses(GtkToggleButton *button, Symbol_Window *sw);
  static void toggle_constants(GtkToggleButton *button, Symbol_Window *sw);
  static void toggle_registers(GtkToggleButton *button, Symbol_Window *sw);
  static gboolean do_popup(GtkWidget *widget, GdkEventButton *event, Symbol_Window *sw);
  static void symbol_list_row_selected(GtkTreeSelection *treeselection,
                                       gpointer user_data);
  static gboolean delete_event(GtkWidget *widget, GdkEvent  *event,
                               Symbol_Window *sw);

  GtkWidget *build_menu(GtkWidget *sheet);
  void do_symbol_select(Value *e);
};


#endif // GUI_GUI_SYMBOLS_H_

