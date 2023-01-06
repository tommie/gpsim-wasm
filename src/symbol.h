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

//
// symbol.h
//
// gpsim Symbol Table
//
// The symbol table provides an interface to all named gpsim objects.
// Symbols can be added either automatically (i.e. gpsim adds global
// symbols for its own use), by Modules and Processors, or by users.
// In all cases, symbols placed into the symbol table are available
// for expressions, breakpoints, or querying.
//
// Implementation details:
//
// The gpsim symbol table relies heavily on the STL map. A map is
// created for each gpsim Module. An additional map is created for the
// global symbols. Each one of these maps can be accessed via the
// Module or via the Module name. The global symbols belong to no
// Module, however the map holding them is named '__global__'.
//
// In addition, there is another map for holding all of the
// Modules. This map together with all of the Module symbol tables
// form a two-level hierarchical symbol table.
//
// At the global scope (e.g. the command line), symbols are referred
// to by:
//
//      ModuleName.ModuleSymbolName
//
// For example, p16f873.foo refers to the symbol 'foo' in the p16f873
// module.
//
// A notion of a 'current module' exists. This feature is not fully
// implemented, but currently the most recently loaded Processor is
// designated as the current module. Symbols in the current module can
// be referenced with just the '.' scoping operator. For example, if
// p16f873 is the current module, then typing '.foo' will display
// p16f873.foo.
//
// The symbol table avoids interpreting or operating on the data it
// stores. For example, only the object's name is ever
// referenced. Clients wishing for specialized symbol manipulation are
// required to do the appropriate type casting. A mechanism based on
// the STL 'for_each' algorithm is provided to assist iterating
// through all of the symbols.


#ifndef SRC_SYMBOL_H_
#define SRC_SYMBOL_H_

#include <algorithm>
#include <string>
#include <map>
#include <utility>

#include "exports.h"

// Forward definitions
class gpsimObject;
class Integer;
class Value;
class Module;
class SymbolTable_t;
class SymbolTable;

typedef std::map<const char *, Module *> ModuleList_t;
typedef std::map<const std::string, SymbolTable_t *> MSymbolTable_t;
typedef void (*PFN_ForEachModule)(const std::pair<std::string, SymbolTable_t *> &st);
typedef void (*PFN_ForEachSymbol)(const std::pair<std::string, gpsimObject *> &sym);
typedef std::pair<std::string, SymbolTable_t *> SymbolTableEntry_t;
typedef std::pair<std::string, gpsimObject *> SymbolEntry_t;

//************************************************************************
// SymbolTable_t
//
// A gpsim symbol table is an STL map of gpsimObject pointers that are keyed with
// by the object's name.
//

class SymbolTable_t
{
  // The SymbolTable class has access to all map<>'s methods.
  friend class SymbolTable;
public:

  int addSymbol(gpsimObject *, std::string *AliasedName = nullptr);
  int removeSymbol(gpsimObject *);
  int removeSymbol(const std::string &);
  int deleteSymbol(const std::string &);
  gpsimObject *findSymbol(const std::string &);

  /// ForEachModuleSymbolTable -- thin wrapper around map<>'s for_each() algorithm.
  /// The pointer to the function passed must be declared like:
  /// void MyForEach(const SymbolEntry_t &sym) { /* do something with sym */ }
  /// Then invoked the algorithm by:
  ///   ASymbolTable.ForEachModuleSymbolTable(MyForEach);

  inline void ForEachSymbolTable(PFN_ForEachSymbol forEach)
  {
    for_each(table.begin(), table.end(), forEach);
  }

protected:
  // stiFound an iterator that points to the most recently found symbol.
  std::map<std::string, gpsimObject *>::iterator stiFound;

private:
  std::map<std::string, gpsimObject *> table;
};


//************************************************************************
//

class SymbolTable
{
public:
  SymbolTable();
  ~SymbolTable();

  /// Globally scoped symbols are added and removed here
  int addSymbol(gpsimObject *);
  int removeSymbol(gpsimObject *);
  int removeSymbol(const std::string &);
  int deleteSymbol(const std::string &);

  /// Each Module maintains its own symbol table. If the module wants
  /// its symbols to be accessed at the global scope, then the module
  /// has to add itself to the table.

  void addModule(Module *);
  void removeModule(Module *);
  void listModules(); // ugh

  /// find - search for a particular symbol
  gpsimObject *find(const std::string &);
  gpsimObject *findObject(gpsimObject *);

  ///
  void ForEachModule(PFN_ForEachModule forEach);

  /// Convenience functions for finding a symbol of a particular type:
  Value         *findValue(const std::string &);
  Integer       *findInteger(const std::string &);
  Module        *findModule(const std::string &);

protected:
  MSymbolTable_t MSymbolTables;
};



#if defined(_WIN32)
#if !defined(IN_MODULE)
extern SymbolTable  gSymbolTable;
#endif
// we are in Windows: don't access the symbol table object directly!
LIBGPSIM_EXPORT SymbolTable & globalSymbolTable();
#else
// we are in gpsim: use of getSymbolTable() is recommended,
// even if it can be accessed directly.
extern SymbolTable gSymbolTable;
inline SymbolTable &globalSymbolTable()
{
  return gSymbolTable;
}
#endif

#endif  // SRC_SYMBOL_H_
