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

#include <glib.h>

#include <iostream>
#include <string>

#include "command.h"
#include "cmd_disasm.h"
#include "misc.h"

#include "../src/errors.h"
#include "../src/expr.h"
#include "../src/processor.h"
#include "../src/value.h"

cmd_disassemble disassemble;

static cmd_options cmd_disassemble_options[] =
{
  {nullptr, 0, 0}
};


cmd_disassemble::cmd_disassemble()
  : command("disassemble", "da")
{
  brief_doc = "Disassemble the current cpu";

  long_doc = "\ndisassemble [startCount : endCount] | [count]]\n\n\
\t startCount, endCount and count may all be expressions that evaluate\n\
\t to an integer value. The colon is used to indicate a range.\n\n\
\t startCount   - Start list with the instruction startCount from the \n\
\t                instruction at the PC.\n\
\t endCount     - List instruction in the list is the endCount\n\
\t                instruction from the PC.\n\
\t count        - List count instructions from starting with the\n\
\t                instruction at thePC.\n\n\
\t no  arguments: disassembles 10 instructions before and 5 after the pc.\n\
\t one argument:  disassemble [count] instructions after the pc.\n\
\t two arguments: disassemble from [startCount] to [endCount] relative\n\
\t                to the PC.\n";

  op = cmd_disassemble_options;
}


void cmd_disassemble::disassemble(Expression *expr)
{
  Processor *cpu = GetActiveCPU();
  if (cpu) {
    // Select a default range:

    int start = -10;
    int end = 5;

    if (expr) {

      try {
        Value *v = expr->evaluate();

        AbstractRange *ar = dynamic_cast<AbstractRange *>(v);
        if (ar) {
          start = ar->get_leftVal();
          end = ar->get_rightVal();
        } else if (v) {
          start = 0;
          gint64 i;
          v->get(i);
          end = (int) i;
        }
      }

      catch (const Error &err) {
          std::cout << "ERROR:" << err.what() << '\n';
      }

    }

    if (cpu->pma) {
      int current_pc = cpu->pma->get_PC();

      if (start < 0) {
        start += current_pc;
        end   += current_pc;
      }

      std::cout << std::hex << " current pc = 0x" << current_pc << '\n';
      cpu->disassemble(start, end);
    }
  }
}
