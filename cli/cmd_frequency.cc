/*
   Copyright (C) 2001 Salvador E. Tropea

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


#include <iostream>
#include <string>

#include "command.h"
#include "cmd_frequency.h"
#include "misc.h"

#include "../src/processor.h"

static cmd_options cmd_frequency_options[] =
{
  {nullptr, 0, 0}
};

cmd_frequency::cmd_frequency()
  : command("frequency", "freq")
{
  brief_doc = "Set the clock frequency";

  long_doc = "\nfrequency [value]\n"
    "\tThis command sets the clock frequency. By default gpsim uses 4 MHz\n"
    "\tas clock. The clock frequency is used to compute time in seconds.\n"
    "\tUse this command to adjust this value.\n"
    "\tIf no value is provided this command prints the current clock.\n"
    "\tNote that PICs have an instruction clock that's a forth of the\n"
    "\texternal clock. This value is the external clock.\n";

  op = cmd_frequency_options;
}


void cmd_frequency::set(Expression *expr)
{
  Processor *pCpu = GetActiveCPU(true);
  if (!pCpu)
    return;

  double freq = evaluate(expr);

  if (freq <= 0.0)
    std::cout << "Error: the clock must be a positive value.\n";
  else
    pCpu->set_frequency(freq);
}

void cmd_frequency::print()
{
  Processor *pCpu = GetActiveCPU(true);
  if (pCpu)
    std::cout << "Clock frequency: " << pCpu->get_frequency() / 1e6 << " MHz.\n";
}

cmd_frequency frequency;
