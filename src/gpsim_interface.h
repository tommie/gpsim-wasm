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

#ifndef SRC_GPSIM_INTERFACE_H_
#define SRC_GPSIM_INTERFACE_H_

#include "gpsim_classes.h"
#include "interface.h"
#include "sim_context.h"
#include "trigger.h"

#include <list>
#include <string>

class Stimulus;
class Stimulus_Node;
class Processor;
class Module;

//---------------------------------------------------------------------------
//
//  struct Interface
//
//  Here's a structure containing all of the information for gpsim
// and an external interface like the GUI to communicate. There are
// two major functions. First, a GUI can provide pointers to callback
// functions that the simulator can invoke upon certain conditions.
// For example, if the contents of a register change, the simulator
// can notify the GUI and thus save the gui having to continuously
// poll. (This may occur when the command line interface changes
// something; such changes need to be propogated up to the gui.)
//
// FIXME -- shouldn't this be a pure virtual class?

class Interface
{
public:
  explicit Interface(void * new_object = nullptr);
  /*
   * destructor - called when the interface is destroyed - this gives
   *  the interface object a chance to save state information.
   */
  virtual ~Interface()
  {
  }


  /*
   * simulation_has_stopped - invoked when gpsim has stopped simulating (e.g.
   *                          when a breakpoint is encountered).
   *
   * Some interfaces have more than one instance, so the 'object' parameter
   * allows the interface to uniquely identify the particular one.
   */

  virtual void SimulationHasStopped(void * /* object */) {}


  /*
   * new_processor - Invoked when a new processor is added to gpsim
   */

  virtual void NewProcessor(Processor *) {}


  /*
   * new_module - Invoked when a new module is instantiated.
   *
   */

  virtual void NewModule(Module *) {}

  /*
   * node_configuration_changed - invoked when stimulus configuration has changed
   */

  virtual void NodeConfigurationChanged(Stimulus_Node *) {}

  /*
   * Update - Invoked when the interface should update itself
   */

  virtual void Update(void * /* object */) {}

  unsigned int get_id() { return interface_id; }
  void set_id(unsigned int new_id) { interface_id = new_id; }

  unsigned int interface_id;
  void * objectPTR;
};


class gpsimInterface : public TriggerObject
{
public:
  gpsimInterface();

  /*
   * step_simulation - run the simulation for one or more simulation cycles.
   */
  void step_simulation(std::function<bool(unsigned int)> cond);

  void reset(RESET_TYPE resetType = SIM_RESET);
  void simulation_has_stopped();
  bool bSimulating();
  bool bUsingGUI();
  void setGUImode(bool);
  // gpsim will call these functions to notify gui and/or modules
  // that something has changed.

  void update();
  void new_processor(Processor *);
  void new_module(Module *module);
  void node_configuration_changed(Stimulus_Node *node);
  void new_program(Processor *);
  void set_update_rate(uint64_t rate);
  uint64_t get_update_rate();

  unsigned int add_interface(Interface *new_interface);
  unsigned int prepend_interface(Interface *new_interface);
  unsigned int add_socket_interface(Interface *new_interface);
  Interface *get_socket_interface() { return socket_interface;}
  void remove_interface(unsigned int interface_id);


  bool enable_break() override { return false; }
  void callback() override;
  void callback_print() override;
  void print() override;
  void clear() override;
  char const * bpName() override { return "gpsim interface"; }

  CSimulationContext& simulation_context() { return sim_context; }

private:
  CSimulationContext sim_context;
  std::list<Interface *> interfaces;
  Interface *socket_interface;

  unsigned int interface_seq_number;

  uint64_t update_rate;
  uint64_t future_cycle;

  bool mbSimulating;   // Set true if the simulation is running.
  bool mbUseGUI;       // Set true if gui is being used.
};

#if defined(IN_MODULE) && defined(_WIN32)
// we are in a module: don't access gi object directly!
LIBGPSIM_EXPORT gpsimInterface & get_interface();
#else
// we are in gpsim: use of get_interface() is recommended,
// even if gi object can be accessed directly.
extern gpsimInterface gi;

inline gpsimInterface &get_interface()
{
  return gi;
}
#endif

//------------------------------------------------------------------------

class ModuleInterface
{
public:
  Module *module;  // The module we're interfacing with.

  explicit ModuleInterface(Module *new_module);
  virtual ~ModuleInterface() = default;
};

//------------------------------------------------------------------------

class ProcessorInterface : public ModuleInterface
{
public:
  explicit ProcessorInterface(Processor *cpu);
};

#define CMD_ERR_OK                    0
#define CMD_ERR_ABORTED               1
#define CMD_ERR_ERROR                 2
#define CMD_ERR_PROCESSORDEFINED      3
#define CMD_ERR_PROCESSORNOTDEFINED   4
#define CMD_ERR_COMMANDNOTDEFINED     5
#define CMD_ERR_NOTIMPLEMENTED        6

#endif // SRC_GPSIM_INTERFACE_H_
