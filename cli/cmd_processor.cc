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

#include <stdio.h>

#include <iostream>
#include <string>

#include "command.h"
#include "cmd_processor.h"
#include "misc.h"
#include "../src/processor.h"
#include "../src/sim_context.h"
#include "../src/ui.h"

cmd_processor c_processor;

static cmd_options cmd_processor_options[] =
{
  {"list", 1,    OPT_TT_BITFLAG},
  {"pins", 2,    OPT_TT_BITFLAG},
  {nullptr, 0, 0}
};


static void put_chars(char c, int number_of_chars)
{
  for (int i = 0; i < number_of_chars; i++)
    putchar(c);
}

void dump_pins(Processor *cpu)
{
  int number_of_pins = 0;
  int i;
  size_t longest_name = 0;

  if (cpu)
    number_of_pins = cpu->get_pin_count();

  if (number_of_pins <= 0)
    return;

  for (i = 1; i <= number_of_pins / 2; i++)
    {
      const size_t len = cpu->get_pin_name(i).size();
      if (len > longest_name)
        longest_name = len;
    }

  printf("  +--+");
  put_chars('-', longest_name + 3);
  printf("\\/");
  put_chars('-', longest_name + 3);
  printf("+--+\n");

  for (i = 1; i <= number_of_pins / 2; i++)
    {
      const char *s = cpu->get_pin_name(i).c_str();

      if (*s)
        {
          printf("%c |%2d| %s", (cpu->get_pin_state(i) > 0) ? 'H' : 'L', i, s);
          put_chars(' ', longest_name - cpu->get_pin_name(i).size() + 6);
        }
      else
        {
          printf("  |%2d| ", i);
          put_chars(' ', longest_name + 6);
        }

      int j = number_of_pins - i + 1;
      s = cpu->get_pin_name(j).c_str();
      if (*s)
        {
          printf("%s |%2d| %c\n", s, j, (cpu->get_pin_state(j) > 0) ? 'H' : 'L');
        }
      else
        {
          put_chars(' ', longest_name);
          printf(" |%2d|\n", j);
        }

    }

  printf("  +--+");
  put_chars('-', 2 * longest_name + 8);
  printf("+--+\n");
}

cmd_processor::cmd_processor()
  : command("processor", "proc")
{
  brief_doc = "Select & Display processors";

  long_doc = "processor [new_processor_type [new_processor_name]] | [list] | [dump]\n"
    "\tIf no new processor is specified, then the currently defined processor(s)\n"
    "\twill be displayed. To see a list of the processors supported by gpsim,\n"
    "\ttype 'processor list'.  To define a new processor, specify the processor\n"
    "\ttype and name. To display the state of the I/O processor, type 'processor\n"
    "\tdump' (For now, this will display the pin numbers and their current state.\n"
    "\n"
    "\texamples:\n"
    "\n"
    "\tprocessor               // Display the processors you've already defined.\n"
    "\tprocessor list          // Display the list of processors supported.\n"
    "\tprocessor pins          // Display the processor package and pin state\n"
    "\tprocessor p16cr84 fred  // Create a new processor.\n"
    "\tprocessor p16c74 wilma  // and another.\n"
    "\tprocessor p16c65        // Create one with no name.\n";

  op = cmd_processor_options; 
}


void cmd_processor::processor()
{
  if (verbose)
    std::cout << "cmd_processor: display processors\n";
  CSimulationContext::GetContext()->dump_processor_list();
}

void cmd_processor::processor(int bit_flag)
{
  switch (bit_flag) {
  case 1:
    std::cout << ProcessorConstructor::listDisplayString();
    break;

  case 2:
    dump_pins(GetActiveCPU());
    break;
  }
}


void cmd_processor::processor(const char * processor_type,
                              const char * processor_new_name)
{
  if (!CSimulationContext::GetContext()->SetDefaultProcessor( processor_type, processor_new_name))
   std:: cout << "Unable to find processor " << processor_type << '\n';
}
