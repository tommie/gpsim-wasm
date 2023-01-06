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

  client3.cc - a program to demonstrate gpsim scripting.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "client_interface.h"

#define PORT        0x1234


pthread_t thThread1;
pthread_attr_t thAttribute1;

pthread_t thThread2;
pthread_attr_t thAttribute2;

pthread_t thThread3;
pthread_attr_t thAttribute3;

int something=1;

ClientSocketInterface *sock1=0;
ClientSocketInterface *sock2=0;
ClientSocketInterface *sock3=0;

void *thread1(void *ignored)
{
  printf( "thread1 running\n");

  sleep(1);

  bool bPassed = true;  // regression test results - assume we pass

  ClientSocketInterface *sock=sock1;
  if(!sock)
    exit(1);

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
  //sock->SendCmd("load s gensquares.cod\n");

  unsigned int h_reg1 = sock->CreateLinkUInt32("reg1");

  if(!h_reg1) {
    printf("unable to create handles\n");
    delete sock;
    exit(1);
  }

  unsigned int reg1=0;

  for(unsigned int i=0; i<128; i++) {
    printf("t1 query link\n");
    sock->WriteToLinkUInt32(h_reg1, i);
    printf("t1 query link got %u\n", i);
    if(! sock->QueryLinkUInt32(h_reg1,reg1) )
      printf("failed to decode reg1\n");
    if(i != reg1) {
      printf("t1 failed expected %u, but got %u\n", i, reg1);
      bPassed = false;
      break;
    }
  }
  /*
    release the handle
  */

  sock->RemoveLink(h_reg1);

  sock->WriteSymbolUInt32("sim.verbosity",1);

  delete sock;

  if(bPassed)
    printf("The simulation passed!\n");
  else
    printf("The simulation failed\n");


  printf("thread 1 is complete\n");

  return 0;
}


void *thread2(void *ignored)
{
  printf( "thread2 running\n");


  bool bPassed = true;  // regression test results - assume we pass

  ClientSocketInterface *sock=sock2;
  if(!sock)
    exit(1);

  sleep(1);
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
  //sock->SendCmd("load s gensquares.cod\n");

  unsigned int h_reg2 = sock->CreateLinkUInt32("reg2");

  /*
    start running
  */

  //sock->SendCmd("run\n");




  if(!h_reg2) {
    printf("unable to create handles\n");
    delete sock;
    exit(1);
  }

  unsigned int reg2=0;

  for(unsigned int i=0; i<128; i++) {
    printf("t2 query link\n");
    sock->WriteToLinkUInt32(h_reg2, i);
    printf("t2 query link got %u\n", i);
    if(! sock->QueryLinkUInt32(h_reg2,reg2) )
      printf("failed to decode reg2\n");

    if(i != reg2) {
      printf("t2 failed expected %u, but got %u\n", i, reg2);
      bPassed = false;
      break;
    }

  }
  /*
    release the handle
  */

  sock->RemoveLink(h_reg2);

  sock->WriteSymbolUInt32("sim.verbosity",1);

  delete sock;

  if(bPassed)
    printf("The simulation passed!\n");
  else
    printf("The simulation failed\n");


  printf("thread 2 is complete\n");

  return 0;
}


void *thread3(void *ignored)
{
  printf( "thread3 running\n");

  ClientSocketInterface *sock=sock3;
  if (!sock)
    exit(1);

  int i = 0;
  while (i++<4)
    sleep(1);

  sock->SendCmd("load s gensquares.cod\n");
  if (sock->CreateCallbackLink(0x1000)) {
    printf("created callback link\n");
  }
  sock->SendCmd("break c 0x5000\n");
  sock->SendCmd("run\n");



  for(int i = 0; i<4; i++)
  {
      printf("thread 3 recieved %s\n", sock->Receive());
  }

  sock->SendCmd("quit\n");
  delete sock;
  printf("thread 3 is complete\n");
  return 0;
}

void start_threads(void)
{
  printf( "starting threads....\n");

  pthread_attr_init(&thAttribute1);
  pthread_attr_setdetachstate(&thAttribute1, PTHREAD_CREATE_JOINABLE);
  pthread_create(&thThread1, &thAttribute1, thread1, (void *)&something);

  pthread_attr_init(&thAttribute2);
  pthread_attr_setdetachstate(&thAttribute2, PTHREAD_CREATE_JOINABLE);
  pthread_create(&thThread2, &thAttribute2, thread2, (void *)&something);

  pthread_attr_init(&thAttribute3);
  pthread_attr_setdetachstate(&thAttribute3, PTHREAD_CREATE_JOINABLE);
  pthread_create(&thThread3, &thAttribute3, thread3, (void *)&something);

  printf( "started threads\n");
}

//========================================================================
//
//
//
int main(int argc, char **argv)
{

  sock1 = new ClientSocketInterface(argc>2 ? argv[2] : 0, PORT);
  sock2 = new ClientSocketInterface(argc>2 ? argv[2] : 0, PORT);
  sock3 = new ClientSocketInterface(argc>2 ? argv[2] : 0, PORT);

  start_threads();
  void * status;

  pthread_join(thThread3, &status);
  printf("thread 3 exit %d\n", status);
  return 0;
}

 
