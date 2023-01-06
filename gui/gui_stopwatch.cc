/*
   Copyright (C) 2000,2001
    Ralf Forsberg

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

#include <cstdlib>

#include "../config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>

#include "../src/gpsim_time.h"
#include "../src/processor.h"

#include "gui.h"
#include "gui_processor.h"
#include "gui_stopwatch.h"

void StopWatch_Window::Update()
{
  char frequencystring[100];
  char cyclestring[100];
  char timestring[100];
  char offsetstring[100];
  char rolloverstring[100];

  if (!gp || !gp->cpu)
    return;
  if (!enabled)
    return;

  if (!bIsBuilt)
    Build();

  if (rollover <= 0)
    rollover = 1;

  if (offset > rollover)
    offset %= rollover;

  double frequency = gp->cpu->get_frequency();
  unsigned int cycle_per_inst = gp->cpu->get_ClockCycles_per_Instruction();

  long long _cyclecounter = cyclecounter;

  ////////////////////////

  if (count_dir < 0)
    _cyclecounter -= get_cycles().get() - cyclecounter_last;
  else
    _cyclecounter += get_cycles().get() - cyclecounter_last;

  cyclecounter_last = get_cycles().get();


  // %%% FIXME %%% - This surely must be wrong, given that we're working with
  // the local copy ('_cyclecounter')
  while (cyclecounter < offset)
    cyclecounter += rollover;

  cyclecounter = _cyclecounter;   // See! We overwrite what we just did

  _cyclecounter = (_cyclecounter - offset) % rollover;
  ////////////////////////

  double timevalue = (_cyclecounter * 1000000 * cycle_per_inst) / frequency;

  if (frequency < 1e6)
     g_snprintf(frequencystring, sizeof(frequencystring), "%.3f kHz", frequency / 1e3);
  else
     g_snprintf(frequencystring, sizeof(frequencystring), "%.3f MHz", frequency / 1e6);

  g_snprintf(cyclestring, sizeof(cyclestring), "%Ld", _cyclecounter);
  if (timevalue < 1000)
    g_snprintf(timestring, sizeof(timestring), "%.2f us", timevalue);
  else if (timevalue < 1000000)
    g_snprintf(timestring, sizeof(timestring), "%.3f ms", timevalue / 1000.0);
  else if (timevalue < 1000000000)
    g_snprintf(timestring, sizeof(timestring), "%.3f s", timevalue / 1000000.0);
  else
    {
      double v = timevalue / 1000000.0;
      int hh = v / 3600;
      v -= hh * 3600.0;
      int mm = v / 60;
      v -= mm * 60.0;
      int ss = v;
      g_snprintf(timestring, sizeof(timestring), "    %02dh %02dm %02ds", hh, mm, ss);
    }
  g_snprintf(offsetstring, sizeof(offsetstring), "%Ld", offset);
  g_snprintf(rolloverstring, sizeof(rolloverstring), "%Ld", rollover);

  EnterUpdate();
  gtk_entry_set_text(GTK_ENTRY(frequencyentry), frequencystring);
  gtk_entry_set_text(GTK_ENTRY(cycleentry), cyclestring);
  gtk_entry_set_text(GTK_ENTRY(timeentry), timestring);
  gtk_entry_set_text(GTK_ENTRY(offsetentry), offsetstring);
  gtk_entry_set_text(GTK_ENTRY(rolloverentry), rolloverstring);
  ExitUpdate();
}

void StopWatch_Window::zero_cb(GtkWidget *, StopWatch_Window *sww)
{
  sww->offset = sww->cyclecounter;

  sww->Update();
}

void
StopWatch_Window::modepopup_activated(GtkWidget *, StopWatch_Window *sww)
{
  gint dir = gtk_combo_box_get_active(GTK_COMBO_BOX(sww->option_menu));
  switch(dir)
  {
  case 0:
    sww->count_dir = 1;
    config_set_variable(sww->name(), "count_dir", sww->count_dir);
    break;

  case 1:
    sww->count_dir = -1;
    config_set_variable(sww->name(), "count_dir", sww->count_dir);
    break;

  default:
    break;
  }

  sww->Update();
}

void
StopWatch_Window::cyclechanged(GtkWidget *widget, StopWatch_Window *sww)
{
  if (!sww->IsUpdate())
  {
    const char *text = gtk_entry_get_text(GTK_ENTRY(widget));
    long long v = ::strtoll(text, 0, 10);
    if (v != (sww->cyclecounter - sww->offset) % sww->rollover)
    {
      v = (v + sww->offset) % sww->rollover;

      sww->cyclecounter = v;
      sww->Update();
    }
  }
}

void
StopWatch_Window::offsetchanged(GtkWidget *widget, StopWatch_Window *sww)
{
  if (!sww->IsUpdate())
  {
    const char *text = gtk_entry_get_text(GTK_ENTRY(widget));
    long long v = ::strtoll(text, 0, 10);

    if (v != sww->offset)
    {
      sww->offset=v;
      sww->Update();
    }
  }
}

void
StopWatch_Window::rolloverchanged(GtkWidget *widget, StopWatch_Window *sww)
{
  if (!sww->IsUpdate())
  {
    const char *text = gtk_entry_get_text(GTK_ENTRY(widget));
    long long v = ::strtoll(text, 0, 10);

    if (v != sww->rollover)
    {
      sww->rollover=v;
      config_set_string(sww->name(), "rollover", text);
      sww->Update();
    }
  }
}


void StopWatch_Window::Build()
{
  if (bIsBuilt)
    return;

  GtkWidget *vbox, *button, *label, *entry;
  GtkWidget *table;

  gtk_window_set_title(GTK_WINDOW(window), "StopWatch");

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  table = gtk_table_new(6, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

  label = gtk_label_new("Cycles");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label = gtk_label_new("Time");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label = gtk_label_new("Processor frequency");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cycleentry = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  g_signal_connect(entry, "changed",
    G_CALLBACK(StopWatch_Window::cyclechanged), this);

  timeentry = entry = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_widget_set_sensitive(entry, FALSE);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  frequencyentry = entry = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_widget_set_sensitive(entry, FALSE);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label = gtk_label_new("Count direction");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  option_menu = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(option_menu), "Up");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(option_menu), "Down");
  gtk_combo_box_set_active(GTK_COMBO_BOX(option_menu), (count_dir > 0) ? 0 : 1);
  g_signal_connect(option_menu, "changed",
    G_CALLBACK(StopWatch_Window::modepopup_activated), this);
  gtk_table_attach(GTK_TABLE(table), option_menu, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label = gtk_label_new("Cycle offset");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  offsetentry = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  g_signal_connect(entry, "changed",
    G_CALLBACK(StopWatch_Window::offsetchanged), this);

  label = gtk_label_new("Rollover");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  rolloverentry = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  g_signal_connect(entry, "changed",
    G_CALLBACK(StopWatch_Window::rolloverchanged), this);

  button = gtk_button_new_with_label("Zero");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 4);
  g_signal_connect(button, "clicked",
    G_CALLBACK(StopWatch_Window::zero_cb), this);

  gtk_widget_show_all(window);

  bIsBuilt = true;

  UpdateMenuItem();
  Update();
}

//------------------------------------------------------------------------
//
//
StopWatch_Window::StopWatch_Window(GUI_Processor *_gp)
  : GUI_Object("stopwatch_viewer"),
    count_dir(1), rollover(1000000), cyclecounter(0),
    offset(0), from_update(0), cyclecounter_last(0)
{
  char *string;

  menu = "/menu/Windows/Stopwatch";

  gp = _gp;

  if (config_get_string(name(), "rollover", &string))
    rollover = ::strtoll(string, 0, 10);
  config_get_variable(name(), "count_dir", &count_dir);

  if (enabled)
    Build();
}


#endif // HAVE_GUI
