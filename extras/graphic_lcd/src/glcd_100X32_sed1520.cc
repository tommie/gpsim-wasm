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

#include <config.h>
#ifdef HAVE_GUI
#include "glcd_100X32_sed1520.h"
#include "glcd.h"
#include "sed1520.h"

#include <src/stimuli.h>
#include <src/ioports.h>

#include <cassert>

//------------------------------------------------------------------------
// I/O pins for the graphic LCD
//------------------------------------------------------------------------
// The gLCD_InputPin is the base class for the E1, E2, RW and A0
// pins. This class is derived from the IO_bi_directional class,
// although the pins are never driven as outputs. The setDrivenState
// method is overridden to capture when this pin is driven.

class gLCD_InputPin : public IO_bi_directional {
public:
  gLCD_InputPin(gLCD_100X32_SED1520 *, const char *pinName, enPins pin);

  // Capture when a node drives this pin.
  virtual void setDrivenState(bool new_dstate);

  char CurrentState()
  {
    return m_cDrivenState;
  }

private:
  // The IO_bi_directional constructor will initialize the direction
  // to be an input. We'll override the update_direction method so that
  // this will never change.

  virtual void update_direction(unsigned int, bool /* refresh */ )
  {
    // Disallow pin direction changes.
  }

  gLCD_100X32_SED1520 *m_pLCD;
  enPins m_pin;
  char m_cDrivenState;
};


//------------------------------------------------------------------------
//
gLCD_InputPin::gLCD_InputPin(gLCD_100X32_SED1520 *pLCD, const char *pinName, enPins pin)
  : IO_bi_directional(pinName), m_pLCD(pLCD), m_pin(pin), m_cDrivenState('Z')
{
  assert(m_pLCD);
}


void gLCD_InputPin::setDrivenState(bool new_dstate)
{
  IO_bi_directional::setDrivenState(new_dstate);
  char cState = getBitChar();

  if (m_cDrivenState != cState) {
    m_cDrivenState = cState;
    m_pLCD->UpdatePinState(m_pin, cState);
  }
}


//------------------------------------------------------------------------
//

class gLCDSignalControl : public SignalControl {
public:
  explicit gLCDSignalControl(gLCD_100X32_SED1520 *pLCD)
    : m_pLCD(pLCD)
  {
    assert(m_pLCD);
  }
  virtual void release()
  {
  }
  virtual char getState()
  {
    // return the data bus direction (note the negative true logic.
    return m_pLCD->dataBusDirection() ? '0' : '1';
  }

private:
  gLCD_100X32_SED1520 *m_pLCD;
};


//------------------------------------------------------------------------
bool gLCD_100X32_SED1520::dataBusDirection()
{
  // FIXME, if both sed's are driving the data bus, then they'll be in
  // contention.
  return m_sed1->dataBusDirection() || m_sed2->dataBusDirection();
}


//------------------------------------------------------------------------
Module *gLCD_100X32_SED1520::construct(const char *_new_name = nullptr)
{
  gLCD_100X32_SED1520 *gLCD = new gLCD_100X32_SED1520(_new_name);
  return gLCD;
}


//------------------------------------------------------------------------
gLCD_100X32_SED1520::gLCD_100X32_SED1520(const char *_new_name)
  : gLCD_Module(_new_name, "SED1520 100X32 Graphics LCD module", 100, 32)
{
  m_dataBus = new LcdPortRegister(this, ".data", "LCD Data Port");
  addSymbol(m_dataBus);
  m_dataBus->setEnableMask(0xff);
  m_A0      = new gLCD_InputPin(this, "a0", enA0);
  m_E1      = new gLCD_InputPin(this, "e1", enE1);
  m_E2      = new gLCD_InputPin(this, "e2", enE2);
  m_RW      = new gLCD_InputPin(this, "rw", enRW);
  addSymbol(m_A0);
  addSymbol(m_E1);
  addSymbol(m_E2);
  addSymbol(m_RW);
  m_sed1    = new SED1520();
  m_sed2    = new SED1520();
  m_sed1->randomizeRAM();
  m_sed2->randomizeRAM();
  create_iopin_map();
  create_widget();
}


gLCD_100X32_SED1520::~gLCD_100X32_SED1520()
{
  delete m_dataBus;
  removeSymbol(m_A0);
  removeSymbol(m_E1);
  removeSymbol(m_E2);
  removeSymbol(m_RW);
  delete m_sed1;
  delete m_sed2;
  gtk_widget_destroy(darea);
}


//------------------------------------------------------------------------
void gLCD_100X32_SED1520::create_iopin_map()
{
  create_pkg(18);
  // Add the individual io pins to the data bus.
  assign_pin(9, m_dataBus->addPin(new IO_bi_directional("d0"), 0));
  assign_pin(10, m_dataBus->addPin(new IO_bi_directional("d1"), 1));
  assign_pin(11, m_dataBus->addPin(new IO_bi_directional("d2"), 2));
  assign_pin(12, m_dataBus->addPin(new IO_bi_directional("d3"), 3));
  assign_pin(13, m_dataBus->addPin(new IO_bi_directional("d4"), 4));
  assign_pin(14, m_dataBus->addPin(new IO_bi_directional("d5"), 5));
  assign_pin(15, m_dataBus->addPin(new IO_bi_directional("d6"), 6));
  assign_pin(16, m_dataBus->addPin(new IO_bi_directional("d7"), 7));
  // Provide a SignalControl object that the dataBus port can query
  // to determine which direction to drive the data bus.
  // (See <gpsim/ioports.h> for more documentation on port behavior.
  //  But in summary, when an I/O port updates its I/O pins, it will
  //  query the pin drive direction via the SignalControl object. )
  SignalControl *pPortDirectionControl = new gLCDSignalControl(this);

  for (int i = 0; i < 8; i++) {
    (*m_dataBus)[i].setControl(pPortDirectionControl);
  }

  assign_pin(4, m_A0);
  assign_pin(5, m_RW);
  assign_pin(6, m_E1);
  assign_pin(7, m_E2);
}


gboolean gLCD_100X32_SED1520::lcd_expose_event(GtkWidget *widget,
    GdkEventExpose * ,
    gLCD_100X32_SED1520 *pLCD)
{
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  pLCD->m_plcd->clear(cr);

  for (unsigned int i = 0; i < pLCD->m_nColumns; ++i) {
    unsigned sedIndex = i < 50 ? i : (i - 50);
    SED1520 *sed = i < 50 ? pLCD->m_sed1 : pLCD->m_sed2;

    for (unsigned int j = 0; j < pLCD->m_nRows / 8; ++j) {
      unsigned int row = 8 * j;
      unsigned displayByte = (*sed)[sedIndex + ((j & 3) * 80)];

      for (unsigned int b = 0; b < 8; b++, displayByte >>= 1)
        if (displayByte & 1) {
          pLCD->m_plcd->setPixel(cr, i, row + b);
        }
    }
  }

  cairo_destroy(cr);
  return TRUE;
}


//------------------------------------------------------------------------
void gLCD_100X32_SED1520::Update(GtkWidget * )
{
  gtk_widget_queue_draw(darea);
}


//------------------------------------------------------------------------
void gLCD_100X32_SED1520::create_widget()
{
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "LCD");
  GtkWidget *frame = gtk_frame_new("gLCD_100X32");
  gtk_container_add(GTK_CONTAINER(window), frame);
  darea = gtk_drawing_area_new();
  gtk_widget_set_size_request(darea, (m_nColumns + 4) * 3, (m_nRows + 4) * 3);
  gtk_container_add(GTK_CONTAINER(frame), darea);
  g_signal_connect(darea, "expose_event", G_CALLBACK(lcd_expose_event), this);
  gtk_widget_set_events(darea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_widget_show_all(window);
  m_plcd = new gLCD(m_nColumns, m_nRows, 3, 3, 1);
}


//------------------------------------------------------------------------

void gLCD_100X32_SED1520::UpdatePinState(enPins pin, char cState)
{
  // One of the control lines has changed states. So refresh the
  // sed's with the most current data bus value.
  if (!m_sed1->dataBusDirection()) {
    m_sed1->driveDataBus(m_dataBus->get());
  }

  if (!m_sed2->dataBusDirection()) {
    m_sed2->driveDataBus(m_dataBus->get());
  }

  bool bState = (cState == '1') || (cState == 'W');

  switch (pin) {
  case enA0:
    m_sed1->setA0(bState);
    m_sed2->setA0(bState);
    break;

  case enE1:
    m_sed1->setE(bState);
    break;

  case enE2:
    m_sed2->setE(bState);
    break;

  case enRW:
    m_sed1->setRW(bState);
    m_sed2->setRW(bState);
    break;
  }

  // If either one of the sed's is driving, then place that
  // data on to the data bus.
  if (m_sed1->dataBusDirection()) {
    m_dataBus->put(m_sed1->getDataBus());

  } else if (m_sed2->dataBusDirection()) {
    m_dataBus->put(m_sed2->getDataBus());

  } else {
    m_dataBus->updatePort();
  }
}


#endif // HAVE_GUI
