/*
   Copyright (C) 2005 T. Scott Dattalo

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


#ifndef EXTRAS_GRAPHIC_LCD_SRC_GLCD_100X32_SED1520_H_
#define EXTRAS_GRAPHIC_LCD_SRC_GLCD_100X32_SED1520_H_

#include <gtk/gtk.h>
#include "glcd.h"

class SED1520;
class gLCD_InputPin;
class PortRegister;
class Module;

enum enPins {
  enA0,
  enE1,
  enE2,
  enRW
};

class gLCD_100X32_SED1520 : public gLCD_Module {
public:
  explicit gLCD_100X32_SED1520(const char *new_name);
  ~gLCD_100X32_SED1520();

  static Module *construct(const char *new_name);
  bool dataBusDirection();
  void Update(GtkWidget *pw = nullptr);
  void UpdatePinState(enPins, char);
  void create_iopin_map();
  void create_widget();

private:
  PortRegister *m_dataBus;
  gLCD_InputPin *m_A0;
  gLCD_InputPin *m_E1;
  gLCD_InputPin *m_E2;
  gLCD_InputPin *m_RW;

  SED1520      *m_sed1;
  SED1520      *m_sed2;

  static gboolean lcd_expose_event(GtkWidget *widget, GdkEventExpose *event,
                                   gLCD_100X32_SED1520 *pLCD);
};

#endif //__GLCD_100X32_SED1520_H__
