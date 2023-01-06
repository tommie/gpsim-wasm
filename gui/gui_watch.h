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

#ifndef GUI_GUI_WATCH_H_
#define GUI_GUI_WATCH_H_

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <vector>

#include "../src/interface.h"
#include "gui_object.h"
#include "gui_register.h"

class GUI_Processor;
class Processor;
class Register;

class WatchEntry : public GUIRegister {
public:
  WatchEntry(REGISTER_TYPE _type, Register *_pRegister);
  virtual ~WatchEntry();

  Processor *cpu;
  REGISTER_TYPE type;
  //register_symbol *pRegSymbol;
  Register *pRegister;
};


class Value;
class ColumnData;

//
// The watch window
//
class Watch_Window : public  GUI_Object
{
public:
  GtkListStore *watch_list;
  GtkWidget *watch_tree;

  explicit Watch_Window(GUI_Processor *gp);
  void Build() override;
  virtual void ClearWatch(GtkTreeIter *iter);
  virtual void UpdateWatch(GtkTreeIter *iter);

  virtual void Add(REGISTER_TYPE type, GUIRegister *reg, Register *pReg = nullptr);
  virtual void Add(Value *);
  void Update() override;
  virtual void UpdateMenus();
  void NewProcessor(GUI_Processor *gp) override;

  // Override set_config() to save variable list on app exit
  int set_config() override;
  void         ReadSymbolList();
  void         WriteSymbolList();
  void         DeleteSymbolList();

private:
  void build_menu();

  static void popup_activated(GtkWidget *widget, gpointer data);
  static gboolean do_popup(GtkWidget *widget, GdkEventButton *event,
    Watch_Window *ww);
  static void set_column(GtkCheckButton *button, Watch_Window *ww);
  void select_columns();
  static gboolean do_symbol_write(GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer data);
  static void watch_list_row_selected(GtkTreeSelection *sel, Watch_Window *ww);

  int count;

  GtkWidget *popup_menu;
  std::vector<GtkWidget *> popup_items;
  std::vector<ColumnData> coldata;
};


#endif // GUI_GUI_WATCH_H_
