/*
   Copyright (C) 1998,1999,2000,2001 T. Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/


#ifndef MODULES_USART_MODULE_H_
#define MODULES_USART_MODULE_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../config.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif


#include "../src/modules.h"


class TXREG;
class RCREG;
class RxBaudRateAttribute;
class TxBaudRateAttribute;
class TxBuffer;
class RxBuffer;
class Boolean;
class USART_TXPIN;
class USART_RXPIN;
class USART_IO;

class USARTModule : public Module {
public:

#ifdef HAVE_GUI
  GtkWidget *window, *text;
#endif // HAVE_GUI

  void CreateGraphics();
  virtual void show_tx(unsigned int data);
  virtual void SendByte(unsigned tx_byte);


  // Inheritances from the Package class
  virtual void create_iopin_map();

  explicit USARTModule(const char *new_name);
  ~USARTModule();

  static Module *USART_construct(const char *new_name = nullptr);

  virtual void new_rx_edge(unsigned int);
  virtual bool mGetTxByte(unsigned int &);
  virtual void newRxByte(unsigned int);
  virtual void get(char *, int len);

private:
  RxBaudRateAttribute *m_RxBaud;
  TxBaudRateAttribute *m_TxBaud;
  Boolean  *m_CRLF;
  Boolean  *m_loop;
  Boolean  *m_console;
  Boolean  *m_ShowHex;
  TxBuffer *m_TxBuffer;
  RxBuffer *m_RxBuffer;

  RCREG *m_rcreg;
  TXREG *m_txreg;
  USART_TXPIN *txpin;
  USART_RXPIN *rxpin;
  USART_IO    *cts;
  USART_IO    *rts;

  unsigned char * m_TxFIFO;
  int m_FifoLen;
  int m_FifoHead;
  int m_FifoTail;

#ifdef HAVE_GUI
  bool m_NewLine = false;
#endif // HAVE_GUI
};


#endif // MODULES_USART_MODULE_H_
