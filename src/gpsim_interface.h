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
#include "trigger.h"

#include <list>
#include <string>

#include <glib.h>

class Stimulus;
class Stimulus_Node;
class ISimConsole;
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
   * UpdateObject - Invoked when an object changes
   *
   * If an object, like the contents of a register, changes then this function
   * will be called. There are two parameters:
   *  xref - this is a pointer to some structure in the client's data space.
   *  new_value - this is the new value to which the object has changed.
   *
   */

  virtual void UpdateObject(void * /* xref */, int /* new_value */) {}

  /*
   * remove_object - Invoked when gpsim has removed something.
   *
   * If an object, like a register, is deleted then this function
   * will be called. There is one parameter:
   *  xref - this is a pointer to some structure in the client's data space.
   *
   */

  virtual void RemoveObject(void * /* xref */) {}


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
   * new_program - Invoked when a new program is loaded into gpsim
   *
   */

  virtual void NewProgram(Processor *) {}

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
   * start_simulation -begin simulating. Simulation stops whenever
   *  a breakpoint is encountered or the specified duration has expired.
   */
  void start_simulation(double duration = 0.0);

  /*
   * step_simulation - run the simulation for one or more simulation cycles.
   */
  void step_simulation(int nSteps);

  /*
   * advance_simulation - run simulation until advancement condition is met.
   */
  enum eAdvancementModes {
    eAdvanceNextInstruction, // processors - step over call instructions
    eAdvanceNextCycle,       // system -
    eAdvanceNextCall,        // processors - run until call instruction
    eAdvanceNextReturn,      // processors - run until next return
  };
  void advance_simulation(eAdvancementModes nAdvancement);
  void reset(RESET_TYPE resetType = SIM_RESET);
  void simulation_has_stopped();
  bool bSimulating();
  bool bUsingGUI();
  void setGUImode(bool);
  // gpsim will call these functions to notify gui and/or modules
  // that something has changed.

  void update_object(void * xref, int new_value);
  void remove_object(void * xref);
  void update();
  void new_processor(Processor *);
  void new_module(Module *module);
  void node_configuration_changed(Stimulus_Node *node);
  void new_program(Processor *);
  void set_update_rate(guint64 rate);
  guint64 get_update_rate();

  unsigned int add_interface(Interface *new_interface);
  unsigned int prepend_interface(Interface *new_interface);
  unsigned int add_socket_interface(Interface *new_interface);
  Interface *get_socket_interface() { return socket_interface;}
  void remove_interface(unsigned int interface_id);


  bool set_break() override { return false; }
  void callback() override;
  void callback_print() override;
  void print() override;
  void clear() override;
  char const * bpName() override { return "gpsim interface"; }
  virtual ISimConsole &GetConsole();

private:
  std::list<Interface *> interfaces;
  Interface *socket_interface;

  unsigned int interface_seq_number;

  guint64 update_rate;
  guint64 future_cycle;

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

#define GPSIM_GETCOMMANDHANDLER "GetCommandHandler"

class ICommandHandler {
public:
  virtual ~ICommandHandler()
  {
  }

  virtual const char *GetName() = 0;
  // Fixme: should Execute be renamed ExecuteCommand?
  virtual int Execute(const char * commandline, ISimConsole *out) = 0;
  virtual int ExecuteScript(const std::list<std::string> &script, ISimConsole *out) = 0;
};

#endif // SRC_GPSIM_INTERFACE_H_
