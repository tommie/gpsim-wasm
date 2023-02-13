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

#include <config.h>
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib-object.h>

#include <cstdio>

#include <memory>
#include <string>

#include "../src/breakpoints.h"
#include "../src/interface.h"
#include "../src/processor.h"
#include "../src/registers.h"
#include "../src/symbol.h"
#include "../src/ui.h"
#include "../src/value.h"
#include "gui.h"
#include "gui_processor.h"
#include "gui_register.h"
#include "gui_regwin.h"
#include "gui_watch.h"

class gpsimObject;

enum {
  NAMECOL,
  ADDRESSCOL,
  DECIMALCOL,
  HEXCOL,
  ASCIICOL,
  BITCOL,
  ENTRYCOL,
  N_COLUMNS
};

static const gchar *watch_titles[]={"name","address","dec","hex","ascii","bits"};

#define COLUMNS sizeof(watch_titles)/sizeof(char*)

class ColumnData {
public:
  ColumnData(GtkTreeViewColumn *_tc, bool visible);
  void SetVisibility(bool bVisibility);
  bool GetVisibility() { return isVisible;}

private:
  GtkTreeViewColumn *tc;
  bool isVisible;
};

typedef enum {
    MENU_REMOVE,
    MENU_SET_VALUE,
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_COLUMNS,
} menu_id;


typedef struct _menu_item {
    const char *name;
    menu_id id;
} menu_item;

static const menu_item menu_items[] = {
    {"Remove watch", MENU_REMOVE},
    {"Set value...", MENU_SET_VALUE},
    {"Clear breakpoints", MENU_BREAK_CLEAR},
    {"Set break on read", MENU_BREAK_READ},
    {"Set break on write", MENU_BREAK_WRITE},
    {"Set break on read value...", MENU_BREAK_READ_VALUE},
    {"Set break on write value...", MENU_BREAK_WRITE_VALUE},
    {"Columns...", MENU_COLUMNS},
};

//========================================================================

class WatchWindowXREF : public CrossReferenceToGUI
{
public:
  virtual ~WatchWindowXREF();
  void Update(int new_value) override;
};

WatchWindowXREF::~WatchWindowXREF()
{
  gtk_tree_row_reference_free(static_cast<GtkTreeRowReference *>((void *)data));
}

void WatchWindowXREF::Update(int)
{
  Watch_Window *ww  = static_cast<Watch_Window *>(parent_window);
  if (ww) {
    GtkTreePath *path
      = gtk_tree_row_reference_get_path(static_cast<GtkTreeRowReference *>((void *)data));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(ww->watch_list), &iter, path))
      ww->UpdateWatch(&iter);
  }
}

//========================================================================
WatchEntry::WatchEntry(REGISTER_TYPE _type, Register *_pRegister)
  : cpu(nullptr), type(_type), pRegister(_pRegister)
{
}

WatchEntry::~WatchEntry()
{
  Clear_xref();
}

//========================================================================
ColumnData::ColumnData(GtkTreeViewColumn *_tc, bool visible)
  : tc(_tc), isVisible(visible)
{
  int show = isVisible;
  gtk_tree_view_column_set_visible(tc, show);
}

void ColumnData::SetVisibility(bool bVisibility)
{
  isVisible = bVisibility;
  int show = isVisible;
  gtk_tree_view_column_set_visible(tc, show);
}

//========================================================================

void Watch_Window::ClearWatch(GtkTreeIter *iter)
{
  WatchEntry *entry;

  gtk_tree_model_get(GTK_TREE_MODEL(watch_list), iter, ENTRYCOL, &entry, -1);
  delete entry;
  gtk_list_store_remove(watch_list, iter);
}

void Watch_Window::UpdateMenus()
{
  GtkTreeSelection *sel =
    gtk_tree_view_get_selection(GTK_TREE_VIEW(watch_tree));
  GtkTreeIter iter;
  gboolean selected = gtk_tree_selection_get_selected(sel, nullptr, &iter);
  WatchEntry *entry;
  if (selected) {
    gtk_tree_model_get(GTK_TREE_MODEL(watch_list), &iter, ENTRYCOL, &entry, -1);
  }

  for (size_t i = 0; i < G_N_ELEMENTS(menu_items); ++i) {
    GtkWidget *item = popup_items[i];
    if (menu_items[i].id != MENU_COLUMNS) {
      if (menu_items[i].id !=MENU_COLUMNS &&
          (!selected ||
          (entry->type == REGISTER_EEPROM && menu_items[i].id == MENU_BREAK_CLEAR) ||
          (entry->type == REGISTER_EEPROM && menu_items[i].id == MENU_BREAK_READ) ||
          (entry->type == REGISTER_EEPROM && menu_items[i].id == MENU_BREAK_WRITE) ||
          (entry->type == REGISTER_EEPROM && menu_items[i].id == MENU_BREAK_READ_VALUE) ||
          (entry->type == REGISTER_EEPROM && menu_items[i].id == MENU_BREAK_WRITE_VALUE)
          ))
        gtk_widget_set_sensitive(item, FALSE);
      else
        gtk_widget_set_sensitive(item, TRUE);
    }
  }
}

int Watch_Window::set_config()
{
  int iRet = GUI_Object::set_config();
  WriteSymbolList();
  return iRet;
}

gboolean Watch_Window::do_symbol_write(GtkTreeModel *, GtkTreePath *,
  GtkTreeIter *iter, gpointer data)
{
  Watch_Window *ww = static_cast<Watch_Window *>(data);
  WatchEntry *entry;
  gtk_tree_model_get(GTK_TREE_MODEL(ww->watch_list), iter, ENTRYCOL, &entry, -1);

  if (entry && entry->pRegister) {
    char cwv[100];
    g_snprintf(cwv, sizeof(cwv), "WV%d", ww->count);
    config_set_string(ww->name(), cwv, entry->pRegister->name().c_str());
  }

  ww->count += 1;

  return FALSE;
}

void Watch_Window::WriteSymbolList()
{
  // delete previous list
  DeleteSymbolList();
  // write the current list
  count = 0;
  if (watch_list)
      gtk_tree_model_foreach(GTK_TREE_MODEL(watch_list), do_symbol_write, this);
}

void Watch_Window::DeleteSymbolList()
{
  char cwv[100];
  for (int i = 0; i < 1000; ++i) {
    g_snprintf(cwv, sizeof(cwv), "WV%d", i);
    if (config_remove(name(), cwv) == 0) {
      break;
    }
  }
}

void Watch_Window::ReadSymbolList()
{
  // now read symbols watched from a prior simulation session
  char cwv[100];
  char *vname;
  for (int i = 0; i < 1000; ++i) {
    g_snprintf(cwv, sizeof(cwv), "WV%d", i);
    vname = 0;
    if (config_get_string(name(), cwv, &vname) ) {
      Value *val = globalSymbolTable().findValue(vname);
      if (val)
        Add(val);
    }
    else
      break;
  }
}

// called when user has selected a menu item
void Watch_Window::popup_activated(GtkWidget *widget, gpointer data)
{
  Watch_Window *ww = static_cast<Watch_Window *>(data);
  WatchEntry *entry = nullptr;

  int value;

  GtkTreeSelection *sel
    = gtk_tree_view_get_selection(GTK_TREE_VIEW(ww->watch_tree));
  GtkTreeIter iter;
  gboolean selected = gtk_tree_selection_get_selected(sel, nullptr, &iter);

  if (selected) {
    gtk_tree_model_get(GTK_TREE_MODEL(ww->watch_list), &iter, ENTRYCOL, &entry, -1);
  }

  int id = GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(widget), "id"));

  if (id == MENU_COLUMNS) {
    ww->select_columns();
    return;
  }

  if (!entry || !entry->cpu)
    return;

  switch (id)
    {
    case MENU_REMOVE:
      ww->ClearWatch(&iter);
      break;
    case MENU_SET_VALUE:
      value = gui_get_value("value:");
      if (value < 0)
        break; // Cancel
      entry->put_value(value);
      break;
    case MENU_BREAK_READ:
      get_bp().set_read_break(entry->cpu,entry->address);
      break;
    case MENU_BREAK_WRITE:
      get_bp().set_write_break(entry->cpu,entry->address);
      break;
    case MENU_BREAK_READ_VALUE:
      value = gui_get_value("value to read for breakpoint:");
      if (value < 0)
        break; // Cancel
      get_bp().set_read_value_break(entry->cpu,entry->address,value);
      break;
    case MENU_BREAK_WRITE_VALUE:
      value = gui_get_value("value to write for breakpoint:");
      if (value < 0)
        break; // Cancel
      get_bp().set_write_value_break(entry->cpu,entry->address,value);
      break;
    case MENU_BREAK_CLEAR:
      get_bp().clear_all_register(entry->cpu,entry->address);
      break;
    default:
      break;
    }
}

//------------------------------------------------------------
// call back function to toggle column visibility in the configuration popup
void Watch_Window::set_column(GtkCheckButton *button, Watch_Window *ww)
{
  int id = GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(button), "id"));
  gboolean vis = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

  ww->coldata[id].SetVisibility(vis != 0);

  config_set_variable(ww->name(), watch_titles[id], vis);
}

void Watch_Window::select_columns()
{
  GtkWidget *dialog;

  dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(window),
    GTK_DIALOG_MODAL,
    "_Close", GTK_RESPONSE_CLOSE,
    nullptr);

  GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 30);

  for (size_t i = 0; i < COLUMNS; ++i) {
    GtkWidget *button = gtk_check_button_new_with_label(watch_titles[i]);
    g_object_set_data(G_OBJECT(button), "id", GSIZE_TO_POINTER(i));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
      coldata[i].GetVisibility());
    gtk_box_pack_start(GTK_BOX(area), button, FALSE, FALSE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(set_column), this);
  }

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

// helper function, called from do_popup
void Watch_Window::build_menu()
{
  GtkWidget *menu = gtk_menu_new();
  popup_menu = menu;

  popup_items.reserve(G_N_ELEMENTS(menu_items));
  for (size_t i = 0; i < G_N_ELEMENTS(menu_items); ++i) {
    GtkWidget *item = gtk_menu_item_new_with_label(menu_items[i].name);
    popup_items.push_back(item);
    g_object_set_data(G_OBJECT(item), "id", GSIZE_TO_POINTER(i));

    g_signal_connect(item, "activate", G_CALLBACK(popup_activated), this);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  UpdateMenus();
}

// button press handler
gboolean Watch_Window::do_popup(GtkWidget *, GdkEventButton *event,
  Watch_Window *ww)
{
  GtkWidget *popup = ww->popup_menu;

  if ((event->type == GDK_BUTTON_PRESS) &&  (event->button == 3)) {
    gtk_menu_popup(GTK_MENU(popup), nullptr, nullptr, nullptr, nullptr, 3, event->time);
    return TRUE;
  }

  return FALSE;
}

static gint
key_press(GtkWidget *,
          GdkEventKey *key,
          gpointer data)
{
  Watch_Window *ww = static_cast<Watch_Window *>(data);

  if (!ww || !ww->gp || !ww->gp->cpu) return FALSE;

  switch (key->keyval) {

  case GDK_KEY_Delete:
    GtkTreeSelection *sel
      = gtk_tree_view_get_selection(GTK_TREE_VIEW(ww->watch_tree));
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, nullptr, &iter))
      ww->ClearWatch(&iter);
    break;
  }
  return TRUE;
}

void Watch_Window::watch_list_row_selected(GtkTreeSelection *sel, Watch_Window *ww)
{
  WatchEntry *entry;
  GUI_Processor *gp = ww->gp;

  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected(sel, nullptr, &iter))
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(ww->watch_list), &iter, ENTRYCOL, &entry, -1);

  if (entry->type == REGISTER_RAM)
    gp->regwin_ram->SelectRegister(entry->address);
  else if (entry->type == REGISTER_EEPROM)
    gp->regwin_eeprom->SelectRegister(entry->address);

  ww->UpdateMenus();
}

//========================================================================
// UpdateWatch
// A single watch entry is updated here. Here's what's done:
//
// If the value has not changed since the last update, then the
// foreground and background colors are refreshed and we return.
//
// If the value has changed, then each of the value fields are refreshed.
// Then the foreground and background are refreshed.

void Watch_Window::UpdateWatch(GtkTreeIter *iter)
{
  WatchEntry *entry;
  gtk_tree_model_get(GTK_TREE_MODEL(watch_list), iter, ENTRYCOL, &entry, -1);

  RegisterValue rvNewValue = entry->getRV();

  // If the value has not changed, then simply update the foreground and background
  // colors and return.

  if (entry->get_shadow() == rvNewValue) {
    // Disable colour change for now
    return;
  }

  RegisterValue rvMaskedNewValue;
  unsigned int uBitmask;

  entry->put_shadow(rvNewValue);

  if (entry->pRegister) {
    rvMaskedNewValue = entry->pRegister->getRV_notrace();
    uBitmask = entry->pRegister->mValidBits; //getBitmask();
  }
  else {
    rvMaskedNewValue = entry->getRV();
    uBitmask = entry->cpu->register_mask();
  }

  char str[80] = "?";

  if (!(rvNewValue.init & uBitmask))
    g_snprintf(str, sizeof(str), "%d", rvNewValue.data);

  // Hexadecimal representation:
  char hStr[80];
  rvMaskedNewValue.toString(hStr, 80);

  // ASCII representation
  char aStr[2];
  aStr[0] = ( rvNewValue.data>0x20 && rvNewValue.data<0x7F ) ? rvNewValue.data : 0;
  aStr[1] = 0;

  gtk_list_store_set(watch_list, iter,
    DECIMALCOL, str,
    HEXCOL, hStr,
    ASCIICOL, aStr,
    -1);

  // Set foreground and background colors
  // Diable colour change for now
}

//------------------------------------------------------------------------
// Update
//
//

static gboolean do_an_update(GtkTreeModel *, GtkTreePath *,
  GtkTreeIter *iter, gpointer data)
{
  Watch_Window *ww = static_cast<Watch_Window *>(data);
  ww->UpdateWatch(iter);

  return FALSE;
}

void Watch_Window::Update()
{
  if (watch_list)
      gtk_tree_model_foreach(GTK_TREE_MODEL(watch_list), do_an_update, this);
}

void Watch_Window::Add(REGISTER_TYPE type, GUIRegister *reg, Register * pReg)
{
  if (!gp || !gp->cpu || !reg || !reg->bIsValid())
    return;

  if (!enabled)
    Build();

  pReg = pReg ? pReg : reg->get_register();
  if (!pReg)
    return;

  unsigned int uAddrMask = 0;
  unsigned int uLastAddr = gp->cpu->register_memory_size() - 1;
  while (uLastAddr) {
    uLastAddr >>= 4;
    uAddrMask <<= 4;
    uAddrMask |= 0xf;
  }

  WatchEntry *watch_entry = new WatchEntry(type, pReg);
  watch_entry->address = reg->address;
  watch_entry->cpu = gp->cpu;
  watch_entry->rma = reg->rma;

  GtkTreeIter iter;
  gtk_list_store_append(watch_list, &iter);
  gtk_list_store_set(watch_list, &iter,
    NAMECOL, pReg ? pReg->name().c_str() : "NULLREG",
    ADDRESSCOL, GetUserInterface().FormatProgramAddress(pReg->getAddress(),
      uAddrMask, IUserInterface::eHex),
    ENTRYCOL, gpointer(watch_entry),
    -1);

  UpdateWatch(&iter);

  GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(watch_list), &iter);
  WatchWindowXREF *cross_reference = new WatchWindowXREF();
  cross_reference->parent_window = (gpointer) this;
  cross_reference->data =
    (gpsimObject *) gtk_tree_row_reference_new(GTK_TREE_MODEL(watch_list), path);
  gtk_tree_path_free(path);

  watch_entry->Assign_xref(cross_reference);

  UpdateMenus();
}

//---
// Add - given a symbol, verify that it is a RAM or EEPROM register
// symbol. If it is then extract the register and use it's address
// to get the GUI representation of the register.

void Watch_Window::Add( Value *regSym)
{
  if (regSym && gp) {

    Register *reg = dynamic_cast<Register *>(regSym);

    if (reg)
    {
      GUIRegister *greg = gp->m_pGUIRamRegisters->Get(reg->getAddress());
      Add(REGISTER_RAM, greg, reg);
    }
  }
}

//------------------------------------------------------------------------
// NewSymbols
//
void Watch_Window::NewProcessor(GUI_Processor *)
{
  if (!gp || !gp->cpu)
    return;

  ReadSymbolList();
}

//------------------------------------------------------------------------
// Build
//
//

void Watch_Window::Build()
{
  if (bIsBuilt)
    return;

  GtkWidget *vbox;
  GtkWidget *scrolled_window;

  gtk_window_set_title(GTK_WINDOW(window), "Watch Viewer");

  watch_list = gtk_list_store_new(N_COLUMNS,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  watch_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(watch_list));

  coldata.reserve(COLUMNS);

  for (size_t i = 0; i < COLUMNS; i++) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
      watch_titles[i], renderer, "text", int(i), nullptr);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(watch_tree), column);

    int vis;
    if (!config_get_variable(name(), (gchar *)watch_titles[i], &vis))
      config_set_variable(name(), (gchar *)watch_titles[i], 1);

    coldata.push_back(ColumnData(column, vis));
  }

  {
    // Fix a db error in previous versions of gpsim
    int j;
    while (config_get_variable(name(), "hex", &j))
      config_remove(name(), "hex");

    const int hexIndex = 3;
    config_set_variable(name(), watch_titles[hexIndex], coldata[hexIndex].GetVisibility());
  }

  // TODO:No column sorting for now

  GtkTreeSelection *sel =
    gtk_tree_view_get_selection(GTK_TREE_VIEW(watch_tree));
  g_signal_connect(sel, "changed", G_CALLBACK(watch_list_row_selected), this);

  g_signal_connect(watch_tree, "button_press_event", G_CALLBACK(do_popup), this);
  g_signal_connect(watch_tree, "key_press_event", G_CALLBACK(key_press), this);

  scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);

  vbox = gtk_vbox_new(FALSE, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), watch_tree);

  gtk_container_add(GTK_CONTAINER(window),vbox);

  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

  build_menu();

  gtk_widget_show_all(window);


  enabled = 1;

  bIsBuilt = true;

  UpdateMenuItem();
}

Watch_Window::Watch_Window(GUI_Processor *_gp)
  : GUI_Object("watch_viewer"), watch_list(nullptr)
{
  menu = "/menu/Windows/Watch";

  gp = _gp;

  if (enabled)
    Build();
}

#endif // HAVE_GUI
