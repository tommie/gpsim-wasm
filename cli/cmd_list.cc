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
#include "cmd_list.h"
#include "misc.h"

#include "../src/processor.h"
#include "../src/registers.h"

cmd_list c_list;

static cmd_options cmd_list_options[] =
{
  {"l",1,    OPT_TT_BITFLAG},
  {"s",2,    OPT_TT_BITFLAG},
  {nullptr, 0, 0}
};


cmd_list::cmd_list()
  : command("list", nullptr)
{
  brief_doc = "Display source and list files";

  long_doc = "list [[s | l] [*pc] [line_number1 [,line_number2]]]\n"
    "\n"
    "\tDisplay the contents of source and list files.\n"
    "\tWithout any options, list will use the last specified options.\n"
    "\tlist s will display lines in the source (or .asm) file.\n"
    "\tlist l will display lines in the .lst file\n"
    "\tlist *pc will display either .asm or .lst lines around the\n"
    "\t   value specified by pc (e.g. list *20 will list lines around\n"
    "\t   address 20)\n"
    "\tline_number1, line_number2 - specify the list range.\n"
    "\n"
    "\tExamples:\n"
    "\tlist s *0x3a -5 5\n"
    "\t  will list 11 lines (5 before, 5 after, & 1 at) around addr 3a\n"
    "\tlist\n"
    "\t  repeat the last list except use the current pc as the reference.\n"
    "\tlist l\n"
    "\t  will list lines from .lst file around the current pc.\n";

  op = cmd_list_options; 

  file_id = 0;
  starting_line = -5;
  ending_line = +5;
}

void cmd_list::list()
{
  Processor *pCpu = GetActiveCPU(true);
  if (pCpu)
    pCpu->list(file_id, pCpu->pc->value, starting_line, ending_line);
}

void cmd_list::list(cmd_options *opt)
{
  switch (opt->value) {
  case 1:
    file_id = 1;
    break;

  case 2:
    file_id = 0;
    break;
  }

  list();
}

