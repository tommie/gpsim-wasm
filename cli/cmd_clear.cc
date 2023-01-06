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

#include <string.h>

#include <iostream>
#include <string>
#include <typeinfo>

#include "command.h"
#include "cmd_clear.h"
#include "misc.h"

#include "../src/breakpoints.h"
#include "../src/errors.h"
#include "../src/expr.h"
#include "../src/processor.h"
#include "../src/value.h"

cmd_clear clear;

static cmd_options cmd_clear_options[] =
{
  {nullptr, 0, 0}
};


cmd_clear::cmd_clear()
  : command("clear", "cl")
{
  brief_doc = "Remove a break point";

  long_doc = "clear bp_number\n\
where bp_number is the number assigned to the break point\n\
when it was created. (type \"break\" without any arguments to\n\
display the currently set break points.\n";

  op = cmd_clear_options;
}


void cmd_clear::clear(Expression *expr)
{
  if (expr) {
    try {

      Value *v = expr->evaluate();
      if (v) {

        if (typeid(*v) == typeid(String)) {
          char szParam[20];
          ((String*)v)->get(szParam, 20);
          if (strcmp(szParam, "all") == 0) {
            get_bp().clear_all(get_active_cpu());
          }
        }
        else if (typeid(*v) == typeid(Integer)) {
          /* for now, assume that the expression evaluates to an integer
             (later, things like 'clear all' will be add)
          */
          int64_t i;
          v->get(i);
          get_bp().clear((unsigned int)i);
        }
        delete v;
      }
    }
    catch (const Error &err) {
        std::cerr << "***ERROR cmd_clear " << err.what() << '\n';
    }

    delete expr;
  }
}
