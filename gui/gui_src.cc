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

#include <typeinfo>

#include <config.h>
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#include "gui_src.h"
#include "gui_processor.h"

#include "../src/gpsim_interface.h"
#include "../src/pic-instructions.h"
#include "../src/processor.h"
#include "../src/value.h"

static gint
key_press(GtkWidget *, GdkEventKey *key, gpointer data)
{
  SourceBrowser_Window *sbw = static_cast<SourceBrowser_Window *>(data);

  if (!sbw) return FALSE;
  if (!sbw->pma) return FALSE;
  if (!sbw->gp) return FALSE;
  if (!sbw->gp->cpu) return FALSE;

  switch (key->keyval) {
  case 's':
  case 'S':
  case GDK_KEY_F7:
    sbw->pma->step(1);
    break;

  case 'o':
  case 'O':
  case 'n':
  case GDK_KEY_F8:
    sbw->pma->step_over();
    break;

  case 'r':
  case 'R':
  case GDK_KEY_F9:
    get_interface().start_simulation();
    break;

  case GDK_KEY_Escape:
    sbw->pma->stop();
    break;

  case 'f':
  case 'F':
    sbw->pma->finish();
    break;

  default:
    return FALSE;
  }

  return TRUE;
}

SourceBrowser_Window::SourceBrowser_Window(const char *name)
  : GUI_Object(name)
{
  gtk_container_set_border_width(GTK_CONTAINER(window), 0);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* Add a signal handler for key press events. This will capture
   * key commands for single stepping, running, etc.
   */
  g_signal_connect(window, "key_press_event",
                     G_CALLBACK(key_press),
                     (gpointer) this);
}

void SourceBrowser_Window::Update()
{
  if (!gp || !gp->cpu)
    return;

  SetPC(gp->cpu->pma->get_PC());
}

void SourceBrowser_Window::Create()
{
  last_simulation_mode = eSM_INITIAL;
}

void SourceBrowser_Window::SetTitle() {

  if (!gp->cpu || !pma) {
      return;
  }

  if (last_simulation_mode != eSM_INITIAL &&
    ((last_simulation_mode == eSM_RUNNING &&
    gp->cpu->simulation_mode == eSM_RUNNING) ||
    (last_simulation_mode != eSM_RUNNING &&
    gp->cpu->simulation_mode != eSM_RUNNING)) &&
    sLastPmaName == pma->name()) {
      return;
  }

  last_simulation_mode = gp->cpu->simulation_mode;
  const char * sStatus;
  if (gp->cpu->simulation_mode == eSM_RUNNING)
    sStatus = "Run";
  else // if (gp->cpu->simulation_mode == eSM_STOPPED)
    sStatus = "Stopped";

  char *buffer = g_strdup_printf("Source Browser: [%s] %s", sStatus, pma->name().c_str());
  sLastPmaName = pma->name();
  gtk_window_set_title(GTK_WINDOW(window), buffer);
  g_free(buffer);
}

void SourceBrowser_Window::SelectAddress(Value *addrSym)
{

  if (typeid(*addrSym) == typeid(LineNumberSymbol) ||
     typeid(*addrSym) == typeid(AddressSymbol)) {

    int i;
    addrSym->get(i);
    SelectAddress(i);
  }

}

#endif // HAVE_GUI
