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

#include "client_interface.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef putc 
#undef putc
#endif

#include <gpsim/protocol.h>


int ClientSocketInterface::Send(const char *msg, int msgLength,
				char *response, int respLength)
{

  if(!sd || !msg || !msgLength ||  !response || !respLength)
    return 0;

  if (send(sd, msg, msgLength, 0) == -1) {
    perror("send");
    exit(1);
  }

  int bytes = recv(sd, response, respLength, 0);
  if (bytes == -1) {
    perror("recv");
    exit(1);
  }

  return bytes;
}

//========================================================================
bool ClientSocketInterface::SendCmd(const char *cmd)
{
  if(cmd) {

    char response[256];

    int len = Send(cmd, strlen(cmd), response, sizeof(response));

    if(len && len <sizeof(response))
      response[len] = 0;

    if(len)
      return true;
  }

  return false;
}

//========================================================================
void ClientSocketInterface::Send()
{
  printf("Send sock=%d data:%s\n",sd,p->txBuff());
  if (send(sd, p->txBuff(), p->txBytesBuffered(), 0) == -1) {
    perror("send");
    exit(1);
  }

  int bytes= recv(sd, p->rxBuff(), p->rxSize(), 0);

  if (bytes  == -1) {
    perror("recv");
    exit(1);
  }

  p->rxTerminate(bytes);

  printf("Send sock=%d rx data:%s\n",sd,p->rxBuff());

}

//========================================================================
const char *ClientSocketInterface::Receive()
{
  printf("Receive sock=%d data:%s\n",sd,p->txBuff());
  p->prepare();

  /*
  if (send(sd, p->txBuff(), p->txBytesBuffered(), 0) == -1) {
    perror("send");
    exit(1);
  }
  */

  int bytes= recv(sd, p->rxBuff(), p->rxSize(), 0);

  if (bytes  == -1) {
    perror("recv");
    exit(1);
  }

  p->rxTerminate(bytes);

  printf("Receive sock=%d rx data:%s\n",sd,p->rxBuff());

  return p->rxBuff();
}

//========================================================================
//
unsigned int ClientSocketInterface::CreateCallbackLink(guint64 interval)
{
  p->prepare();

  p->EncodeObjectType(GPSIM_CMD_CREATE_CALLBACK_LINK);
  p->EncodeUInt64(interval);

  Send();

  unsigned int handle=0;

  if(p->DecodeHeader() && p->DecodeUInt32(handle) )
    return handle;

  return 0;
}

unsigned int ClientSocketInterface::CreateLinkUInt32(const char *sym_name)
{
  p->prepare();

  p->EncodeObjectType(GPSIM_CMD_CREATE_SOCKET_LINK);
  p->EncodeString(sym_name);

  Send();

  unsigned int handle=0;

  if(p->DecodeHeader() && p->DecodeUInt32(handle) )
    return handle;

  return 0;
}

void ClientSocketInterface::RemoveLink(unsigned int  handle)
{
  if(handle) {
    p->prepare();
    p->EncodeObjectType(GPSIM_CMD_REMOVE_SOCKET_LINK);
    p->EncodeUInt32(handle);
    Send();
  }

}

bool ClientSocketInterface::QueryLinkUInt32(unsigned int  handle, unsigned int &i)
{
  if(handle) {
    p->prepare();
    p->EncodeObjectType(GPSIM_CMD_QUERY_SOCKET_LINK);
    p->EncodeUInt32(handle);
    Send();
    if(p->DecodeHeader() && p->DecodeUInt32(i))
      return true;
  }
  return false;
}

void ClientSocketInterface::WriteToLinkUInt32(unsigned int  handle, unsigned int val)
{
  if(handle) {
    p->prepare();
    p->EncodeObjectType(GPSIM_CMD_WRITE_TO_SOCKET_LINK);
    p->EncodeUInt32(handle);
    p->EncodeUInt32(val);
    Send();
  }
}

bool ClientSocketInterface::QuerySymbolUInt32(const char *sym_name, unsigned int &i)
{
  if(sym_name) {
    p->prepare();
    p->EncodeObjectType(GPSIM_CMD_QUERY_SYMBOL);
    p->EncodeString(sym_name);
    p->txTerminate();
    Send();

    if(p->DecodeHeader() && p->DecodeUInt32(i))
      return true;
  }

  return false;
}

bool ClientSocketInterface::NotifyUInt32(const char *sym_name, unsigned int val)
{
  if(sym_name) {
    p->prepare();
    p->EncodeObjectType(GPSIM_CMD_CREATE_NOTIFY_LINK);
    p->EncodeString(sym_name);
    p->EncodeUInt32(val);
    p->txTerminate();
    Send();
    return true;

  }

  return false;
}

bool ClientSocketInterface::WriteSymbolUInt32(const char *sym_name, unsigned int v)
{
  if(sym_name) {
    p->prepare();
    p->EncodeObjectType(GPSIM_CMD_WRITE_TO_SYMBOL);
    p->EncodeString(sym_name);
    p->EncodeUInt32(v);
    p->txTerminate();
    Send();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
ClientSocketInterface::ClientSocketInterface(const char *hostname, int port)
{

  struct  sockaddr_in sin;
  struct  sockaddr_in pin;
  struct  hostent *hp;

  sd = 0;

  char        hname_buff[256];
  const char *hname;

  if(!hostname) {
    gethostname(hname_buff, sizeof(hname_buff));
    hname = hname_buff;
  } else
    hname = hostname;

  if ((hp = gethostbyname(hname)) == 0) {
    perror("gethostbyname");
    exit(1);
  }

  /* fill in the socket structure with host information */
  memset(&pin, 0, sizeof(pin));
  pin.sin_family = AF_INET;
  pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
  pin.sin_port = htons(port);

  /* grab an Internet domain socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  /* connect to PORT on HOST */
  if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
    perror("connect");
    exit(1);
  }
  printf("connected with socket %d\n",sd);
  p = new Packet(8192,8192);
}

