/*
   Copyright (C) 2007 T. Scott Dattalo

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

#ifndef EXTRAS_GRAPHIC_LCD_SRC_OSRAM_H_
#define EXTRAS_GRAPHIC_LCD_SRC_OSRAM_H_

#include <gtk/gtk.h>

#include "glcd.h"

class SSD0323;
class Module;
class PortRegister;

// gpsim graphics LCD modules for OSRAM Pictivia(tm) OLED Displays
namespace OSRAM {

class SSD0323_InputPin;
class StateAttribute;

class PK27_Series : public gLCD_Module {
public:
  explicit PK27_Series(const char *new_name);
  ~PK27_Series();

  static Module *construct(const char *new_name);
  void Update(GtkWidget *pw = nullptr);
  bool dataBusDirection();
  /*
    void UpdatePinState(ePins, char);
  */
  void create_iopin_map();
  void create_widget();

private:
  PortRegister *m_dataBus;
  SSD0323_InputPin *m_CS;
  SSD0323_InputPin *m_RES;
  SSD0323_InputPin *m_DC;
  SSD0323_InputPin *m_E;
  SSD0323_InputPin *m_RW;
  SSD0323_InputPin *m_BS1;
  SSD0323_InputPin *m_BS2;
  IO_bi_directional *io_bus[8];

  SSD0323 *m_pSSD0323;

  StateAttribute *m_state;
  static gboolean lcd_expose_event(GtkWidget *widget, GdkEventExpose *event,
                                   PK27_Series *pLCD);
};

} // End of OSRAM namespace


#endif // EXTRAS_GRAPHIC_LCD_SRC_OSRAM_H_
