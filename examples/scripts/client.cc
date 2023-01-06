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

  client.cc - a program to demonstrate gpsim scripting.

  The purpose of this program is to demonstrate how one might write a
  script to perform regression testing on an algorithm. In this
  example, the algorithm is one that squares an 8-bit number. It is
  implemented in the PIC source code gensquares.asm. 

  The scripting mechanism is implemented on top of a socket
  interface. When gpsim is started, it will act as a server and began
  listening to clients. This client will initiate the communication
  link. It probably goes with out saying, but gpsim needs to be
  running before this client script will even work!

  Once this client establishs a communication link, it will proceed to
  set up the simulation environment and perform a regression test.

  Technical details:

  The scripting interface is implemented with a gpsim specific
  protocol. This protocol is somewhat analogous to gdb's
  protocol. gpsim's protocol operates in two modes. In one mode,
  character strings are sent across the socket interface and sent
  through the command line parser. Thus, it's possible to issue any
  gpsim command through a socket that can be typed at the command
  line. As of this writing, this interface is only uni-directional
  though. In other words, the client can only issue commands, but
  can't see the response. However, the other mode of the protocol
  allows bi-directional communication. This other mode ties directly
  into gpsim symbols and attributes. 

  In the attribute mode of the protocol, there are two ways to
  interface to the symbols. The first is by symbol name and the second
  is by a 'handle'. For either one of these, it's possible to read and
  write symbols. Handles are more efficient since they only have to be
  processed only one. Accessing by name on the other hand, requires
  string processing and symbol table accesses. 

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

  ClientSocketInterface *sock = new ClientSocketInterface(argc>2 ? argv[2] : 0, PORT);


  /*
    Set up.

    Since we are controlling the simulator entirely from a script, there really is
    not much need for the command line output it provides. So we'll turn it off
    by setting the simulator attribute 'sim.verbosity' to zero.

  */

  sock->WriteSymbolUInt32("sim.verbosity",0);


  /*
    The simulator's command line parser is still available for us to use.
    So we'll use it to load the source code and to set a break point.
  */
  sock->SendCmd("load s gensquares.cod\n");
  sock->SendCmd("break e start\n");

  /*
    There's a break point set at the label 'start'. Run to it.
  */
  sock->SendCmd("run\n");


  /*
    Now, we'll establish links to some of the internal symbols we're
    interested in. These links are called 'handles'.
  */

  unsigned int h_count = sock->CreateLinkUInt32("count");
  unsigned int h_x_lo = sock->CreateLinkUInt32("x_lo");
  unsigned int h_x_hi = sock->CreateLinkUInt32("x_hi");

  if(!h_count || !h_x_lo || !h_x_hi) {
    printf("unable to create handles\n");
    delete sock;
    exit(1);
  }

  /*
    
  */

  unsigned int count;
  unsigned int x_lo=0;
  unsigned int x_hi=0;

  for(count=0; count < 128; count++) {

    sock->WriteToLinkUInt32(h_count, count);

    sock->SendCmd("run\n");

    
    if(! sock->QueryLinkUInt32(h_x_lo,x_lo) )
      printf("failed to decode x_lo");

    if(! sock->QueryLinkUInt32(h_x_hi,x_hi) )
      printf("failed to decode x_hi");


    /*
      Check the results.
    */
    if((count*count) != ((x_hi<<8) + x_lo)) {
      unsigned int a = count *count;
      unsigned int b = (x_hi<<8) + x_lo;
      printf("%x  %x  ", a, b);
      bPassed = false;
      printf("failed sent: 0x%02X  received 0x%02X%02X\n",count, x_hi,x_lo);
    }
  }

  /*
    release the handles created above.
  */

  sock->RemoveLink(h_count);
  sock->RemoveLink(h_x_lo);
  sock->RemoveLink(h_x_hi);

  
  sock->WriteSymbolUInt32("sim.verbosity",1);

  delete sock;

  if(bPassed) 
    printf("The simulation passed!\n");
  else
    printf("The simulation failed\n");

  return 0;
}

 
