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

#ifndef GUI_GUI_PROFILE_H_
#define GUI_GUI_PROFILE_H_

#include "gui_object.h"
#include <glib.h>
#include <gtk/gtk.h>

class GUI_Processor;

//
// The profile window
//

struct cycle_histogram_counter{
    // Three variables that determine which cycle_histogram_counter we add
    // the differences in cycle counter to:
    unsigned int start_address; // Start profile address
    unsigned int stop_address; // Stop profile address
    guint64 histo_cycles; // The number of cycles that this counter counts.

    unsigned int count; // The number of time 'cycles' cycles are used.
};

class Profile_Window : public GUI_Object
{
public:
  int program;    // if non-zero window has program

  GtkListStore *profile_list;
  GtkWidget *profile_tree;

  GtkListStore *profile_register_list;
  GtkWidget *profile_register_tree;

  GtkListStore *profile_exestats_list;
  GtkWidget *profile_exestats_tree;

  GtkWidget *notebook;

  // List of cycle_count structs
  GList *histogram_profile_list;

  explicit Profile_Window(GUI_Processor *gp);
  void Build() override;
  void Update() override;
  void NewProcessor(GUI_Processor *gp) override;
  virtual void NewProgram(GUI_Processor *gp);
  virtual void StopExe(int address);
  virtual void StartExe(int address);
};

#endif // GUI_GUI_PROFILE_H_

