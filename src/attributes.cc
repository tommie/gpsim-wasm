/*
   Copyright (C) 1998-2005 T. Scott Dattalo

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

#include "attributes.h"
#include "processor.h"
#include "gpsim_time.h"
#include "protocol.h"
#include <config.h>

#include <stdio.h>
#include <iostream>
#include <string>

//========================================================================
// Attribute wrappers
//
// Attribute wrappers are simple classes that handle specific processor
// attributes. Their primary purpose is to allow for a way to call back
// into the processor classs whenever the attribute changes.



//========================================================================
// WarnMode
//========================================================================

WarnModeAttribute::WarnModeAttribute(Processor *_cpu)
  : Boolean("WarnMode", false, " enable warning messages when true"),
    cpu(_cpu)
{}

void WarnModeAttribute::set(bool b)
{
  Boolean::set(b);
  cpu->setWarnMode(b);
}

//========================================================================
// SafeModeAttribute
//========================================================================

SafeModeAttribute::SafeModeAttribute(Processor *_cpu)
  : Boolean("SafeMode", false,
            " Model the processor's specification when true. Model the actual\n"
            " processor when false (e.g. TRIS instruction for mid range PICs\n"
            " will emit a warning if SafeMode is true)."),
  cpu(_cpu)
{
}

void SafeModeAttribute::set(bool b)
{
  Boolean::set(b);
  cpu->setSafeMode(b);
}

//========================================================================
// UnknownModeAttribute
//========================================================================
UnknownModeAttribute::UnknownModeAttribute(Processor *_cpu)
  : Boolean("UnknownMode", false,
            " Enable three-state register logic. Unknown values are treated \n"
            " as 0 when this is false."), cpu(_cpu)
{}

void UnknownModeAttribute::set(bool b)
{
  Boolean::set(b);
  cpu->setUnknownMode(b);
}

//========================================================================
// BreakOnResetAttribute
//========================================================================
BreakOnResetAttribute::BreakOnResetAttribute(Processor *_cpu)
  : Boolean("BreakOnReset", false,
            " If true, halt simulation when reset occurs \n"), cpu(_cpu)
{}

void BreakOnResetAttribute::set(bool b)
{
  Boolean::set(b);
  cpu->setBreakOnReset(b);
}

//========================================================================
//
// Cycle_Counter attribute
//
// The Cycle_counter attribute exposes the cycle counter through the
// gpsim attribute interface. This allows for it to be queried from
// the command line or sockets.

class CycleCounterAttribute : public Integer
{
public:
  CycleCounterAttribute()
    : Integer("cycles", 0, " Simulation time in terms of cycles.")
  {}

  void set(int64_t) override
  {
    static bool warned = false;
    if (!warned)
      std::cout << "cycle counter is read only\n";
    warned = true;
  }

  std::string toString() override
  {
    char buf[256];
    int64_t i = get();
    long long int j = i;
    snprintf(buf, sizeof(buf), "%" PRINTF_INT64_MODIFIER
             "d = 0x%08" PRINTF_INT64_MODIFIER "X", j, j);
    return buf;
  }
};


//========================================================================
//
Integer *verbosity;


//########################################################################
void init_attributes()
{
  // Define internal simulator attributes .
  verbosity = new Integer("sim.verbosity", 1, "gpsim's verboseness 0=nothing printed 0xff=very verbose");
  globalSymbolTable().addSymbol(verbosity);
  globalSymbolTable().addSymbol(new CycleCounterAttribute());
  stop_watch = new StopWatch;

  globalSymbolTable().addSymbol(new Integer("POR_RESET",  POR_RESET));    // Power-on reset
  globalSymbolTable().addSymbol(new Integer("WDT_RESET",  WDT_RESET));    // Watch Dog timer timeout reset
  globalSymbolTable().addSymbol(new Integer("IO_RESET",   IO_RESET));     // I/O pin reset
  globalSymbolTable().addSymbol(new Integer("SOFT_RESET", SOFT_RESET));   // Software initiated reset
  globalSymbolTable().addSymbol(new Integer("BOD_RESET",  BOD_RESET));    // Brown out detection reset
  globalSymbolTable().addSymbol(new Integer("SIM_RESET",  SIM_RESET));    // Simulation Reset
  globalSymbolTable().addSymbol(new Integer("MCLR_RESET", MCLR_RESET));   // MCLR (Master Clear) Reset

}

void destroy_attributes()
{
  globalSymbolTable().deleteSymbol("SourcePath");
  globalSymbolTable().deleteSymbol("sim.verbosity");
  globalSymbolTable().deleteSymbol("cycles");
  globalSymbolTable().deleteSymbol("POR_RESET");
  globalSymbolTable().deleteSymbol("WDT_RESET");
  globalSymbolTable().deleteSymbol("IO_RESET");
  globalSymbolTable().deleteSymbol("SOFT_RESET");
  globalSymbolTable().deleteSymbol("BOD_RESET");
  globalSymbolTable().deleteSymbol("SIM_RESET");
  globalSymbolTable().deleteSymbol("MCLR_RESET");

  delete stop_watch;
}
