/*
   Copyright (C) 1998 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

//-------------------------------------------------------------------
//                     interface.cc
//
// interface.cc provides a layer of code on top of the simulator
// portion of gpsim. It's purpose is to provide an abstract interface
// that hides the details of the simulator. Currently only the gui
// interfaces to gpsim through this layer. However, since the simulator
// 'engine' is built as a library, it's possible for other code to
// interface through here as well.
//
//-------------------------------------------------------------------

#include <glib.h>
#include <string.h>
#include <iostream>
#include <list>

#include "interface.h"

#include "../config.h"
#define GPSIM_VERSION VERSION

#include "cmd_manager.h"
#include "gpsim_classes.h"
#include "gpsim_interface.h"
#include "gpsim_time.h"
#include "icd.h"
#include "processor.h"
#include "sim_context.h"
#include "trace.h"
#include "ui.h"

class Integer;
class Module;
class Stimulus_Node;

extern Integer *verbosity;  // in ../src/init.cc

// Flag to tell us when all of the init stuff is done.
unsigned int gpsim_is_initialized = 0;

/**************************************************************************
 *
 *  Here's the gpsim interface class instantiation. It's through this class
 * that gpsim will notify the gui and/or modules of internal gpsim changes.
 *
 **************************************************************************/

gpsimInterface gi;

// create an instance of inline get_interface() method by taking its address
gpsimInterface &(*dummy_gi)() = get_interface;

//------------------------------------------------------------------------
// Temporary -- provide a flag to inihibit multithreaded support.
bool gUsingThreads()
{
  return false;
}

//---------------------------------------------------------------------------
//   void gpsim_set_bulk_mode(int flag)
//---------------------------------------------------------------------------
void gpsim_set_bulk_mode(int flag)
{
  if (get_use_icd()) {
    icd_set_bulk(flag);
  }
}

void initialization_is_complete()
{
  gpsim_is_initialized = 1;
}

//========================================================================
//========================================================================


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
  *            Module Interface
  *
  *
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

ModuleInterface::ModuleInterface(Module *new_module)
{
  module = new_module;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
  *            Processor Interface
  *
  *
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

ProcessorInterface::ProcessorInterface(Processor *new_cpu)
  : ModuleInterface(new_cpu)
{
}

//--------------------------------------------------------------------------
//
// callback registration functions
//
//
//--------------------------------------------------------------------------
Interface::Interface(void * new_object)
  : interface_id(0), objectPTR(new_object)
{
}


//--------------------------------------------------------------------------
//
// gpsimInterface
//
// Here are where the member functions for the gpsimInterface class are
// defined.
//
// The gpsimInterface class contains a singly-linked-list of Interface objects.
// Interface objects are structures that primarily contain pointers to a whole
// bunch of functions. The purpose is to have some external entity, like the
// gui code, define where these functions point. gpsim will then use these
// functions as a means of notifying the gui when something has changed.
// In addition to the gui, this class also provides the support for interfacing
// to modules. When a module is loaded from a module library, a new Interface
// object is created for it. The module will be given the opportunity to
// register functions (e.g. provide pointers to functions) that gpsim can
// then call.
//
//--------------------------------------------------------------------------


void gpsimInterface::update()
{
  for (const auto &an_interface : interfaces) {
    an_interface->Update(an_interface->objectPTR);
  }
}

void gpsimInterface::callback()
{
  if (update_rate) {
    future_cycle = get_cycles().get() + update_rate;
    get_cycles().set_break(future_cycle, this);
  }

  update();
}

void gpsimInterface::clear()
{
}

void gpsimInterface::print()
{
  std::cout << "Interface update rate " << update_rate << '\n';
}

void gpsimInterface::callback_print()
{
  std::cout << "gpsim Interface callback\n";
}

void update_gui()
{
  gi.update();
}

gpsimInterface::gpsimInterface()
  : socket_interface(nullptr), interface_seq_number(0),
    future_cycle(0), mbSimulating(false), mbUseGUI(false)
{
}

ISimConsole & gpsimInterface::GetConsole()
{
  // The static ISimConsole object is currently in the
  // CCommandManger class because it initially was used
  // to enable external modules to write to the console.
  // We may want to put it somewhere else someday.
  return CCommandManager::m_CommandManger.GetConsole();
}

//--------------------------------------------------------------------------
//
// A xref, or cross reference, object is an arbitrary thing that gpsim
// will pass back to the gui or module. The gui (or module) will then
// interpret the contents of the xref and possibly update some state
// with 'new_value'. An example is when one of the pic registers changes;
// if there's a xref object associated with the register gpsim will
// then notify the gui (or module) through the xref.

void gpsimInterface::update_object(void *xref, int new_value)
{
  for (const auto &an_interface : interfaces) {
    an_interface->UpdateObject(xref, new_value);
  }
}

void gpsimInterface::remove_object(void *xref)
{
  for (const auto &an_interface : interfaces) {
    an_interface->RemoveObject(xref);
  }
}

void gpsimInterface::simulation_has_stopped()
{
  profile_keeper.catchup();     // FIXME: remove this!

  for (const auto &an_interface : interfaces) {
    an_interface->SimulationHasStopped(an_interface->objectPTR);
  }
}

//========================================================================
//
void gpsimInterface::start_simulation(double /* duration */ )
{
  Processor *cpu = get_active_cpu();
  if (cpu) {
    mbSimulating = true;
    std::cout << "running...\n";
    cpu->run(true);
    mbSimulating = false;
    trace.dump_last_instruction();
    simulation_has_stopped();
  }
}

void gpsimInterface::step_simulation(int nSteps)
{
  Processor *cpu = get_active_cpu();

  if (cpu)
    cpu->step(nSteps);
}

void gpsimInterface::advance_simulation(eAdvancementModes nAdvancement)
{
  switch (nAdvancement)
  {
    case eAdvanceNextInstruction:
      {
        Processor *cpu = get_active_cpu();

        if (cpu)
          cpu->step_over();

      }
      break;
    case eAdvanceNextCycle:
    case eAdvanceNextCall:
    case eAdvanceNextReturn:
      break;
  }
}

void gpsimInterface::reset(RESET_TYPE resetType)
{
  CSimulationContext::GetContext()->Reset(resetType);
}

bool gpsimInterface::bSimulating()
{
  return mbSimulating;
}

bool gpsimInterface::bUsingGUI()
{
  return mbUseGUI;
}

void gpsimInterface::setGUImode(bool bnewGUImode)
{
  // We can only turn the gui on we can't turn it off.
  mbUseGUI |= bnewGUImode;
}

void gpsimInterface::new_processor(Processor *new_cpu)
{
  for (const auto &an_interface : interfaces) {
    an_interface->NewProcessor(new_cpu);
  }
}

void gpsimInterface::new_module(Module *module)
{
  for (const auto &an_interface : interfaces) {
    an_interface->NewModule(module);
  }
}

void gpsimInterface::node_configuration_changed(Stimulus_Node *node)
{
  for (const auto &an_interface : interfaces) {
    an_interface->NodeConfigurationChanged(node);
  }
}

void gpsimInterface::new_program(Processor *cpu)
{
  for (const auto &an_interface : interfaces) {
    an_interface->NewProgram(cpu);
  }
}

unsigned int gpsimInterface::add_interface(Interface *new_interface)
{
  interface_seq_number++;

  new_interface->set_id(interface_seq_number);
  interfaces.push_back(new_interface);

  return interface_seq_number;
}

unsigned int gpsimInterface::prepend_interface(Interface *new_interface)
{
  interface_seq_number++;

  new_interface->set_id(interface_seq_number);
  interfaces.push_front(new_interface);

  return interface_seq_number;
}

unsigned int gpsimInterface::add_socket_interface(Interface *new_interface)
{
  if (!socket_interface)
    return add_interface(new_interface);

  return 0;
}

void gpsimInterface::remove_interface(unsigned int interface_id)
{
  for (auto iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
    Interface *an_interface = *iter;
    if (an_interface->get_id() == interface_id) {
      if (an_interface == socket_interface)
        socket_interface = nullptr;

      interfaces.erase(iter);
      delete an_interface;
      return;
    }
  }
}

void gpsimInterface::set_update_rate(guint64 _update_rate)
{
  update_rate = _update_rate;

  if (update_rate) {
    guint64 fc = get_cycles().get() + _update_rate;

    if (fc) {
      if (future_cycle)
        get_cycles().reassign_break(future_cycle, fc, this);
      else
        get_cycles().set_break(fc, this);

      future_cycle = fc;
    }
  }
}

guint64 gpsimInterface::get_update_rate()
{
  return update_rate;
}

const char *get_dir_delim(const char *path)
{
#ifdef _WIN32
  const char *p = path + strlen(path);

  do
  {
    if (--p < path)
      return 0;
  }
  while (*p != '/' && *p != '\\');

  return p;
#else
  return strrchr(path, '/');
#endif
}

// For libgpsim.dll
// The console now owns the verbose flags. At some point as set
// of functions called TraceDisplayXXX() the conditionally
// display message depending on the verbose flags.
// I'll leave this it as it for now because I'm in the middle
// of making the src project its own DLL on windows and I have
// enough changes for now.
// Replaced the int verbose = 0; with GlobalVerbosityAccessor verbose.
// GlobalVerbosityAccessor that has overridden operators for 'if(verbose)'
// and 'if(verbose&4)' to still work as desired.
// The purpose was to decouple verbose from cli and gui. Now these
// modules (acutally gpsim.exe) will allocate there own
// GlobalVerbosityAccessor verbose object to gain access to the
// verbose flags and for the overridden operators.
GlobalVerbosityAccessor verbose;

