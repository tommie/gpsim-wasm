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

#include "cmd_bus.h"
#include "misc.h"

cmd_bus c_bus;

//extern void dump_bus_list(void);
//extern void add_bus(char *node_name);

static cmd_options cmd_bus_options[] = {
  {nullptr, 0, 0}
};


cmd_bus::cmd_bus()
  : command("bus", nullptr)
{
  brief_doc = "Add or display node busses";
  long_doc = "bus [new_bus1 new_bus2 ...]\n"
             "\t If no new_bus is specified then all of the busses that have been\n"
             "\tdefined are displayed. If a new_bus is specified then it will be\n"
             "\tadded to the bus list. See the \"attach\" and \"stimulus\" commands\n"
             "\tto see how stimuli are added to the busses.\n"
             "\n"
             "\texamples:\n"
             "\n"
             "\tbus              // display the bus list\n"
             "\tbus b1 b2 b3     // create and add 3 new busses to the list\n";
  op = cmd_bus_options;
}


void cmd_bus::list_busses()
{
  //  dump_bus_list();
}


void cmd_bus::add_busses(std::list<std::string> * /* busses */ )
{
  /*
  if(busses) {

    list <string> :: iterator si;

    for (si = busses->begin();
   si != busses->end();
   ++si) {

      string s = *si;
      add_bus((char *)s.c_str());
    }

  }
  */
}
