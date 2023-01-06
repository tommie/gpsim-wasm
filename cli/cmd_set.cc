/*
   Copyright (C) 1999-2000 T. Scott Dattalo

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

#include <glib.h>

#include <iostream>
#include <string>

#include "command.h"
#include "cmd_set.h"
#include "misc.h"

#include "../src/errors.h"
#include "../src/expr.h"
#include "../src/ui.h"
#include "../src/value.h"

static int radix = 0;   // FIXME

cmd_set c_set;

enum {
  SET_VERBOSE,
  SET_RADIX,
};

static cmd_options cmd_set_options[] =
{
  {"r",          SET_RADIX,      OPT_TT_NUMERIC},
  {"radix",      SET_RADIX,      OPT_TT_NUMERIC},
  {"v",          SET_VERBOSE,    OPT_TT_BITFLAG},
  {"verbose",    SET_VERBOSE,    OPT_TT_BITFLAG},
  {nullptr, 0, 0}
};


cmd_set::cmd_set()
  : command("set", nullptr)
{
  brief_doc = "display and control gpsim behavior flags";

  long_doc = "set\n"
    "\twith no options, set will display the state of all of gpsim's\n"
    "\tbehavior flags. Use this to determine the flags that may be\n"
    "\tmodified.\n"
    "\n";

  op = cmd_set_options;
}


void cmd_set::set()
{
  std::cout << "r | radix = " << radix << " (not fully functional)\n";
  std::cout << "v | verbose =  " << (unsigned int)verbose << '\n';
  //  cout << "gui_update = " << gi.update_rate << '\n';
}

void cmd_set::set(int bit_flag, Expression *expr)
{
  int number = 1;

  if (expr) {
    try
      {
        Value *v = expr->evaluate();
        if (v) {
          gint64 i;
          v->get(i);
          number = (int)i;
          delete v;
        }
        delete expr;
      }
    catch (const Error &err)
      {
        std::cout << "ERROR:" << err.what() << '\n';
        return;
      }
  }

  switch(bit_flag) {
  case SET_VERBOSE:
    GetUserInterface().SetVerbosity(number);
    break;

  default:
    std::cout << " Invalid set option\n";
  }
}

