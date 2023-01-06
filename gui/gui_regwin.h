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


#ifndef GUI_GUI_REGWIN_H_
#define GUI_GUI_REGWIN_H_

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "../src/interface.h"
#include "gui_object.h"
#include "gui_register.h"
#include "gtkextra/gtksheet.h"

#include <string>

class GUI_Processor;
class RegisterMemoryAccess;
class Value;

//======================================================================
// The register window
//
#define REGISTERS_PER_ROW    16
#define MAX_ROWS ((MAX_REGISTERS)/(REGISTERS_PER_ROW))

// Base class for RAM_RegisterWindow and EEPROM_RegisterWindow

class Register_Window : public GUI_Object
{
 public:

  // This array is indexed with row, and gives the address of the
  // first cell in the given row.
  int row_to_address[MAX_ROWS];

  std::string normalfont_string;
  PangoFontDescription *normalfont;

  GtkStyle *current_line_number_style;
  GtkStyle *breakpoint_line_number_style;

  REGISTER_TYPE type;
  GUIRegisterList *registers;
  GtkSheet *register_sheet;

  RegisterMemoryAccess *rma;  // Apointer to the Processor's rma or ema.

  GtkWidget *entry;
  GtkWidget *location;
  GtkWidget *popup_menu;

  int registers_loaded; // non zero when registers array is loaded


  void Build() override;
  int LoadStyles();
  int SettingsDialog();
  void UpdateStyle();
  void SetRegisterSize();
  void Update() override;
  virtual void UpdateASCII(int row);
  virtual void UpdateLabel();
  virtual void UpdateEntry();
  virtual void UpdateLabelEntry();
  virtual void SelectRegister(Value *);
  virtual void SelectRegister(int reg_number);
  void NewProcessor(GUI_Processor *gp) override;
  virtual GUIRegister *getRegister(int row, int col);
  virtual gboolean UpdateRegisterCell(int reg_number);
  GUIRegister *operator [] (int address);
  int column_width(int col);
  int row_height(int row);

protected:
  Register_Window(GUI_Processor *gp, REGISTER_TYPE type, const char *name);

 private:
  GtkWidget *build_menu();
  void do_popup(GtkWidget *widget, GdkEventButton *event);
  static gboolean button_press(GtkWidget *widget, GdkEventButton *event,
    Register_Window *rw);
  static gboolean popup_menu_handler(GtkWidget *widget, Register_Window *rw);

  // Formatting
  int register_size;    // The size (in bytes) of a single register
  int char_width;       // nominal character width.
  int char_height;      // nominal character height
  int chars_per_column; // width of 1 column
};

class RAM_RegisterWindow : public Register_Window
{
 public:
  explicit RAM_RegisterWindow(GUI_Processor *gp);
  void NewProcessor(GUI_Processor *gp) override;
  void Update() override;
};

class EEPROM_RegisterWindow : public Register_Window
{
 public:
  explicit EEPROM_RegisterWindow(GUI_Processor *gp);
  void NewProcessor(GUI_Processor *gp) override;
};


#endif // GUI_GUI_REGWIN_H_
