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
// symbol.cc
//
//  The file contains the code that controls all of the symbol
// stuff for gpsim. Most of the work is handled by the C++ map
// container class.
//
#include "symbol.h"

#include <algorithm>
#include <iostream>
#include <string>

#include "gpsim_object.h"
#include "modules.h"
#include "value.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

SymbolTable   gSymbolTable;
SymbolTable_t globalSymbols;
static SymbolTable_t *currentSymbolTable;

#define DEBUG 0

#if defined(_WIN32)
SymbolTable &globalSymbolTable()
{
  return gSymbolTable;
}
#endif


//-------------------------------------------------------------------
//-------------------------------------------------------------------
int SymbolTable_t::addSymbol(gpsimObject *pSym, std::string *ps_AliasedName)
{
    if (pSym)
    {
        ps_AliasedName = (ps_AliasedName && !ps_AliasedName->empty()) ? ps_AliasedName : &pSym->name();
        auto sti = table.find(*ps_AliasedName);
        if (sti == table.end())
        {
            table[*ps_AliasedName] = pSym;
            return 1;
        }
        else if (pSym != sti->second)
        {
            std::cout << "SymbolTable_t::addSymbol " << *ps_AliasedName << " exists " << pSym << ' ' << sti->second << '\n';
            return 0;
        }
    }
    return 0;
}

int SymbolTable_t::removeSymbol(gpsimObject *pSym)
{
    if (pSym)
    {
        auto it = find_if(table.begin(), table.end(),
            [pSym](const SymbolEntry_t &se){ return se.second == pSym; });

        if (it != table.end())
        {
            if (DEBUG)
            {
                std:: cout << __FILE__ ":" STR(__LINE__) " removing symbol ";

                if (pSym)
                    std::cout << pSym->name() << '\n';
            }
            table.erase(it);
            return 1;
        }
    }
    return 0;
}

int SymbolTable_t::removeSymbol(const std::string &s)
{
    auto sti = table.find(s);
    if (sti != table.end())
    {
        if (DEBUG)
            std::cout << __FILE__ ":"  STR(__LINE__) " Removing symbol " << s << '\n';

        table.erase(sti);
        return 1;
    }

    return 0;
}

int SymbolTable_t::deleteSymbol(const std::string &s)
{
    auto sti = table.find(s);
    if (sti != table.end())
    {
        if (DEBUG)
            std::cout << __FILE__ ":" STR(__LINE__) "  Deleting symbol " << s << '\n';

        delete sti->second;
        table.erase(sti);
        return 1;
    }

    return 0;
}


gpsimObject *SymbolTable_t::findSymbol(const std::string &searchString)
{
    stiFound = table.find(searchString);
    return stiFound != table.end() ? stiFound->second : nullptr;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------


SymbolTable::SymbolTable()
{
  MSymbolTables["__global__"] = &globalSymbols;
  currentSymbolTable = &globalSymbols;
}


static void dumpOneSymbol(const SymbolEntry_t &sym)
{
  std::cout << "  " //<< sym.second->name()  // name may not be valid.
       << " stored as " << sym.first
       << " pointer:" << sym.second
       << "  Type:" << sym.second->showType()
       << '\n';
}

static void dumpSymbolTables(const SymbolTableEntry_t &st)
{
  std::cout << " Symbol Table: " << st.first << '\n';
  (st.second)->ForEachSymbolTable(dumpOneSymbol);
}

SymbolTable::~SymbolTable()
{
  if (DEBUG) {
    std::cout << "Deleting the symbol table, here's what is still left in it:\n";
    ForEachModule(dumpSymbolTables);
  }
}

int SymbolTable::addSymbol(gpsimObject *pSym)
{
    return globalSymbols.addSymbol(pSym);
}

int SymbolTable::removeSymbol(gpsimObject *pSym)
{
    return globalSymbols.removeSymbol(pSym);
}


void SymbolTable::addModule(Module *pModule)
{
  if (pModule) {
    MSymbolTables[pModule->name()] = &pModule->getSymbolTable();
    globalSymbols.addSymbol(pModule);
  }
}

void SymbolTable::removeModule(Module *pModule)
{
  if (pModule) {
    //cout << "Removing " << pModule->name() << " from the global symbol table\n";
    MSymbolTable_t::iterator mi = MSymbolTables.find(pModule->name());
    if (mi != MSymbolTables.end())
      MSymbolTables.erase(mi);
    globalSymbols.removeSymbol(pModule);
  }
}

static  SymbolTable_t *searchTable;

gpsimObject *SymbolTable::find(const std::string &s)
{
  // First check scoping
  //
  //   SymbolTableName.SymbolName
  //                  ^
  //                  |
  //                  +---  scoping operator
  //
  // If the search string contains the scoping operator (i.e. '.')
  // then the symbol table specified by the scope. In other words,
  // if the search string begins with a period, then search the
  // current symbol table.
  // If the search string contains a period, then search for the
  // symbol table named with the string to the left of the period
  // and if that table is found search in it for the string to
  // the right of the period.

    const char scopeOperator = '.';

    size_t scopeOperatorPosition = s.find_first_of(scopeOperator);
    if (scopeOperatorPosition != std::string::npos)
    {
        searchTable = &globalSymbols;
        if (scopeOperatorPosition == 0)   // Select the current symbol table
        {
            searchTable = currentSymbolTable;
            scopeOperatorPosition++;
        }
        else
        {
            // Find the symbol table with the scoped name:
            std::string moduleName = s.substr(0, scopeOperatorPosition);
            MSymbolTable_t::iterator mti = MSymbolTables.find(moduleName);
            if (mti != MSymbolTables.end()) {
                searchTable = mti->second;
                scopeOperatorPosition++;
            }
        }
        auto sti = searchTable->table.find(s.substr(scopeOperatorPosition));
        if (sti != searchTable->table.end())
            return sti->second;
    }

    gpsimObject *pFound = nullptr;  // assume the symbol is not found.

    auto mti = find_if(MSymbolTables.begin(), MSymbolTables.end(),
        [&s, &pFound](const std::pair<const std::string, SymbolTable_t *> &st)
        {
            pFound = st.second->findSymbol(s);
            return pFound != nullptr;
        });

    if (mti != MSymbolTables.end())
        searchTable = mti->second;

    return pFound;
}

int SymbolTable::removeSymbol(const std::string &s)
{
    gpsimObject *pObj = find(s);
    if (pObj && searchTable)
    {
        if (searchTable->stiFound != searchTable->table.end())
        {
            searchTable->table.erase(searchTable->stiFound);
            return 1;
        }
    }

    return 0;
}

int SymbolTable::deleteSymbol(const std::string &s)
{
    gpsimObject *pObj = find(s);
    if (pObj && searchTable)
    {
        if (searchTable->stiFound != searchTable->table.end())
        {
            searchTable->table.erase(searchTable->stiFound);
            delete pObj;
            return 1;
        }
    }

    return 0;
}


//------------------------------------------------------------------------
// Convenience find functions
// All these do is call SymbolTable::find and cast the found symbol into
// another type

gpsimObject *SymbolTable::findObject(gpsimObject *pObj)
{
  return pObj ? find(pObj->name()) : nullptr;
}

Integer *SymbolTable::findInteger(const std::string &s)
{
  return dynamic_cast<Integer *>(find(s));
}

Value *SymbolTable::findValue(const std::string &s)
{
  return dynamic_cast<Value *>(find(s));
}

Module *SymbolTable::findModule(const std::string &s)
{
  return dynamic_cast<Module *>(find(s));
}


static void dumpModules(const SymbolTableEntry_t &st)
{
  std::cout << " Module: " << st.first << '\n';
}

void SymbolTable::listModules()
{
  ForEachModule(dumpModules);
}

void SymbolTable::ForEachModule(PFN_ForEachModule forEach)
{
  for_each(MSymbolTables.begin(), MSymbolTables.end(), forEach);
}

