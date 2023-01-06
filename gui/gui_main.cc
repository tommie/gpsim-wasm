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

#include <clocale>

#include "../config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <glib.h>

#include "../src/gpsim_interface.h"
#include "../src/xref.h"

#include "gui.h"
#include "gui_breadboard.h"
#include "gui_processor.h"
#include "gui_profile.h"
#include "gui_regwin.h"
#include "gui_scope.h"
#include "gui_src.h"
#include "gui_stack.h"
#include "gui_stopwatch.h"
#include "gui_symbols.h"
#include "gui_trace.h"
#include "gui_watch.h"

#ifndef _WIN32
#undef TRUE
#undef FALSE
#include "settings_exdbm.h"
#else
#include "settings_reg.h"
#endif

class Module;
class Processor;
class Settings;
class Stimulus_Node;

extern int gui_animate_delay; // in milliseconds

/*
 * --- Function prototypes
 */
void init_link_to_gpsim(GUI_Processor *gp);
void link_src_to_gpsim(GUI_Processor *gp);

/*
 * --- Global variables
 */

GUI_Processor *gpGuiProcessor;
unsigned int interface_id;

Settings *settings;

extern GtkWidget *dispatcher_window;
extern void dispatch_Update();

//------------------------------------------------------------------------

CrossReferenceToGUI::CrossReferenceToGUI() : XrefObject()
{
}

CrossReferenceToGUI::~CrossReferenceToGUI()
{
}

void CrossReferenceToGUI::Remove()
{
}

//------------------------------------------------------------------------
//

class GUI_Interface : public Interface
{
private:
  GUI_Processor *gp;

public:
  explicit GUI_Interface(GUI_Processor *_gp);
  virtual ~GUI_Interface();

  void UpdateObject(gpointer xref, int new_value) override;
  void RemoveObject(gpointer xref) override;
  void SimulationHasStopped(gpointer object) override;
  void NewProcessor(Processor *) override;
  void NewModule(Module *module) override;
  void NodeConfigurationChanged(Stimulus_Node *node) override;
  void NewProgram(Processor *) override;
  void Update(gpointer object) override;
};


GUI_Interface::GUI_Interface(GUI_Processor *_gp)
  : Interface ( (gpointer *) _gp), gp(_gp)
{
}

GUI_Interface::~GUI_Interface()
{
  if (gp) {
    gp->regwin_ram->set_config();
    gp->regwin_eeprom->set_config();
    gp->program_memory->set_config();
    gp->source_browser->set_config();
    gp->watch_window->set_config();
    gp->stack_window->set_config();
    gp->breadboard_window->set_config();
    gp->trace_window->set_config();
    gp->profile_window->set_config();
    gp->stopwatch_window->set_config();
    gp->scope_window->set_config();
  }
}


/*------------------------------------------------------------------
 * UpdateObject
 *
 * Each 'thing' that the gui displays about a simulated pic has an
 * associated cross reference structure. Sometimes these 'things'
 * displayed in more than one place (like the status register).
 * Each graphical instance has its own structure. All of the structures
 * pertaining to the same pic object (again, like the status register)
 * are stored in a singly-linked list. This routine scans through
 * this list and updates each instance of the object.
 */

void GUI_Interface::UpdateObject(gpointer gui_xref, int new_value)
{
  CrossReferenceToGUI *xref = static_cast<CrossReferenceToGUI *>(gui_xref);
  xref->Update(new_value);
}

/*------------------------------------------------------------------
 * RemoveObject
 *
 */

void GUI_Interface::RemoveObject(gpointer gui_xref)
{
  CrossReferenceToGUI *xref = static_cast<CrossReferenceToGUI *>(gui_xref);
  xref->Remove();
}

extern int gui_animate_delay; // in milliseconds

/*------------------------------------------------------------------
 * SimulationHasStopped
 *
 */

void GUI_Interface::SimulationHasStopped(gpointer callback_data)
{
  if (callback_data) {
    GUI_Processor *lgp = static_cast<GUI_Processor *>(callback_data);

    while (gtk_events_pending())
      gtk_main_iteration();

    lgp->regwin_ram->Update();
    lgp->regwin_eeprom->Update();
    lgp->program_memory->Update();
    lgp->source_browser->Update();
    lgp->watch_window->Update();
    lgp->stack_window->Update();
    lgp->breadboard_window->Update();
    lgp->trace_window->Update();
    lgp->profile_window->Update();
    lgp->stopwatch_window->Update();
    lgp->scope_window->Update();

// TODO: should have an alternative way of doing a delay.
// This causes the GUI to become unresponsive.
    if (gui_animate_delay != 0)
      g_usleep(1000 * gui_animate_delay);

    dispatch_Update();
  }
}


/*------------------------------------------------------------------
 * NewProcessor - Add a new processor
 *
 * This routine adds another processor to the list of currently
 * simulated processors (as of 0.0.14 though, you're still limited
 * to a list of one). It then notifies each child window. Finally
 * a communication link between the gui and the simulator is established.
 */

void GUI_Interface::NewProcessor(Processor *new_cpu)
{
  // Create a gui representation of the new processor

  if (gp) {
    gp->SetCPU(new_cpu);

    gp->regwin_ram->NewProcessor(gp);
// RRR    gp->program_memory->NewProcessor(gp);
    gp->source_browser->CloseSource();
    gp->source_browser->NewProcessor(gp);
    gp->symbol_window->NewSymbols();
    //gp->watch_window->NewProcessor(gp);
    gp->breadboard_window->NewProcessor(gp);
    gp->stack_window->NewProcessor(gp);
    gp->trace_window->NewProcessor(gp);
    gp->profile_window->NewProcessor(gp);
    gp->stopwatch_window->NewProcessor(gp);
    //gp->scope_window->NewProcessor(gp);

    Dprintf((" New processor has been added"));
  }

}
/*------------------------------------------------------------------
 *
 */
void GUI_Interface::NewModule(Module *module)
{
  // FIX ME - need to search for *p in the gp list...
  if (gp)
    gp->breadboard_window->NewModule(module);
}

/*------------------------------------------------------------------
 *
 */
void GUI_Interface::NodeConfigurationChanged(Stimulus_Node *node)
{
  // FIX ME - need to search for *p in the gp list...
  if (gp)
    gp->breadboard_window->NodeConfigurationChanged(node);
}

/*------------------------------------------------------------------
 *
 */
void GUI_Interface::NewProgram(Processor *)
{
  if (gp) {
    gp->regwin_eeprom->NewProcessor(gp);

    gp->source_browser->CloseSource();
    gp->source_browser->NewSource(gp);
    gp->symbol_window->NewSymbols();
    gp->program_memory->NewSource(gp);
    gp->profile_window->NewProgram(gp);
    gp->watch_window->NewProcessor(gp);
    link_src_to_gpsim( gp);
  }
}


void GUI_Interface::Update(gpointer object)
{
  SimulationHasStopped(object);
}

/*------------------------------------------------------------------
 * quit_gui
 *
 */
void quit_gui()
{
  if (!get_interface().bUsingGUI())
    return;

  int x, y, width, height;

  gtk_window_get_position(GTK_WINDOW(dispatcher_window), &x, &y);
  gtk_window_get_size(GTK_WINDOW(dispatcher_window), &width, &height);

  config_set_variable("dispatcher", "enable", 1);
  config_set_variable("dispatcher", "x", x);
  config_set_variable("dispatcher", "y", y);
  config_set_variable("dispatcher", "width", width);
  config_set_variable("dispatcher", "height", height);

  get_interface().remove_interface(interface_id);

  gtk_main_quit();
}

/*------------------------------------------------------------------
 * gui_init
 *
 */

int gui_init(int argc, char **argv)
{
#ifndef _WIN32
  settings = new SettingsEXdbm("gpsim");
#else
  settings = new SettingsReg("gpsim");
#endif

  if (!gtk_init_check(&argc, &argv)) {
    return -1;
  }

  // standardize IO format ref bug 1832702
  setlocale(LC_NUMERIC, "C");

  gpGuiProcessor = new GUI_Processor();
  interface_id = get_interface().add_interface(new GUI_Interface(gpGuiProcessor));

  return 0;
}

void gui_main()
{
  gtk_main ();
}

#endif //HAVE_GUI
