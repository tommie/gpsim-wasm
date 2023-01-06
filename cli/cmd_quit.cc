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


#include <iostream>

#include "command.h"
#include "cmd_quit.h"
#include "misc.h"

cmd_quit quit;

static struct cmd_options cmd_quit_options[] = {
  {nullptr, 0, 0}
};


cmd_quit::cmd_quit()
  : command("quit", "q")
{
  brief_doc = "Quit gpsim";
  long_doc = "Quit gpsim\n";
  op = cmd_quit_options;
}


void cmd_quit::quit(int /* bit_flag */ )
{
  std::cout << " cmd_quit::quit with bit flag\n";
}

