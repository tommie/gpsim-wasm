/*
   Copyright (C) 1999 T. Scott Dattalo

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


#include <string>

#include "command.h"
#include "cmd_step.h"
#include "misc.h"

#include "../src/gpsim_interface.h"

cmd_step step;

static cmd_options cmd_step_options[] =
{
  {"over", 1, OPT_TT_BITFLAG},
  {nullptr, 0, 0}
};


cmd_step::cmd_step()
  : command("step","s")
{ 
  brief_doc = "Execute one or more instructions.";

  long_doc = "\nstep [over | n]\n\n\
\t    no arguments:  step one instruction.\n\
\tnumeric argument:  step a number of instructions\n\
\t \"over\" argument:  step over the next instruction\n\
\n";

  op = cmd_step_options; 
}


void cmd_step::step(int instructions)
{
  get_interface().step_simulation(instructions);
}

void cmd_step::step(Expression *expr)
{
  get_interface().step_simulation((int)evaluate(expr));
}

void cmd_step::over()
{
  get_interface().advance_simulation(gpsimInterface::eAdvanceNextInstruction);
}
