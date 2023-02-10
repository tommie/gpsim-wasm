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
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib-object.h>

#include "../src/breakpoints.h"
#include "../src/interface.h"
#include "../src/processor.h"
#include "../src/registers.h"
#include "../src/trace.h"
#include "../src/value.h"

#include "gui.h"
#include "preferences.h"
#include "gui_processor.h"
#include "gui_register.h"
#include "gui_regwin.h"
#include "gui_watch.h"

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>

class gpsimObject;

#define TRACE_FILE_FORMAT_ASCII 0
#define TRACE_FILE_FORMAT_LXT 1

extern int gui_question(const char *question, const char *a, const char *b);


// extern GUI_Processor *gp;

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_ON_CHANGE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_ADD_WATCH,
    MENU_SETTINGS,
    MENU_LOG_SETTINGS,
    MENU_LOG_READ,
    MENU_LOG_WRITE,
    MENU_LOG_READ_VALUE,
    MENU_LOG_WRITE_VALUE,
    MENU_REGWIN_REFRESH,
} menu_id;

typedef struct _menu_item {
    const char *name;
    const menu_id id;
} menu_item;

static const menu_item menu_items[] = {
    {"Clear breakpoints", MENU_BREAK_CLEAR},
    {"Set break on read", MENU_BREAK_READ},
    {"Set break on write", MENU_BREAK_WRITE},
    {"Set break on change", MENU_BREAK_ON_CHANGE},
    {"Set break on read value...", MENU_BREAK_READ_VALUE},
    {"Set break on write value...", MENU_BREAK_WRITE_VALUE},
    {"Set log settings...", MENU_LOG_SETTINGS},
    {"Set log on read", MENU_LOG_READ},
    {"Set log on write", MENU_LOG_WRITE},
    {"Set log on read value...", MENU_LOG_READ_VALUE},
    {"Set log on write value...", MENU_LOG_WRITE_VALUE},
    {"Add watch", MENU_ADD_WATCH},
    {"Refresh", MENU_REGWIN_REFRESH},
    {"Settings...", MENU_SETTINGS}
};

//========================================================================
//

//--------------------------------------------------
// get_register
// get the "real" register. If 'bTopLevelOnly' is true
//
Register *GUIRegister::get_register()
{
  return rma ? rma->get_register(address) : nullptr;
}

void GUIRegister::put_value(unsigned int new_value)
{
  Register *reg = get_register();

  if (reg)
  {
    reg->put_value(new_value);

  // Shadow a copy of the register value so that we can tell if it has changed
  // when we go to perform an update in the future.

    shadow = reg->getRV_notrace();
  }
}

void GUIRegister::put_shadow(RegisterValue new_value)
{
  // Update the shadow copy of the register without updating the register.
  shadow = new_value;
}

unsigned int GUIRegister::get_value()
{
  Register *reg = get_register();

  return reg ? reg->get_value() : 0;
}


RegisterValue GUIRegister::getRV()
{
  Register *reg = get_register();

  if (reg)
  {
    return reg->getRV_notrace();
  }

  return RegisterValue(0, 0);
}

char * GUIRegister::getValueAsString(char *str, int len, RegisterValue value)
{
  if (!str || len <= 0)
    return nullptr;

  if (bIsValid()) {

    const char hex2ascii[] = "0123456789ABCDEF";
    const int min = (len < register_size * 2) ? len : register_size * 2;

    if (value.data == INVALID_VALUE)
      value.init = 0xfffffff;

    for (int i = 0; i < min; i++) {
      if (value.init & 0x0f)
        str[min - i - 1] = '?';
      else
        str[min - i - 1] = hex2ascii[value.data & 0x0f];
      value >>= 4;
    }
    str[min] = 0;

  } else
    *str = '\0';

  return str;
}

bool GUIRegister::hasChanged(RegisterValue &current_value) const
{
  return (shadow != current_value);
}

void GUIRegister::Clear_xref()
{
  Register *reg = get_register();

  if (reg)
    reg->remove_xref((gpointer *)xref);
}

void GUIRegister::Assign_xref(CrossReferenceToGUI *new_xref)
{
  Register *reg = get_register();

  if (reg)
    reg->add_xref( (gpointer *)new_xref);

  xref = new_xref;
}

bool GUIRegister::hasBreak()
{
  return rma ? rma->hasBreak(address) : false;
}

std::string GUIRegister::name()
{
  Register *reg = get_register();
  if (!reg) {
    return "NULL";
  }

  //register_symbol * pRegSym = get_symbol_table().findRegisterSymbol(reg->address);

  if (reg->isa() == Register::INVALID_REGISTER)
    return "";

  std::string buffer;
  if (bIsAliased)
    buffer = "alias (" + reg->name() + ")";
  else
    buffer = reg->name();

  return buffer;
}

bool GUIRegister::bIsValid()
{
  if (rma && (*rma)[address].getReg())
    return true;
  return false;
}

bool GUIRegister::bIsSFR()
{
  if (rma && (*rma)[address].isa() == Register::SFR_REGISTER)
    return true;
  return false;
}

GUIRegister::GUIRegister()
  : rma(nullptr), address(0), row(0), col(0), register_size(0),
    bUpdateFull(false), bIsAliased(false), xref(nullptr)
{
}

GUIRegister::~GUIRegister()
{
  delete xref;
}

//========================================================================

class RegisterWindowXREF : public CrossReferenceToGUI
{
public:

  virtual void Update(int /* new_value */) override
  {
    GUIRegister *reg = (GUIRegister *) (data);
    Register_Window *rw  = (Register_Window *) (parent_window);

    if (reg->row > GTK_SHEET(rw->register_sheet)->maxrow)
      {
      puts("Warning reg->row > maxrow in xref_update_cell");
      return;
      }

    int address = rw->row_to_address[reg->row] + reg->col;

    rw->registers->Get(address)->bUpdateFull = true;
    rw->UpdateRegisterCell(address);

    rw->UpdateASCII(reg->row);
  }
};


//========================================================================
//
// Create a class for an invalid register and instantiate a single instance
// of it. The purpose of this is to provide a place holder for the gui
// register array.
//

class InvalidGuiRegister : public GUIRegister {
public:
  void put_value(unsigned int /* new_value */ )
  {
    printf("(gui_regwin)Warning: writing to invalid register\n");
  }
  unsigned int get_value() { return 0; }

  InvalidGuiRegister()
  {
    rma = nullptr;
  }
private:
  void operator delete(void * /* ignore */ ) {}
};

static InvalidGuiRegister THE_invalid_register;

//========================================================================
// GtkSheet extensions
//
// The gtk_sheet api does not provide access to row and column labels.
// This means we can manipulate the font the way the cells can be manipulated

#if 0 // defined but not used
static GtkSheetButton *
gtk_sheet_row_button_get(GtkSheet *sheet, gint row)
{
  g_return_val_if_fail (sheet != NULL, NULL);
  g_return_val_if_fail (GTK_IS_SHEET (sheet), NULL);

  if(row < 0 || row > sheet->maxrow) return NULL;

  return (&sheet->row[row].button);
}
#endif

static void
gtk_sheet_REALLY_set_row_height(GtkSheet *sheet, gint row, gint height)
{
  g_return_if_fail(sheet != nullptr);
  g_return_if_fail(GTK_IS_SHEET(sheet));

  if (row < 0 || row > sheet->maxrow)
    return;

  sheet->row[row].height = height;
}

// Sanatize the numeric text input
// Only checks that the input is less that 0xffff

static void sanatize_numeric(GtkEditable *editable, gchar *new_text,
  gint new_text_length, gint *position, gpointer user_data)
{
  bool ok = true;
  char *current_text = gtk_editable_get_chars(editable, 0, -1);
  std::string text(current_text);
  text.insert(*position, new_text);
  g_free(current_text);

  if (text != "0x" && text != "0X") {
    char *end_char;
    unsigned long value = strtoul(text.c_str(), &end_char, 0);
    if (value > 0xffff || *end_char != '\0')
      ok = false;
  }

  if (ok) {
    g_signal_handlers_block_by_func(G_OBJECT(editable),
      gpointer(sanatize_numeric), user_data);
    gtk_editable_insert_text(editable, new_text, new_text_length, position);
    g_signal_handlers_unblock_by_func(G_OBJECT(editable),
      gpointer(sanatize_numeric), user_data);
  }

  g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
}

static void sanatize_numeric_0x(GtkEditable *editable, gchar *new_text,
  gint new_text_length, gint *position, gpointer user_data)
{
  bool ok = true;
  char *current_text = gtk_editable_get_chars(editable, 0, -1);
  std::string text(current_text);
  text.insert(*position, new_text);
  g_free(current_text);

  char *end_char;
  unsigned long value = strtoul(text.c_str(), &end_char, 16);
  if (value > 0xffff || *end_char != '\0')
    ok = false;

  if (ok) {
    g_signal_handlers_block_by_func(G_OBJECT(editable),
      gpointer(sanatize_numeric_0x), user_data);
    gtk_editable_insert_text(editable, new_text, new_text_length, position);
    g_signal_handlers_unblock_by_func(G_OBJECT(editable),
      gpointer(sanatize_numeric_0x), user_data);
  }

  g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
}

//========================================================================
// get_value
// used for reading a value from user when break on value is requested
int gui_get_value(const char *prompt)
{
  GtkWidget *dialog;

  dialog = gtk_dialog_new_with_buttons("enter value",
    nullptr,
    GTK_DIALOG_MODAL,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    nullptr);

  GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *label = gtk_label_new("values can be entered in decimal, hexadecimal, and octal.\nFor example: 31 is the same as 0x1f and 037");

  gtk_box_pack_start(GTK_BOX(area), label, FALSE, FALSE, 18);

  GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(area), hbox, FALSE, FALSE, 18);
  label = gtk_label_new(prompt);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  GtkWidget *entry = gtk_entry_new();
  g_signal_connect(entry, "insert-text", G_CALLBACK(sanatize_numeric), nullptr);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);

  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
    gtk_widget_destroy(dialog);
    return -1;
  }

  const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(entry));

  if (*entry_text == '\0') {
    gtk_widget_destroy(dialog);
    return -1;
  }

  int value = strtoul(entry_text, nullptr, 0);
  gtk_widget_destroy(dialog);
  return value;
}

// used for reading a value from user when break on value is requested
void gui_get_2values(const char *prompt1, int *value1, const char *prompt2, int *value2)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("enter values",
    nullptr,
    GTK_DIALOG_MODAL,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    nullptr);

  GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *label = gtk_label_new("values can be entered in decimal, hexadecimal, and octal.\nFor example: 31 is the same as 0x1f and 037");
  gtk_box_pack_start(GTK_BOX(area), label, FALSE, FALSE, 18);

  GtkWidget *hbox, *entry1, *entry2;

  hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(area), hbox, FALSE, FALSE, 18);
  label = gtk_label_new(prompt1);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  entry1 = gtk_entry_new();
  g_signal_connect(entry1, "insert-text", G_CALLBACK(sanatize_numeric), nullptr);
  gtk_box_pack_start(GTK_BOX(hbox), entry1, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(area), hbox, FALSE, FALSE, 18);
  label = gtk_label_new(prompt2);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  entry2 = gtk_entry_new();
  g_signal_connect(entry2, "insert-text", G_CALLBACK(sanatize_numeric), nullptr);
  gtk_box_pack_start(GTK_BOX(hbox), entry2, FALSE, FALSE, 0);

  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
    *value1 = -1;
    *value2 = -1;
    gtk_widget_destroy(dialog);
    return;
  }

  const gchar *entry_text;

  entry_text = gtk_entry_get_text(GTK_ENTRY(entry1));
  if (*entry_text == '\0') {
    *value1 = -1;
    *value2 = -1;
    gtk_widget_destroy(dialog);
    return;
  }
  *value1 = strtoul(entry_text, nullptr, 0);

  entry_text = gtk_entry_get_text(GTK_ENTRY(entry2));
  if (*entry_text == '\0') {
    *value1 = -1;
    *value2 = -1;
    gtk_widget_destroy(dialog);
    return;
  }
  *value2 = strtoul(entry_text, nullptr, 0);
  gtk_widget_destroy(dialog);
}

// Select logging file.
// Returns pointer to file name. This needs to be freed with g_free()
// Can return the optional mode
static char *gui_get_log_settings(int *mode)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Log settings",
    nullptr,
    GTK_FILE_CHOOSER_ACTION_SAVE,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
    nullptr);

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

  GtkWidget *combo_text;

  if (mode) {
    combo_text = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_text), "ASCII");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_text), "LXT");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_text), 0);

    GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
    GtkWidget *label = gtk_label_new("File format:");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), combo_text, FALSE, FALSE, 0);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), hbox);

    gtk_widget_show_all(hbox);
  }

  char *file = nullptr;
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    if (mode) {
      gint position = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_text));
      if (position == 0) *mode = TRACE_FILE_FORMAT_ASCII;
      if (position == 1) *mode = TRACE_FILE_FORMAT_LXT;
    }
  }
  gtk_widget_destroy(dialog);

  return file;
}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, Register_Window *rw)
{
  int i, j;
  int value, mask;
  char *filename;
  int mode;

  Dprintf((" popup_activated\n"));

  if (!rw->gp || !rw->gp->cpu) {
    printf(" no cpu\n");
    return;
  }

  GtkSheet *sheet = GTK_SHEET(rw->register_sheet);
  GtkSheetRange range = sheet->range;
  gsize item = GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(widget), "item"));

  for (j = range.row0; j <= range.rowi; j++) {
    for (i = range.col0; i <= range.coli; i++) {

      unsigned int address = rw->row_to_address[j] + i;
      switch (item) {
      case MENU_BREAK_READ:
        get_bp().set_read_break(rw->gp->cpu, address);
        break;

      case MENU_BREAK_WRITE:
        get_bp().set_write_break(rw->gp->cpu, address);
        break;

      case MENU_BREAK_ON_CHANGE:
        get_bp().set_change_break(rw->gp->cpu, address);
        break;

      case MENU_BREAK_READ_VALUE:
        value = gui_get_value("value to read for breakpoint:");
        if (value < 0)
          break; // Cancel
        get_bp().set_read_value_break(rw->gp->cpu,address,value);
        break;

      case MENU_BREAK_WRITE_VALUE:
        value = gui_get_value("value to write for breakpoint:");
        if (value < 0)
          break; // Cancel
        get_bp().set_write_value_break(rw->gp->cpu,address,value);
        break;

      case MENU_BREAK_CLEAR:
        get_bp().clear_all_register(rw->gp->cpu,address);
        break;

      case MENU_ADD_WATCH:
        rw->gp->watch_window->Add(rw->type, rw->registers->Get(address));
        break;

      case MENU_LOG_READ:
        GetTraceLog().enable_logging();
        // FIXME the register type is ignored here (and in all other cases
        // where we're logging -- it's assumed that the register address is
        // for ram, even if in fact the user requests eeprom.
        get_bp().set_notify_read(rw->gp->cpu,address);
        break;

      case MENU_LOG_WRITE:
        get_bp().set_notify_write(rw->gp->cpu,address);
        break;

      case MENU_LOG_READ_VALUE:
        gui_get_2values("Value that the read must match for logging it:", &value,
                        "Bitmask that specifies the bits to bother about:", &mask);
        if (value < 0)
          break; // Cancel
        get_bp().set_notify_read_value(rw->gp->cpu,address, value, mask);
        break;

      case MENU_LOG_WRITE_VALUE:
        gui_get_2values("Value that the write must match for logging it:", &value,
                        "Bitmask that specifies the bits to bother about:", &mask);
        if (value < 0)
          break; // Cancel
        get_bp().set_notify_write_value(rw->gp->cpu,address, value, mask);
        break;

      case MENU_SETTINGS:
        rw->SettingsDialog();
        return;
        break;

      case MENU_LOG_SETTINGS:
        filename = gui_get_log_settings(&mode);
        if (filename)
          GetTraceLog().enable_logging(filename,mode);
        g_free(filename);
        return;
        break;

      case MENU_REGWIN_REFRESH:
        rw->Update();
        return;
        break;

      default:
        puts("Unhandled menuitem?");
        break;
      }
    }
  }
}


GtkWidget *Register_Window::build_menu()
{
  GtkWidget *menu = gtk_menu_new();

  for (gsize i = 0; i < G_N_ELEMENTS(menu_items); i++) {
    GtkWidget *item = gtk_menu_item_new_with_label(menu_items[i].name);

    g_signal_connect(item, "activate", G_CALLBACK(popup_activated), this);
    g_object_set_data(G_OBJECT(item), "item",
      GSIZE_TO_POINTER(menu_items[i].id));

    if (type == REGISTER_EEPROM
       && menu_items[i].id != MENU_ADD_WATCH
       && menu_items[i].id != MENU_SETTINGS)
    {
      gtk_widget_set_sensitive(item, FALSE);
    }
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  return menu;
}

// button press handler
void Register_Window::do_popup(GtkWidget *, GdkEventButton *event)
{
  int button, event_time;

  if (event) {
    button = event->button;
    event_time = event->time;
  } else {
    button = 0;
    event_time = gtk_get_current_event_time();
  }
  gtk_menu_popup(GTK_MENU(popup_menu), nullptr, nullptr, nullptr, nullptr,
    button, event_time);
}

// button press signal handler (static)

gboolean Register_Window::button_press(GtkWidget *widget,
  GdkEventButton *event, Register_Window *rw)
{
  if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
    rw->do_popup(widget, event);
    return TRUE;
  }

  return FALSE;
}

// Popup menu keyboard signal handler (static)

gboolean
Register_Window::popup_menu_handler(GtkWidget *widget,
  Register_Window *rw)
{
  rw->do_popup(widget, nullptr);
  return TRUE;
}

// when a new cell is selected, we write changes in
// previously selected cell to gpsim
// (the name of the signal seems a bit strange)
static void
set_cell(GtkWidget *widget, int row, int col, Register_Window *rw)
{
  Dprintf((" set_cell\n"));

  GtkSheet *sheet = GTK_SHEET(widget);

  if (!widget ||
     row > sheet->maxrow || row < 0 ||
     col > sheet->maxcol || col < 0 || !rw)
    {
      printf("Warning set_cell(%p,%x,%x,%p)\n", widget, row, col, rw);
      return;
    }

  GUIRegister *reg = rw->getRegister(row, col);

  if (!reg)
    return; // ignore user changes in ascii column for right now

  // extract value from sheet cell
  GtkWidget * sheet_entry = gtk_sheet_get_entry(sheet);
  if (!sheet_entry)
    return;

  const gchar *text = gtk_entry_get_text(GTK_ENTRY(sheet_entry));
  unsigned long n = strtoul(text, nullptr, 16);

  if (*text == '\0') {
    // n = reg->get_value();
    reg->put_shadow(RegisterValue(INVALID_VALUE, INVALID_VALUE));
  } else if (n != (unsigned long) reg->get_shadow().data) {
    reg->put_value(n & gpGuiProcessor->cpu->register_mask());
    rw->UpdateASCII(row);
  }
}

//------------------------------------------------------------------------
// int column_width(int col)
//
// Return the width of one of the register sheet columns.
//
// Column = -1 is the row label
// Columns 0-REGISTERS_PER_ROW are ram/registers
// Column REGISTERS_PER_ROW is an ASCII string of the row data.
//
// The width of the column is based on width of a single character,
// char_width. This width is depends on the font and is computed in
// LoadStyles()
//
// FIXME: column_width assumes there are fewer than 0x1000 registers.
int Register_Window::column_width(int col)
{
  if (!char_width)
    return 0;

  // Row Labels
  if (col < 0)
    return char_width * 3;

  // Register data
  if (col < REGISTERS_PER_ROW)
    return char_width * chars_per_column;

  // ASCII column
  return char_width * (REGISTERS_PER_ROW + 1) + char_width / 2;
}

//------------------------------------------------------------------------
int Register_Window::row_height(int)
{
  return char_height ? char_height : 20;
}

//------------------------------------------------------------------------
// UpdateLabel
//
//

void Register_Window::UpdateLabel()
{
  int row = -1, col = -1;

  if (register_sheet != 0) {
    gtk_sheet_get_active_cell(register_sheet, &row, &col);

    if (col > -1 && row > -1) {
      if (col >= REGISTERS_PER_ROW)
        gtk_label_set_text(GTK_LABEL(location), "  ascii  ");
      else {

        GUIRegister *reg = getRegister(row, col);

        std::string n = reg ? reg->name() : "INVALID_REGISTER";

        gtk_label_set_text(GTK_LABEL(location), n.c_str());
      }
    }
  }
}

//------------------------------------------------------------------------
// UpdateEntry
//
//

void Register_Window::UpdateEntry()
{
  gint row, col;

  if (register_sheet != 0) {
    GtkWidget *sheet_entry = gtk_sheet_get_entry(register_sheet);
    gtk_sheet_get_active_cell(register_sheet, &row, &col);

    if (row_to_address[row] < 0)
      return;

    GUIRegister *reg = getRegister(row, col);

    if (reg && reg->bIsValid())
    {
      const char *text = gtk_entry_get_text(GTK_ENTRY(sheet_entry));
      gtk_entry_set_text(GTK_ENTRY(entry), text);
    }
  }
}

//------------------------------------------------------------------------
// UpdateLabelEntry
//
//

void Register_Window::UpdateLabelEntry()
{
  UpdateLabel();
  UpdateEntry();
}

//------------------------------------------------------------------------
void Register_Window::UpdateStyle()
{
  if (!register_sheet || !normalfont)
    return;

  GtkSheetRange range;

  //gtk_sheet_freeze(register_sheet);

  // Update the font for the cells
  range.row0 = 0;
  range.rowi = register_sheet->maxrow;
  range.col0 = 0;
  range.coli = register_sheet->maxcol;
  gtk_sheet_range_set_font(register_sheet, &range, normalfont);

  // Update the font for the row and column labels
  gtk_widget_modify_font(GTK_WIDGET(register_sheet), normalfont);

  // Adjust the cell sizes based on the font size
  int i;

  for (i = 0; i <= register_sheet->maxcol; i++)
    gtk_sheet_set_column_width(register_sheet, i, column_width(i));

  for (i = 0; i <= register_sheet->maxrow; i++)
    gtk_sheet_REALLY_set_row_height(register_sheet, i, row_height(i));

  gtk_sheet_set_row_titles_width(register_sheet, column_width(-1));
  gtk_sheet_set_column_titles_height(register_sheet, row_height(0));
  //gtk_sheet_thaw(register_sheet);
}

//------------------------------------------------------------------------
int Register_Window::LoadStyles()
{
  normalfont = pango_font_description_from_string(normalfont_string.c_str());

  if (!normalfont)
  {
    char_width = 0;
    char_height = 0;
    return 0;
  }

  {
    PangoRectangle rect;
    PangoLayout *layout;

    layout = gtk_widget_create_pango_layout(GTK_WIDGET(register_sheet), "A");
    pango_layout_set_font_description(layout, normalfont);

    pango_layout_get_extents(layout, nullptr, &rect);
    char_width =  PANGO_PIXELS(rect.width);
    char_height = PANGO_PIXELS(rect.height + (rect.height << 1)) >> 1;
    g_object_unref(G_OBJECT(layout));
  }

  return 1;
}

/********************** Settings dialog ***************************/

int Register_Window::SettingsDialog()
{
  GtkWidget *dialog;

  dialog = gtk_dialog_new_with_buttons("Register window settings",
    nullptr,
    GTK_DIALOG_MODAL,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    nullptr);

  GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
  GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_box_pack_start(GTK_BOX(area), hbox, FALSE, FALSE, 18);

  GtkWidget *label = gtk_label_new("Normal font:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  GtkWidget *font_btn = gtk_font_button_new_with_font(normalfont_string.c_str());
  gtk_box_pack_start(GTK_BOX(hbox), font_btn, FALSE, FALSE, 0);

  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
    gtk_widget_destroy(dialog);
    return 0;
  }

  PangoFontDescription *font;
  const gchar *font_name = gtk_font_button_get_font_name(GTK_FONT_BUTTON(font_btn));
  font = pango_font_description_from_string(font_name);

  if (font) {
    pango_font_description_free(font);
    normalfont_string = font_name;
    config_set_string(name(), "normalfont", normalfont_string.c_str());

    gtk_sheet_freeze(register_sheet);
    UpdateStyle();
    gtk_sheet_thaw(register_sheet);
  }

  gtk_widget_destroy(dialog);

  return 0;
}

static gboolean
clipboard_handler(GtkWidget *widget, GdkEventKey *key)
{
  Dprintf((" clipboard_handler\n"));

  GtkSheet *sheet;

  sheet = GTK_SHEET(widget);

  if (key->state & GDK_CONTROL_MASK || key->keyval == GDK_KEY_Control_L ||
     key->keyval == GDK_KEY_Control_R){
    if ((key->keyval == 'c' || key->keyval == 'C') && sheet->state != gint(GTK_STATE_NORMAL)) {

      /*
        --- tsd - commented out because this function
        is not defined in the official gtkextra-2.0 release.
      if (gtk_sheet_in_clip(sheet))
        gtk_sheet_unclip_range(sheet);
      */

      gtk_sheet_clip_range(sheet, &sheet->range);
    }
    if (key->keyval == 'x' || key->keyval == 'X')
            gtk_sheet_unclip_range(sheet);
  }
  return 0;
}

static void
resize_handler(GtkWidget *widget, GtkSheetRange *old_range,
                                  GtkSheetRange *new_range,
                                  Register_Window *rw)
{
  Dprintf((" resize_handler\n"));

  if (!widget || !old_range || !new_range || !rw)
    {
      printf("Warning resize_handler(%p,%p,%p,%p)\n", widget, old_range, new_range, rw);
      return;
    }

  int cti = new_range->coli - new_range->col0 + 1;
  int ctj = new_range->rowi - new_range->row0 + 1;

  // We always copy from this one cell.
  int from = rw->row_to_address[old_range->row0] + old_range->col0;

  for (int j = 0; j < ctj; j++)
    {
      for (int i = 0; i < cti; i++)
        {
          int to = rw->row_to_address[new_range->row0 + j] + new_range->col0 + i;
          rw->registers->Get(to)->put_value(rw->registers->Get(from)->get_value());
        }
    }
}

static void
move_handler(GtkWidget *widget,
             GtkSheetRange *old_range,
             GtkSheetRange *new_range,
             Register_Window *rw)
{
  Dprintf((" move_handler\n"));

  if (!widget || !old_range || !new_range || !rw)
      return;

  if (old_range->row0 < 0 || old_range->col0 < 0 ||
    new_range->row0 < 0 || new_range->col0)
      return;

  int cti = new_range->coli - new_range->col0 + 1;
  int ctj = new_range->rowi - new_range->row0 + 1;

  for (int j = 0; j < ctj; j++)
    {
      for (int i = 0; i < cti; i++)
        {
          int from = rw->row_to_address[old_range->row0 + j] + old_range->col0 + i;
          int to = rw->row_to_address[new_range->row0 + j] + new_range->col0 + i;
          rw->registers->Get(to)->put_value(rw->registers->Get(from)->get_value());
        }
    }
}


/* when the entry above the sheet is changed (typed a digit), we
   copy it to the cell entry */
static void
show_sheet_entry(GtkWidget *widget, Register_Window *rw)
{
  Dprintf((" show_sheet_entry\n"));

  int row, col;

  if (!widget || !rw)
    {
      printf("Warning show_sheet_entry(%p,%p)\n", widget, rw);
      return;
    }

  if (!gtk_widget_has_focus(widget)) return;

  GtkSheet *sheet = GTK_SHEET(rw->register_sheet);
  GtkEntry *sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

  //row=sheet->active_cell.row; col=sheet->active_cell.col;
  gtk_sheet_get_active_cell(sheet, &row, &col);

  GUIRegister *reg = rw->getRegister(row, col);

  if (reg && reg->bIsValid()) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(rw->entry));
    if (sheet_entry)
      gtk_entry_set_text(sheet_entry, text);
  }
}

/* when we have new data in the entry above the sheet, we
 copy the data to the cells/registers

 this don't get called when it is clicked
 in, only when we hit return
 */
static void
activate_sheet_entry(GtkWidget *widget, Register_Window *rw)
{
  Dprintf((" activity_sheet_entry\n"));

  gint row, col;

  if (!widget || !rw)
  {
      printf("Warning activate_sheet_entry(%p,%p)\n", widget, rw);
      return;
  }

  GtkSheet *sheet = GTK_SHEET(rw->register_sheet);

  gtk_sheet_get_active_cell(sheet, &row, &col);

  // if there are text written in the entry above the sheet, then
  // the same data is in the sheet cell (because of show_sheet_entry())

  // so we use set_cell() to write the changes from the sheet cell to gpsim
  set_cell(GTK_WIDGET(sheet), row, col, rw);
  rw->UpdateASCII(row);
}

/*
 we get here when the entry in a cell is changed (typed a digit), we
 copy it to the entry above the sheet.
 */
static void
show_entry(GtkWidget *widget, Register_Window *rw)
{
  Dprintf((" show_entry\n"));

    if (!widget || !rw)
    {
        printf("Warning show_entry(%p,%p)\n", widget, rw);
        return;
    }

    if (!gtk_widget_has_focus(widget)) return;

    rw->UpdateEntry();
}

/* when the sheet cursor has activated a new cell, we set the
   label and entry above the sheet
 */
static gint
activate_sheet_cell(GtkWidget *widget, gint row, gint column, Register_Window *rw)
{
  Dprintf((" activate_sheet_cell rma=%p\n", (rw ? rw->rma : nullptr)));

  GtkSheet *sheet = rw ? rw->register_sheet : nullptr;

  if (!sheet)
    return 0;

  if (!widget || row > sheet->maxrow || row < 0 ||
     column > sheet->maxcol || column < 0 || !rw)
    {
      printf("Warning activate_sheet_cell(%p,%x,%x,%p)\n", widget, row, column, rw);
      return 0;
    }

  GUIRegister *reg = rw->getRegister(row, column);

  if (reg && reg->bIsValid())
    // enable editing valid cells
    gtk_editable_set_editable(GTK_EDITABLE(gtk_sheet_get_entry(rw->register_sheet)), TRUE);
  else
    // disable editing invalid cells
    gtk_editable_set_editable(GTK_EDITABLE(gtk_sheet_get_entry(rw->register_sheet)), FALSE);


  rw->UpdateLabelEntry();

  return TRUE;
}

GUIRegister *Register_Window::getRegister(int row, int col)
{
  if (registers && col < REGISTERS_PER_ROW && row < MAX_ROWS) {

    int reg_address = row_to_address[row];

    if (reg_address < 0)
      return nullptr;

    if (reg_address + col < MAX_REGISTERS)
      return registers->Get(reg_address + col);
  }

  return nullptr;
}

//-------------------------------------------------------------------
GUIRegister *Register_Window::operator [] (int address)
{
  if (!registers || address >= MAX_REGISTERS || address < 0)
    return nullptr;

  return registers->Get(address);
}

void Register_Window::SelectRegister(int regnumber)
{
  GtkSheetRange range;
  int row, col;

  if (regnumber > MAX_REGISTERS || regnumber < 0) {
    printf("Warning: %s - regnumber = %x\n", __FUNCTION__, regnumber);
    return;
  }

  if (!gp || !gp->cpu ||!registers || !registers->Get(regnumber)) {
    printf("SelectRegister is not ready yet\n");
    return;
  }
  row = registers->Get(regnumber)->row;
  col = registers->Get(regnumber)->col;
  range.row0 = range.rowi = row;
  range.col0 = range.coli = col;

  gtk_sheet_select_range(GTK_SHEET(register_sheet), &range);
  if (register_sheet != nullptr &&
     (GTK_SHEET(register_sheet)->view.col0 > range.col0 ||
     GTK_SHEET(register_sheet)->view.coli < range.coli ||
     GTK_SHEET(register_sheet)->view.row0 > range.row0 ||
      GTK_SHEET(register_sheet)->view.rowi < range.rowi))
    gtk_sheet_moveto(GTK_SHEET(register_sheet), row, col, 0.5, 0.5);

  UpdateLabelEntry();
}

void Register_Window::SelectRegister(Value *regSym)
{
  Register *pReg = dynamic_cast<Register *>(regSym);
  if (pReg && register_sheet)
    SelectRegister(pReg->getAddress());
  /*
  if(regSym  && typeid(*regSym) == typeid(register_symbol) &&
      register_sheet != NULL) {
    Register* pReg = (Register*)((register_symbol*)regSym)->getReg();
    SelectRegister(pReg->address);
  }
  */
}

static void
build_entry_bar(GtkWidget *main_vbox, Register_Window *rw)
{
  Dprintf((" build_entry_bar\n"));

  GtkRequisition request;
  GtkWidget *status_box;

  if (!main_vbox || !rw)
  {
      printf("Warning build_entry_bar(%p,%p)\n", main_vbox, rw);
      return;
  }

  status_box = gtk_hbox_new(FALSE, 1);
  gtk_container_set_border_width(GTK_CONTAINER(status_box), 0);
  gtk_box_pack_start(GTK_BOX(main_vbox), status_box, FALSE, TRUE, 0);
  gtk_widget_show(status_box);

  rw->location=gtk_label_new("");
  gtk_widget_size_request(rw->location, &request);
  gtk_widget_set_size_request(rw->location, 160, request.height);
  gtk_box_pack_start(GTK_BOX(status_box), rw->location, FALSE, TRUE, 0);
  gtk_widget_set_can_default(rw->location, TRUE);
  gtk_widget_show(rw->location);

  rw->entry = gtk_entry_new();
  g_signal_connect(rw->entry, "insert-text", G_CALLBACK(sanatize_numeric_0x), nullptr);
  gtk_box_pack_start(GTK_BOX(status_box), rw->entry, TRUE, TRUE, 0);
  gtk_widget_show(rw->entry);
}

void Register_Window::UpdateASCII(gint row)
{
  gint i;
  gchar name[32];

  if (row < 0 || row > register_sheet->maxrow)
  {
      printf("Warning update_ascii(%x)\n", row);
      return;
  }

  if (!registers_loaded)
      return;

  for (i = 0; i < REGISTERS_PER_ROW; i++)
  {
    name[i] = registers->Get(row_to_address[row] + i)->get_shadow().data;

    if ((name[i] < ' ') || (name[i] > '~'))
      name[i] = '.';
  }

  name[REGISTERS_PER_ROW] = 0;

  gtk_sheet_set_cell(GTK_SHEET(register_sheet), row, REGISTERS_PER_ROW, GTK_JUSTIFY_RIGHT, name);
}

gboolean Register_Window::UpdateRegisterCell(int reg_number)
{
  static gboolean bTrace = false;
  gchar name[16];

  GtkSheetRange range;
  gboolean retval = FALSE;

  if (reg_number < 0 || reg_number >= MAX_REGISTERS)
  {
      printf("Warning update_register_cell(%x)\n", reg_number);
      return 0;
  }

  if (!enabled)
    return 0;      // Don't read registers when hidden. Esp with ICD.

  GUIRegister *guiReg = registers->Get(reg_number);

  if (!guiReg || !guiReg->rma)
    return 0;

  if ((unsigned int)reg_number >= guiReg->rma->get_size())
    return 0;

  range.row0 = guiReg->row;
  range.rowi = guiReg->row;
  range.col0 = guiReg->col;
  range.coli = guiReg->col;

  // bulk mode stuff is for the ICD.
  gpsim_set_bulk_mode(1);
  RegisterValue new_value = guiReg->getRV();
  gpsim_set_bulk_mode(0);

  RegisterValue last_value = guiReg->get_shadow();


  if (bTrace)
    printf("UpdateRegisterCell() Entry: regID=%3d, Full=%s, hasChanged=%s\n",
      reg_number, guiReg->bUpdateFull ? "true " : "false",\
      guiReg->hasChanged(new_value) ? "true " : "false");
  if (guiReg->bUpdateFull) {

    // A 'Full Update' means that the foreground and background colors
    // need to be repainted.

    guiReg->bUpdateFull = false;

    if (guiReg->row <= register_sheet->maxrow) {

      guiReg->getValueAsString(name, sizeof(name), new_value);

      gtk_sheet_set_cell(GTK_SHEET(register_sheet),
                         guiReg->row,
                         guiReg->col,
                         GTK_JUSTIFY_RIGHT,name);
    }
    // else the register is invalid and out of the register sheet


    //if(new_value != last_value)
    if (guiReg->hasChanged(new_value)) {

      guiReg->put_shadow(new_value);
      guiReg->bUpdateFull = true;
      if (bTrace)
        printf("UpdateRegisterCell()    regID=3%d, bUpdateFull set to true 1\n", reg_number);
      gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, gColors.item_has_changed());
    } else
      gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, gColors.normal_fg());

    if (bTrace)
      printf("UpdateRegisterCell()    Background\n");
    if (guiReg->hasBreak())
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.breakpoint());
    else if (!guiReg->bIsValid())
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.invalid());
    else if (guiReg->bIsAliased)
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.alias());
    else if (guiReg->bIsSFR())
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.sfr_bg());
    else
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.normal_bg());


    retval = TRUE;
  } else if (guiReg->hasChanged(new_value)) { //new_value!=last_value) {

    if (new_value.data == INVALID_VALUE) {

      guiReg->put_shadow(RegisterValue(INVALID_VALUE, INVALID_VALUE));
      g_snprintf(name, sizeof(name), "??");
    } else {

      // the register has changed since last update
      guiReg->put_shadow(new_value);
      guiReg->getValueAsString(name, sizeof(name), new_value);
    }

    gtk_sheet_set_cell(GTK_SHEET(register_sheet),
                       guiReg->row,
                       guiReg->col,
                       GTK_JUSTIFY_RIGHT,name);

    guiReg->bUpdateFull = true;
    if (bTrace)
      printf("UpdateRegisterCell()    regID=3%d, bUpdateFull set to true 2\n", reg_number);
    gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, gColors.item_has_changed());

    retval = TRUE;
  }

  gint row,col;
  gtk_sheet_get_active_cell(register_sheet, &row, &col);

  if ((int)reg_number == (row_to_address[row] + col))
  {
    // if sheet cursor is standing on a cell that is changed, then
    // we update the entry above the sheet
    if (new_value.data != last_value.data)
      UpdateEntry();
  }

  if (bTrace)
    printf("UpdateRegisterCell() Exit:  regID=%3d, Full=%s, hasChanged=%s, retval=%s\n",
      reg_number, guiReg->bUpdateFull
      ? "true " : "false",
    guiReg->hasChanged(new_value) ? "true " : "false", retval ? "true " : "false");
  return retval;
}


void Register_Window::Update()
{
  if (!enabled)
    return;

  if (!gtk_widget_get_visible(window))
    return;

  if (!registers_loaded)
    return;

  if (!gp || !gp->cpu || !register_sheet || !gp->cpu->isHardwareOnline()) {
    puts("Warning can't update register window");
    return;
  }

  gtk_sheet_freeze(register_sheet);
  for (int j = 0; j <= GTK_SHEET(register_sheet)->maxrow; j++) {

    if (row_to_address[j] == -1)
      continue;

    bool bRowChanged = false;
    for (int i = 0; i < REGISTERS_PER_ROW; i++) {
      int address = row_to_address[j] + i;
      GUIRegister * pGuiReg = registers->Get(address);
      if (pGuiReg != &THE_invalid_register &&
         (pGuiReg->get_shadow().data != INVALID_VALUE ||
         pGuiReg->bUpdateFull)) {

        if (UpdateRegisterCell(address) == (gboolean)true)
          bRowChanged = true;
      }
    }
    if (bRowChanged)
      UpdateASCII(j);
  }

  gtk_sheet_thaw(register_sheet);
}

//------------------------------------------------------------------------
void Register_Window::SetRegisterSize()
{
  if (gp && gp->cpu)
    register_size = gp->cpu->register_size();
  else
    register_size = 1;

  chars_per_column = 1 + 2*register_size;

  if (register_sheet) {
    // Column labels
    for (int i = 0; i < register_sheet->maxcol; i++) {
      char buffer[10];
      g_snprintf(buffer, sizeof(buffer), "%02x", i);
      gtk_sheet_column_button_add_label(register_sheet, i, buffer);
      gtk_sheet_set_column_title(register_sheet, i, buffer);
      gtk_sheet_set_column_width(register_sheet, i, column_width(i));
    }

    // ASCII column
    int i = REGISTERS_PER_ROW;
    const char *ascii = "ASCII";
    gtk_sheet_column_button_add_label(register_sheet, i, ascii);
    gtk_sheet_set_column_title(register_sheet, i, ascii);

    gtk_sheet_set_column_width(register_sheet, i, column_width(i));

    gtk_sheet_set_row_titles_width(register_sheet, column_width(-1));
  }
}

GUIRegisterList::GUIRegisterList(RegisterMemoryAccess *pRMA)
{
  m_pRMA = pRMA;
  unsigned int uAddress;
  unsigned int uRegisterSize;

  uRegisterSize = (pRMA->get_size() < MAX_REGISTERS) ? (pRMA->get_size()) : MAX_REGISTERS;

  for (uAddress = 0; uAddress < uRegisterSize; uAddress++) {
    GUIRegister *pReg = new GUIRegister();
    pReg->rma = m_pRMA;
    pReg->address = uAddress;
    pReg->register_size = m_pRMA->get_cpu()->register_size();
    pReg->bIsAliased = (*m_pRMA)[uAddress].address != (unsigned int)uAddress;
    m_paRegisters[uAddress] = pReg;
  }
  for (; uAddress < MAX_REGISTERS; uAddress++)
    m_paRegisters[uAddress] = &THE_invalid_register;
}

GUIRegisterList::~GUIRegisterList()
{
  unsigned int nRegs;
  unsigned int uAddress;

  nRegs = (m_pRMA->get_size() < MAX_REGISTERS) ? (m_pRMA->get_size()) : MAX_REGISTERS;

  for (uAddress = 0; uAddress < nRegs; uAddress++) {
    if (m_paRegisters[uAddress] != &THE_invalid_register)
    {
       delete m_paRegisters[uAddress];
       m_paRegisters[uAddress] = nullptr;
    }
  }
}

//------------------------------------------------------------------------
void Register_Window::NewProcessor(GUI_Processor *)
{
  gint i,j, border_mask, border_width;
  unsigned int reg_number;
  GtkSheetRange range;

  if (!gp || !gp->cpu || !rma || !gp->cpu->isHardwareOnline())
    return;

  if (!enabled)
    return;

  if (!register_sheet){
    printf("Warning %s:%d\n", __FUNCTION__, __LINE__);
    return;
  }

  gboolean row_created = FALSE;
  unsigned int nRegs;
  nRegs = (rma->get_size() < MAX_REGISTERS) ? (rma->get_size()) : MAX_REGISTERS;

  if (!nRegs)
    return;

  gtk_sheet_freeze(register_sheet);

  j = 0;
  i = 0;

  gtk_sheet_REALLY_set_row_height(register_sheet, j, row_height(i));

  SetRegisterSize();

  for (reg_number = 0; reg_number < nRegs; reg_number++) {
    i = reg_number % REGISTERS_PER_ROW;

    if (i == 0 && row_created) {
      j++;
      row_created = FALSE;
    }

    GUIRegister *pGReg = registers->Get(reg_number);
    pGReg->row = j;
    pGReg->col = i;
    pGReg->put_shadow(RegisterValue(INVALID_VALUE, INVALID_VALUE));
    pGReg->bUpdateFull = true;

    if (pGReg->bIsValid()) {

      gpsim_set_bulk_mode(1);
      pGReg->put_shadow(registers->Get(reg_number)->getRV());
      gpsim_set_bulk_mode(0);

      /* Now create a cross-reference link that the simulator can use to
       * send information back to the gui
       */

      CrossReferenceToGUI *cross_reference = new RegisterWindowXREF();
      cross_reference->parent_window = (gpointer) this;
      cross_reference->data = (gpsimObject *) pGReg;
      pGReg->Assign_xref(cross_reference);

      if (!row_created)
      {
        char row_label[100];
        if (register_sheet->maxrow < j)
        {
          gtk_sheet_add_row(register_sheet, 1);
          gtk_sheet_REALLY_set_row_height(register_sheet, j, row_height(0));
        }

        g_snprintf(row_label, sizeof(row_label),
          "%x0", reg_number / REGISTERS_PER_ROW);

        gtk_sheet_row_button_add_label(register_sheet, j, row_label);
        gtk_sheet_set_row_title(register_sheet, j, row_label);

        row_to_address[j] = reg_number - reg_number % REGISTERS_PER_ROW;
        row_created = TRUE;
      }
    }
  }

  if (j < register_sheet->maxrow)
    gtk_sheet_delete_rows(register_sheet, j, register_sheet->maxrow - j);

  registers_loaded = 1;

  range.row0 = 0;
  range.rowi = register_sheet->maxrow;
  range.col0 = 0;
  range.coli = register_sheet->maxcol;

  UpdateStyle();
  border_mask = GTK_SHEET_RIGHT_BORDER |
    GTK_SHEET_LEFT_BORDER |
    GTK_SHEET_BOTTOM_BORDER |
    GTK_SHEET_TOP_BORDER;

  border_width = 1;

  gtk_sheet_range_set_border(register_sheet, &range, border_mask, border_width, 0);

  border_mask = GTK_SHEET_LEFT_BORDER;
  border_width = 3;

  range.col0 = REGISTERS_PER_ROW;
  range.coli = REGISTERS_PER_ROW;

  gtk_sheet_range_set_border(register_sheet, &range, border_mask, border_width, 0);

  gtk_sheet_thaw(register_sheet);

  // set values in the sheet
  Update();
  SelectRegister(0);
}

static int show_event(GtkWidget *, Register_Window *rw)
{
  Dprintf((" show_event\n"));

  rw->Update();
  return TRUE;
}

//------------------------------------------------------------------------
// Build
//
//

void Register_Window::Build()
{
  if (bIsBuilt)
    return;

  Dprintf((" Register_Window::Build()\n"));
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)

  char *fontstring;

  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 0);
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  if (type == REGISTER_RAM)
  {
    register_sheet = GTK_SHEET(gtk_sheet_new(1, MAXCOLS, "gpsim Register Viewer [RAM]"));
    gtk_window_set_title(GTK_WINDOW(window), "register viewer [RAM]");
  }
  else
  {
    register_sheet = GTK_SHEET(gtk_sheet_new(1, MAXCOLS, "gpsim Register Viewer [EEPROM]"));
    gtk_window_set_title(GTK_WINDOW(window), "register viewer [EEPROM]");
  }
  //gtk_sheet_hide_column_titles(register_sheet);
  //gtk_sheet_hide_row_titles(register_sheet);
  /* create popupmenu */
  popup_menu = build_menu();

  build_entry_bar(main_vbox, this);

  /**************************** load fonts *********************************/
#define DEFAULT_NORMALFONT "Monospace 10"
  normalfont_string = DEFAULT_NORMALFONT;
  if (config_get_string(name(), "normalfont", &fontstring))
      normalfont_string = fontstring;

  while (!LoadStyles())
  {
    if (gui_question("Some fonts did not load.", "Open font dialog", "Try defaults") == FALSE)
      {
      normalfont_string = DEFAULT_NORMALFONT;
      config_set_string(name(), "normalfont", normalfont_string.c_str());
      }
      else
      {
      SettingsDialog();
      }
  }
  UpdateStyle();

  g_signal_connect(window, "show", G_CALLBACK(show_event), this);

  scrolled_window = gtk_scrolled_window_new(0, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(register_sheet));

  GTK_SHEET_CLIP_TEXT(register_sheet);

  gtk_widget_show(GTK_WIDGET(register_sheet));

  gtk_widget_show(scrolled_window);

  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);

  // RP - I think this is wrong. The sheet's entry widget seems to get
  // replaced every time a new cell is selected, so this signal can't
  // work. Should it be hooked to the "changed" signal of the whole sheet?
  g_signal_connect(gtk_sheet_get_entry(GTK_SHEET(register_sheet)),
                     "changed", G_CALLBACK(show_entry), this);

  g_signal_connect(register_sheet,
                     "activate", G_CALLBACK(activate_sheet_cell),
                     this);

  g_signal_connect(entry,
                     "changed", G_CALLBACK(show_sheet_entry), this);

  g_signal_connect(entry,
                     "activate", G_CALLBACK(activate_sheet_entry),
                     this);

  g_signal_connect(register_sheet,
                     "key_press_event",
                     G_CALLBACK(clipboard_handler),
                     0);

  g_signal_connect(register_sheet,
                     "resize_range",
                     G_CALLBACK(resize_handler),
                     this);

  g_signal_connect(register_sheet,
                     "move_range",
                     G_CALLBACK(move_handler),
                     this);

  g_signal_connect(register_sheet, "button_press_event",
    G_CALLBACK(button_press), this);
  g_signal_connect(register_sheet, "popup-menu", G_CALLBACK(popup_menu_handler),
    this);

  g_signal_connect(register_sheet,
                     "set_cell",
                     G_CALLBACK(set_cell),
                     this);

  SetRegisterSize();

  gtk_widget_show(window);

  gtk_widget_grab_default(location);

  bIsBuilt = true;

  NewProcessor(gp);

  UpdateMenuItem();
  Dprintf((" regwin is built\n"));
}

//------------------------------------------------------------------------

Register_Window::Register_Window(GUI_Processor *_gp, REGISTER_TYPE _type, const char *name)
  : GUI_Object(name),
    normalfont(nullptr),
    current_line_number_style(nullptr), breakpoint_line_number_style(nullptr),
    type(_type),
    registers(nullptr), register_sheet(nullptr), rma(nullptr), entry(nullptr),
    location(nullptr),
    popup_menu(nullptr), registers_loaded(0), register_size(0),
    char_width(0), char_height(0), chars_per_column(3)

{
  gp = _gp;

  std::fill_n(row_to_address, MAX_ROWS, -1);
}

RAM_RegisterWindow::RAM_RegisterWindow(GUI_Processor *_gp) :
  Register_Window(_gp, REGISTER_RAM, "register_viewer_ram")
{
  menu = "/menu/Windows/Ram";

  if (enabled)
    Build();
}

void RAM_RegisterWindow::NewProcessor(GUI_Processor *_gp)
{
  if (!_gp || !_gp->cpu)
    return;

  rma = &_gp->cpu->rma;
  registers = _gp->m_pGUIRamRegisters;
  Dprintf((" RAM_RegisterWindow::NewProcessor rma=%p\n", rma));

  Register_Window::NewProcessor(_gp);
}

void RAM_RegisterWindow::Update()
{
  Register_Window::Update();
}

EEPROM_RegisterWindow::EEPROM_RegisterWindow(GUI_Processor *_gp) :
  Register_Window(_gp, REGISTER_EEPROM, "register_viewer_eeprom")
{
  menu = "/menu/Windows/EEPROM";

  if (enabled)
    Build();
}

void EEPROM_RegisterWindow::NewProcessor(GUI_Processor *_gp)
{
  if (!_gp || !_gp->cpu)
    return;

  rma = &_gp->cpu->ema;
  registers = _gp->m_pGUIEEPromRegisters;

  Dprintf((" EEPROM_RegisterWindow::NewProcessor rma=%p\n", rma));

  Register_Window::NewProcessor(_gp);
}

#endif // HAVE_GUI
