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

#ifndef CLI_CMD_ECHO_H_
#define CLI_CMD_ECHO_H_

#include "command.h"

#include <iostream>

class cmd_echo : public command
{
public:
  cmd_echo();

  // The echo command is handled by the lexical analyzer (scan.l)
  // and not by this class. Note that this is different than the
  // other commands. The only reason the 'echo' class exists is to
  // provide a consistant way for displaying help text.

  void echo() { std::cout << "BUG: echo command does not execute!\n"; }

};

extern cmd_echo echo;

#endif

