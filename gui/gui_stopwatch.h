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

#ifndef GUI_GUI_STOPWATCH_H_
#define GUI_GUI_STOPWATCH_H_

#include "gui_object.h"
#include <gtk/gtk.h>
#include <cassert>

class GUI_Processor;

//
// The stopwatch window
//
class StopWatch_Window : public GUI_Object
{
 public:
  int count_dir;

  long long rollover;
  long long cyclecounter;
  long long offset;

  GtkWidget *cycleentry;
  GtkWidget *timeentry;
  GtkWidget *frequencyentry;
  GtkWidget *offsetentry;
  GtkWidget *rolloverentry;

  GtkWidget *option_menu;

  explicit StopWatch_Window(GUI_Processor *gp);
  void Build() override;
  void Update() override;

  void EnterUpdate()
  {
    assert(from_update >= 0);
    ++from_update;
  }

  void ExitUpdate()
  {
    assert(from_update > 0);
    --from_update;
  }

  bool IsUpdate() const
  {
    assert(from_update >= 0);
    return from_update != 0;
  }

  // Signal call-backs
  static void modepopup_activated(GtkWidget *widget, StopWatch_Window *sww);
  static void zero_cb(GtkWidget *w, StopWatch_Window *sww);
  static void cyclechanged(GtkWidget *widget, StopWatch_Window *sww);
  static void offsetchanged(GtkWidget *widget, StopWatch_Window *sww);
  static void rolloverchanged(GtkWidget *widget, StopWatch_Window *sww);

private:
  int from_update;
  long long cyclecounter_last;
};

#endif // GUI_GUI_STOPWATCH_H_
