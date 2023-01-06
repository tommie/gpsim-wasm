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


#if !defined(CLIENT_INTERFACE_H)
#define  CLIENT_INTERFACE_H
#include <glib.h>
class Packet;

class ClientSocketInterface
{
public:
  ClientSocketInterface(const char *host, int port);

  bool SendCmd(const char *);

  /// Link interface
  unsigned int CreateCallbackLink(guint64);
  unsigned int CreateLinkUInt32(const char *sym_name);
  bool QueryLinkUInt32(unsigned int handle, unsigned int &i);
  void WriteToLinkUInt32(unsigned int  handle, unsigned int val);
  void RemoveLink(unsigned int handle);

  /// Symbol interface
  bool QuerySymbolUInt32(const char *sym_name, unsigned int &i);
  bool WriteSymbolUInt32(const char *sym_name, unsigned int v);
  bool NotifyUInt32(const char *sym_name, unsigned int val);

  const char *Receive();

private:
  void Send();
  int  Send(const char *msg, int msgLength,
	    char *response, int respLength);

  Packet *p;
  int sd;            // Socket descriptor.
};


#endif // CLIENT_INTERFACE_H
