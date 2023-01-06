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
#include "cmd_run.h"
#include "misc.h"

#include "../src/gpsim_interface.h"
#include "../src/symbol.h"
#include "../src/value.h"

extern void redisplay_prompt();  // in input.cc

cmd_run c_run;

static cmd_options cmd_run_options[] =
{
  {nullptr, 0, 0}
};


cmd_run::cmd_run()
  : command("run", nullptr)
{ 
  brief_doc = "Initiate the simulation";

  long_doc = "run\n\
\tStart simulating and don't stop until a break is encountered.\n\
\tUse CTRL->C to halt the simulation execution.\n\n";

  op = cmd_run_options; 
}

void cmd_run::run()
{
  Integer &verbosity = *globalSymbolTable().findInteger("sim.verbosity");

  get_interface().start_simulation();

  if ((int)verbosity)
    redisplay_prompt();
}
