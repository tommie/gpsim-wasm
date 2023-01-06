/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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


/*
  modules.h

  The base class for modules is defined here.

  Include this file into yours for creating custom modules.
 */


#ifndef SRC_MODULES_H_
#define SRC_MODULES_H_

#include <list>
#include <string>
#include <map>

#include "gpsim_object.h"
#include "gpsim_classes.h"
#include "symbol.h"

class Module;
class Module_Types;
class ModuleInterface;
class IOPIN;
class XrefObject;
class Package;
class ICommandHandler;

typedef  Module * (*Module_FPTR)();
typedef  Module_Types * (*Module_Types_FPTR)();


enum SIMULATION_MODES {
  eSM_INITIAL,
  eSM_STOPPED,
  eSM_RUNNING,
  eSM_SLEEPING,
  eSM_SINGLE_STEPPING,
  eSM_STEPPING_OVER,
  eSM_RUNNING_OVER
};


//------------------------------------------------------------------------
//
// ModuleLibrary
//
// A module library is an OS dependent dynamically loadable library of
// gpsim Modules. A Module (see below) can range from anything as simple
// as a resistor to as complicated as microcontroller. However, the interface
// to loading libraries and instantiating modules is kept simple:

class ModuleLibrary {
public:
  static int LoadFile(const std::string &sLibraryName);
  static int InstantiateObject(const std::string &sObjectName, const std::string &sInstantiatedName);
  static void ListLoadableModules();
};


/*
 * interface is a Module class member variable in gpsim,
 * in WIN32 Platform SDK it is a macro, defined in BaseTypes.h
 * the WIN32 Platform SDK definition should be undefined
 */

#ifdef interface
#undef interface
#endif

//------------------------------------------------------------------------
//
/// Module - Base class for all gpsim behavior models.

class Module : public gpsimObject {
public:
  Package  *package = nullptr;                // A package for the module
  ModuleInterface *interface = nullptr;       // An interface to the module.
  SIMULATION_MODES simulation_mode; // describes the simulation state for this module

  XrefObject *xref;                 // Updated when the module changes

  /// I/O pin specific

  virtual int get_pin_count();
  virtual std::string &get_pin_name(unsigned int pin_number);
  virtual int get_pin_state(unsigned int pin_number);
  virtual IOPIN *get_pin(unsigned int pin_number);
  virtual void assign_pin(unsigned int pin_number, IOPIN *pin);
  virtual void create_pkg(unsigned int number_of_pins);
  virtual double get_Vdd()
  {
    return Vdd;
  }
  virtual void set_Vdd(double v)
  {
    Vdd = v;
  }

  /// Symbols
  /// Each module has its own symbol table. The global symbol
  /// table can access this table too.
  SymbolTable_t & getSymbolTable()
  {
    return mSymbolTable;
  }
  int addSymbol(gpsimObject *, std::string *AliasedName = nullptr);
  gpsimObject *findSymbol(const std::string &);
  int removeSymbol(gpsimObject *);
  int removeSymbol(const std::string &);
  int deleteSymbol(const std::string &);
  int deleteSymbol(gpsimObject *);

  /// Registers - mostly processors, but can apply to complex modules
  virtual unsigned int register_mask() const
  {
    return 0xff;
  }
  virtual unsigned int register_size() const
  {
    return 1;
  }

  /// Reset

  virtual void reset(RESET_TYPE r);

  /// Version
  virtual char *get_version()
  {
    return version;
  }

  /// gui
  /// The simulation engine doesn't know anything about the gui.
  /// However, the set_widget and get_widget provide a mechanism
  /// for the gui to associate a pointer with a module.

  virtual void set_widget(void * a_widget)
  {
    widget = a_widget;
  }
  virtual void *get_widget()
  {
    return widget;
  }

  /// cli
  /// Modules can have gpsim CLI scripts associated with them.
  /// add_command will add a single CLI command to a script
  void add_command(const std::string &script_name, const std::string &command);

  /// run_script will pass a script to the gpsim CLI. This script
  /// executes immediately (i.e. it'll execute before any commands
  /// that may already be queued).
  void run_script(const std::string &script_name);

  const char *type()
  {
    return module_type.c_str();
  }
  void set_module_type(const std::string &type)
  {
    module_type = type;
  }

  Module(const char *_name = nullptr, const char *desc = nullptr);
  Module(const Module &) = delete;
  Module& operator =(const Module &) = delete;
  virtual ~Module();

  /// Functions to support actual hardware
  virtual bool isHardwareOnline()
  {
    return true;
  }

private:
  void *widget = nullptr;   // GtkWidget * that is put in the breadboard.
  std::string module_type;

  // Storage for scripts specifically associated with this module.
  class ModuleScript {
  public:
    explicit ModuleScript(const std::string &name_);

    void add_command(const std::string &command);
    void run(ICommandHandler &);

  private:
    // std::string name;
    std::list<std::string> m_commands;
  };

  std::map<std::string, ModuleScript> m_scripts;

protected:
  double	Vdd = 5.0;
  // Derived modules should assign more reasonable values for this.
  char *version = nullptr;
  SymbolTable_t mSymbolTable;
};

class Module_Types {
public:
  const char *names[2];
  Module * (*module_constructor)(const char *module_name);
};

#ifndef SWIG
const int Module_Types_Name_Count = sizeof(((Module_Types*)nullptr)->names) / sizeof(char*);


/**
  * CFileSearchPath
  * Implemented in os_dependent.cc
  */
using CFileSearchPath = std::list<std::string>;

/*****************************************************************************
 *
 * Helper functions
 *
 *****************************************************************************/
void GetFileName(const std::string &sPath, std::string &sName);
void GetFileNameBase(const std::string &sPath, std::string &sName);
void FixupLibraryName(std::string &sPath);
void * load_library(const char *library_name, const char **pszError);
void * get_library_export(const char *name, void *library_handle, const char **pszError);
void free_library(void *handle);
void free_error_message(const char * pszError);
#endif

class DynamicModuleLibraryInfo {
public:
  DynamicModuleLibraryInfo(const std::string &sCanonicalName,
                           const std::string &sUserSuppliedName,
                           void   *pHandle);

  inline std::string user_name()
  {
    return m_sCanonicalName;
  }
  inline Module_Types_FPTR mod_list()
  {
    return get_mod_list;
  }

protected:
  std::string m_sCanonicalName;
  std::string m_sUserSuppliedName;
  void *m_pHandle;
  Module_Types * (*get_mod_list)(void) = nullptr;
};

typedef std::map<std::string, DynamicModuleLibraryInfo *> ModuleLibraries_t;

#endif // SRC_MODULES_H_
