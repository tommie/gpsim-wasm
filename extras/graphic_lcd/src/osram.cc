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

#include <config.h>
#ifdef HAVE_GUI

#include "osram.h"
#include "glcd.h"
#include "ssd0323.h"

#include <src/stimuli.h>
#include <src/ioports.h>
#include <src/trace.h>

#include <cassert>
#include <string>

namespace OSRAM {

//------------------------------------------------------------------------
// Debug Stuff
//------------------------------------------------------------------------

class StateAttribute : public Integer {
public:
  explicit StateAttribute(SSD0323 *pSSD)
    : Integer("state", 0, "Display the state of the SSD0323 graphics controller"),
      m_pSSD(pSSD)
  {
    assert(pSSD);
  }
  virtual std::string toString()
  {
    m_pSSD->showState();
    return "";
  }

private:
  SSD0323 *m_pSSD;
};


//------------------------------------------------------------------------
// I/O pins for the SSD 0323 controller
//------------------------------------------------------------------------
// The LCD_InputPin is the base class for the E1, E2, RW and A0
// pins. This class is derived from the IO_bi_directional class,
// although the pins are never driven as outputs. The setDrivenState
// method is overridden to capture when this pin is driven.
// NOTE: an alternative implementation would be to instantiate
// PinModule objects for each of the pins and supply them with
// standard IO_bi_directional objects. Then SignalSinks could be
// created to capture pin state changes.

class SSD0323_InputPin : public IO_bi_directional {
public:
  SSD0323_InputPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName);

  // Capture when a node drives this pin.
  virtual void setDrivenState(bool new_dstate);

  virtual void UpdateControllerPin(bool /* bState */ ) {}
  char CurrentState()
  {
    return m_cDrivenState;
  }

protected:
  SSD0323 *m_pSSD0323;
  PortRegister *m_pDataBus;
  char m_cDrivenState;

private:
  // The IO_bi_directional constructor will initialize the direction
  // to be an input. We'll override the update_direction method so that
  // this will never change.

  virtual void update_direction(unsigned int, bool /* refresh */ )
  {
    // Disallow pin direction changes.
  }
};


SSD0323_InputPin::SSD0323_InputPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
  : IO_bi_directional(pinName),
    m_pSSD0323(pSSD),
    m_pDataBus(pDataBus),
    m_cDrivenState('Z')
{
  assert(m_pSSD0323);
}


void SSD0323_InputPin::setDrivenState(bool new_dstate)
{
  IO_bi_directional::setDrivenState(new_dstate);

  if (!m_pSSD0323->dataBusDirection()) {
    m_pSSD0323->driveDataBus(m_pDataBus->get());
  }

  char cState = getBitChar();

  if (m_cDrivenState != cState) {
    m_cDrivenState = cState;
    //cout << "LCD pin:" << name() << "-->"<<cState<<endl;
    UpdateControllerPin((cState == '1') || (cState == 'W'));
  }

  if (m_pSSD0323->dataBusDirection()) {
    m_pDataBus->put(m_pSSD0323->getDataBus());
  }
}


//------------------------------------------------------------------------
// SSD E Pin
//------------------------------------------------------------------------
class SSD0323_EPin : public SSD0323_InputPin {
public:
  SSD0323_EPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
    : SSD0323_InputPin(pSSD, pDataBus, pinName)
  {}
  virtual void UpdateControllerPin(bool);
};


void SSD0323_EPin::UpdateControllerPin(bool bState)
{
  m_pSSD0323->setE_RD(bState);
}


//------------------------------------------------------------------------
// SSD CS Pin
//------------------------------------------------------------------------
class SSD0323_CSPin : public SSD0323_InputPin {
public:
  SSD0323_CSPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
    : SSD0323_InputPin(pSSD, pDataBus, pinName)
  {}
  virtual void UpdateControllerPin(bool);
};


void SSD0323_CSPin::UpdateControllerPin(bool bState)
{
  m_pSSD0323->setCS(bState);
}


//------------------------------------------------------------------------
// SSD RES Pin
//------------------------------------------------------------------------
class SSD0323_RESPin : public SSD0323_InputPin {
public:
  SSD0323_RESPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
    : SSD0323_InputPin(pSSD, pDataBus, pinName)
  {}
  virtual void UpdateControllerPin(bool);
};


void SSD0323_RESPin::UpdateControllerPin(bool bState)
{
  m_pSSD0323->setRES(bState);
}


//------------------------------------------------------------------------
// SSD CS Pin
//------------------------------------------------------------------------
class SSD0323_DCPin : public SSD0323_InputPin {
public:
  SSD0323_DCPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
    : SSD0323_InputPin(pSSD, pDataBus, pinName)
  {}
  virtual void UpdateControllerPin(bool);
};


void SSD0323_DCPin::UpdateControllerPin(bool bState)
{
  m_pSSD0323->setDC(bState);
}


//------------------------------------------------------------------------
// SSD RW Pin
//------------------------------------------------------------------------
class SSD0323_RWPin : public SSD0323_InputPin {
public:
  SSD0323_RWPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
    : SSD0323_InputPin(pSSD, pDataBus, pinName)
  {}
  virtual void UpdateControllerPin(bool);
};


void SSD0323_RWPin::UpdateControllerPin(bool bState)
{
  m_pSSD0323->setRW(bState);
}


//------------------------------------------------------------------------
// SSD BS1, BS2 Pin
//------------------------------------------------------------------------
class SSD0323_BSPin : public SSD0323_InputPin {
public:
  SSD0323_BSPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName, unsigned pin)
    : SSD0323_InputPin(pSSD, pDataBus, pinName),
      m_pin(pin)
  {
    assert(m_pin == 1 || m_pin == 2);
  }
  virtual void UpdateControllerPin(bool);

private:
  unsigned int m_pin;
};


void SSD0323_BSPin::UpdateControllerPin(bool bState)
{
  m_pSSD0323->setBS(m_pin, bState);
}


//------------------------------------------------------------------------
// SSD BS1, BS2 Pin
//------------------------------------------------------------------------
class SSD_SPISignalSink : public SignalSink {
public:
  SSD_SPISignalSink(SSD0323 *pSSD, bool bClk_or_Data)
    : m_pSSD0323(pSSD),
      m_bClk(bClk_or_Data),
      m_cState(0)
  {
    assert(m_pSSD0323);
  }

  virtual void release()
  {
  }

  virtual void setSinkState(char c)
  {
    if (m_cState != c) {
      //cout << __FUNCTION__ << " new state: " << c << endl;
      if (m_bClk) {
        m_pSSD0323->setSCLK(c == '1' || c == 'W');

      } else {
        m_pSSD0323->setSDIN(c == '1' || c == 'W');
      }

      m_cState = c;
    }
  }

private:
  SSD0323 *m_pSSD0323;
  bool m_bClk;
  char m_cState;
};


//------------------------------------------------------------------------
//

class LCDSignalControl : public SignalControl {
public:
  explicit LCDSignalControl(PK27_Series *pLCD)
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
  PK27_Series *m_pLCD;
};


//------------------------------------------------------------------------
// PK27_Series -- Module constructor.
//------------------------------------------------------------------------

Module *PK27_Series::construct(const char *_new_name)
{
  PK27_Series *pLCD = new PK27_Series(_new_name);
  pLCD->create_widget();
  return pLCD;
}


//------------------------------------------------------------------------
// PK27_Series -- 128X64 - 4-bit gray scale.
//------------------------------------------------------------------------
PK27_Series::PK27_Series(const char *_new_name)
  : gLCD_Module(_new_name, "OSRAM 128X64 Graphics OLED module", 128, 64)
{
  m_pSSD0323 = new SSD0323();
  m_pSSD0323->setBS(0, false);
  m_dataBus = new LcdPortRegister(this, ".data", "LCD Data Port");
  addSymbol(m_dataBus);
  m_dataBus->setEnableMask(0xff);
  m_CS      = new SSD0323_CSPin(m_pSSD0323, m_dataBus, "cs");
  m_RES     = new SSD0323_RESPin(m_pSSD0323, m_dataBus, "res");
  m_DC      = new SSD0323_DCPin(m_pSSD0323, m_dataBus, "dc");
  m_E       = new SSD0323_EPin(m_pSSD0323, m_dataBus, "e");
  m_RW      = new SSD0323_RWPin(m_pSSD0323, m_dataBus, "rw");
  m_BS1     = new SSD0323_BSPin(m_pSSD0323, m_dataBus, "bs1", 1);
  m_BS2     = new SSD0323_BSPin(m_pSSD0323, m_dataBus, "bs2", 2);
  addSymbol(m_CS);
  addSymbol(m_RES);
  addSymbol(m_DC);
  addSymbol(m_E);
  addSymbol(m_RW);
  addSymbol(m_BS1);
  addSymbol(m_BS2);
  m_state   = new StateAttribute(m_pSSD0323);
  addSymbol(m_state);
  /*
    m_sed1->randomizeRAM();
    m_sed2->randomizeRAM();
  */
  create_iopin_map();
  //create_widget();
}


//------------------------------------------------------------------------
// ~PK27_Series
//------------------------------------------------------------------------
PK27_Series::~PK27_Series()
{
  removeSymbol(m_CS);
  removeSymbol(m_RES);
  removeSymbol(m_DC);
  removeSymbol(m_E);
  removeSymbol(m_RW);
  removeSymbol(m_BS1);
  removeSymbol(m_BS2);
  removeSymbol(m_state);
  gtk_widget_destroy(darea);
  delete m_pSSD0323;
  delete m_dataBus;
  delete m_state;
}


gboolean PK27_Series::lcd_expose_event(GtkWidget *widget,
                                       GdkEventExpose *,
                                       PK27_Series *pLCD)
{
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  pLCD->m_plcd->clear(cr);

  for (unsigned int row = 0; row < pLCD->m_nRows; ++row) {
    for (unsigned int i = 0; i < pLCD->m_nColumns / 2; ++i) {
      unsigned int displayByte = (*(pLCD->m_pSSD0323))[i + 64 * row];

      for (unsigned int b = 0; b < 2; b++, displayByte <<= 4) {
        pLCD->m_plcd->setPixel(cr, 2 * i + b, row, (displayByte >> 4) & 0xf);
      }
    }
  }

  cairo_destroy(cr);
  return TRUE;
}


//------------------------------------------------------------------------
// PK27_Series::Update
//------------------------------------------------------------------------
const int pixelScale = 2;

void PK27_Series::Update(GtkWidget *)
{
  gtk_widget_queue_draw(darea);
}


//------------------------------------------------------------------------
// PK27_Series::create_widget
//------------------------------------------------------------------------
void PK27_Series::create_widget()
{
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "LCD");
  GtkWidget *frame = gtk_frame_new("OSRAM PK27_Series");
  gtk_container_add(GTK_CONTAINER(window), frame);
  darea = gtk_drawing_area_new();
  gtk_widget_set_size_request(darea,
                              (m_nColumns + 3)*pixelScale, (m_nRows + 3)*pixelScale);
  gtk_container_add(GTK_CONTAINER(frame), darea);
  g_signal_connect(darea, "expose_event", G_CALLBACK(lcd_expose_event), this);
  gtk_widget_set_events(darea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_widget_show_all(window);
  m_plcd = new gLCD(m_nColumns, m_nRows, pixelScale, pixelScale, 0, 16);

  for (int i = 0; i < 16; ++i) {
    double c = double(i) / 16.0;
    m_plcd->setColor(i, c, c, 0.0);
  }
}


//------------------------------------------------------------------------
// PK27_Series::create_widget
//------------------------------------------------------------------------

void PK27_Series::create_iopin_map()
{
  create_pkg(30);
  assign_pin(20, m_BS1);
  assign_pin(19, m_BS2);
  assign_pin(17, m_CS);
  assign_pin(16, m_RES);
  assign_pin(15, m_DC);
  assign_pin(14, m_RW);
  assign_pin(13, m_E);
  // Add the individual io pins to the data bus.
  char text[] = "d0";

  for (int i = 0; i < 8; i++) {
    text[1] = '0' + i;
    io_bus[i] = new IO_bi_directional(text);
    addSymbol(io_bus[i]);
    assign_pin(12 - i, m_dataBus->addPin(io_bus[i], i));
  }

  m_dataBus->addSink(new SSD_SPISignalSink(m_pSSD0323, true), 0); // SPI CLK
  m_dataBus->addSink(new SSD_SPISignalSink(m_pSSD0323, false), 1); // SPI Data
  // Provide a SignalControl object that the dataBus port can query
  // to determine which direction to drive the data bus.
  // (See <gpsim/ioports.h> for more documentation on port behavior.
  //  But in summary, when an I/O port updates its I/O pins, it will
  //  query the pin drive direction via the SignalControl object. )
  SignalControl *pPortDirectionControl = new LCDSignalControl(this);

  for (int i = 0; i < 8; i++) {
    (*m_dataBus)[i].setControl(pPortDirectionControl);
  }
}


bool PK27_Series::dataBusDirection()
{
  return  m_pSSD0323->dataBusDirection();
}


} // end of namespace OSRAM

#endif // HAVE_GUI
