/*
   Copyright (C) 2004 T. Scott Dattalo

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

/*
  Scripting

  client2.cc - a program to demonstrate gpsim scripting.

*/

#include <stdio.h>
#include <stdlib.h>

#include "client_interface.h"

#define PORT        0x1234


//========================================================================
//
//
//
int main(int argc, char **argv)
{

  bool bPassed = true;  // regression test results - assume we pass

  ClientSocketInterface *sock = new ClientSocketInterface(argc > 2 ? argv[2] : 0, PORT);


  /*
    Set up.

    Since we are controlling the simulator entirely from a script, there really is
    not much need for the command line output it provides. So we'll turn it off
    by setting the simulator attribute 'sim.verbosity' to zero.

  */

  sock->WriteSymbolUInt32("sim.verbosity", 0);


  /*
    The simulator's command line parser is still available for us to use.
    So we'll use it to load the source code and to set a break point.
  */
  //sock->SendCmd("load s gensquares.cod\n");

  unsigned int h_reg1 = sock->CreateLinkUInt32("reg1");

  /*
    start running
  */

  //sock->SendCmd("run\n");




  if (!h_reg1) {
    printf("unable to create handles\n");
    delete sock;
    exit(1);
  }

  unsigned int reg1=0;


  for (unsigned int i = 0; i < 256; i++) {
    printf("query link\n");
    sock->WriteToLinkUInt32(h_reg1, i);
    printf("query link got %u\n", i);
    if (! sock->QueryLinkUInt32(h_reg1,reg1) )
      printf("failed to decode reg1\n");
    if (i != reg1)
      bPassed = false;
  }
  /*
    release the handle
  */

  sock->RemoveLink(h_reg1);

  sock->WriteSymbolUInt32("sim.verbosity", 1);

  delete sock;

  if(bPassed)
    printf("The simulation passed!\n");
  else
    printf("The simulation failed\n");

  return 0;
}

 
