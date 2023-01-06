/*
   Copyright (C) 2004
   T. Scott Dattalo

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


#ifndef GUI_GUI_STATUSBAR_H_
#define GUI_GUI_STATUSBAR_H_

#include <gtk/gtk.h>
#include <vector>

class RegisterLabeledEntry;	// in gui_statusbar.cc
class MemoryAccess;		// in src/processor.h
class GUI_Processor;

//
// The Status Bar window
//

class StatusBar_Window {
public:
  explicit StatusBar_Window(GtkWidget *vbox_main);

  void NewProcessor(GUI_Processor *_gp, MemoryAccess *);
  void Update();

private:
  GUI_Processor *gp;
  MemoryAccess *ma;
  GtkWidget *hbox;
  std::vector<RegisterLabeledEntry *> entries;
};

#endif // GUI_GUI_STATUSBAR_H__
