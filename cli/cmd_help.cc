/*
   Copyright (C) 1998 T. Scott Dattalo

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

#include <string.h>

#include <iostream>
#include <string>

#include "command.h"
#include "cmd_help.h"
#include "misc.h"

#include "../src/gpsim_object.h"

static cmd_options cmd_help_options[] =
{
  { nullptr, 0, 0 }
};

cmd_help help;


cmd_help::cmd_help()
  : command("help", nullptr)
{
  brief_doc = "Type help \"command\" for more help on a command";

  long_doc = "\n\tgpsim is a software simulator for the Microchip PIC microcontrollers\n\
\tPlease refer to the distributed README files and the ./doc subdirectory\n\
\tfor more information\n\
\n\tTo get help on a command, type help \"command\"\n\
\n\tIn addition, help on most symbols can be obtained by help\"symbol name\"\n\
\n\t(Use the symbol command to see the currently defined symbols\n";

  op = cmd_help_options;
}

void cmd_help::help()
{
  for (int i = 0; i < number_of_commands; i++) {
    command * pCmd = command_list[i];
    std::cout << pCmd->name();
    int l = 16 - strlen(pCmd->name());

    if (pCmd->abbreviation()) {
      std::cout << ":" << pCmd->abbreviation();
      l -= strlen(pCmd->abbreviation()) + 1;
    }

    for (int k = 0; k < l; k++)
      std::cout << ' ';

    std::cout << pCmd->brief_doc << '\n';
  }
}


void cmd_help::help(const char *cmd)
{
  command * pCmd = search_commands(cmd);

  if (pCmd) {
    std::cout << pCmd->long_doc << '\n';
    return;
  }

  std::cout << cmd << " is not a valid gpsim command. Try these instead:\n";
  help();
}

void cmd_help::help(gpsimObject *s)
{
  if (s) {
    std::cout << s->toString() << '\n';
    std::cout << s->description() << '\n';
  }
}
