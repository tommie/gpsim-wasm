/*
   Copyright (C) 2000 T. Scott Dattalo

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

  lcd.cc

  This is a fairly complete/complicated gpsim module that simulates
  an LCD display. This version is currently hardcoded for the Hitachi
  style 2 row by 20 column display. The font is also hardcoded to 5 by
  7 pixels.

  Hardware simulation:

  The Hitachi displays commonly have a 14pin interface:

  8 data lines
  3 control lines (E,RS,R/W)
  pwr
  gnd
  contrast

  This version only supports the 8 data and 3 control lines.

  Software simulation:

  This software uses an event driven behavior model to simulate
  the LCD display. This means that when one of the I/O lines toggle
  the module will be notified and will respond to the event. A state
  machine is used to control the behavior.

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <config.h>
#ifdef HAVE_GUI
#include <gtk/gtk.h>

#include "hd44780.h"
#include "lcd.h"
#include <src/gpsim_time.h>

#include <assert.h>
#include <iostream>

Trace *gTrace = 0;              // Points to gpsim's global trace object.


//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//------------------------------------------------------------------------
// I/O pins for the LCD
//------------------------------------------------------------------------
// The LCD_InputPin is the base class for the E, RW and DC
// pins. This class is derived from the IO_bi_directional class,
// although the pins are never driven as outputs. The setDrivenState
// method is overridden to capture when this pin is driven.

class LCD_InputPin : public IO_bi_directional {
public:
  LCD_InputPin(LcdDisplay *, const char *pinName, ePins pin);

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

  LcdDisplay *m_pLCD;
  ePins m_pin;
  char m_cDrivenState;
};


//------------------------------------------------------------------------
//

class LCDSignalControl : public SignalControl {
public:
  explicit LCDSignalControl(LcdDisplay *pLCD)
    : m_pLCD(pLCD)
  {
    assert(m_pLCD);
  }
  virtual void release()
  {
  }
  virtual char getState()
  {
    // returning a 1 indicates the data bus is an input (the LCD module
    // is being written to).
    char state = m_pLCD->dataBusDirection() ? '1' : '0';
    //Dprintf(("LCDSignalControl returning:%c\n",state));
    return state;
  }

private:
  LcdDisplay *m_pLCD;
};


//------------------------------------------------------------------------
//
LCD_InputPin::LCD_InputPin(LcdDisplay *pLCD, const char *pinName, ePins pin)
  : IO_bi_directional(pinName), m_pLCD(pLCD), m_pin(pin), m_cDrivenState(0)
{
}


void LCD_InputPin::setDrivenState(bool new_dstate)
{
  IO_bi_directional::setDrivenState(new_dstate);
  char cState = getBitChar();
  Dprintf(("LCD_InputPin setDrivenState:%d, cState:%c\n", new_dstate, cState));

  if (m_cDrivenState != cState) {
    m_cDrivenState = cState;
    m_pLCD->UpdatePinState(m_pin, cState);
  }
}


//------------------------------------------------------------------------
// Tracing
//

LcdWriteTO::LcdWriteTO(LcdDisplay *_lcd)
  : LcdTraceObject(_lcd)
{
}


void LcdWriteTO::print(FILE *fp)
{
  fprintf(fp, "  Wrote: LCD");
}


LcdReadTO::LcdReadTO(LcdDisplay *_lcd)
  : LcdTraceObject(_lcd)
{
}


void LcdReadTO::print(FILE *fp)
{
  fprintf(fp, "  Read: LCD");
}


//----------------------------------------
LcdWriteTT::LcdWriteTT(LcdDisplay *_lcd, unsigned int s)
  : LcdTraceType(_lcd, s)
{
}


TraceObject *LcdWriteTT::decode(unsigned int /* tbi */ )
{
  LcdWriteTO *lto = new LcdWriteTO(lcd);
  gTrace->addToCurrentFrame(lto);
  return lto;
}


int LcdWriteTT::dump_raw(unsigned tbi, char *buf, int bufsize)
{
  int n = TraceType::dump_raw(gTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  unsigned int tv = gTrace->get(tbi);
  int m = g_snprintf(buf, bufsize, " LCD Write 0x%08x", tv);

  if (m < bufsize) {
    n += m;
  }

  return n;
}


//----------------------------------------
LcdReadTT::LcdReadTT(LcdDisplay *_lcd, unsigned int s)
  : LcdTraceType(_lcd, s)
{
}


TraceObject *LcdReadTT::decode(unsigned int /* tbi */ )
{
  LcdReadTO *lto = new LcdReadTO(lcd);
  gTrace->addToCurrentFrame(lto);
  return lto;
}


int LcdReadTT::dump_raw(unsigned tbi, char *buf, int bufsize)
{
  int n = TraceType::dump_raw(gTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  unsigned int tv = gTrace->get(tbi);
  int m = g_snprintf(buf, bufsize, " LCD Read 0x%08x", tv);

  if (m < bufsize) {
    n += m;
  }

  return n;
}


//------------------------------------------------------------------------
//
// LCD interface to the simulator
//
LCD_Interface::LCD_Interface(LcdDisplay *_lcd)
  : Interface((gpointer *) _lcd)
{
  lcd = _lcd;
}


//--------------------------------------------------
// SimulationHasStopped (gpointer)
//
// gpsim will call this function when the simulation
// has halt (e.g. a break point was hit.)

void LCD_Interface::SimulationHasStopped(gpointer)
{
  if (lcd) {
    lcd->update();
  }
}


void LCD_Interface::Update(gpointer)
{
  if (lcd) {
    lcd->update();
  }
}


//---------------------------------------------------------------

void LcdDisplay::update()
{
  gtk_widget_queue_draw(darea);
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void LcdDisplay::create_iopin_map()
{
  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  create_pkg(14);
  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.
  m_E  = new LCD_InputPin(this, "E", eE);
  m_RW = new LCD_InputPin(this, "RW", eRW);
  m_DC = new LCD_InputPin(this, "DC", eDC);
  addSymbol(m_E);
  addSymbol(m_RW);
  addSymbol(m_DC);
  // Control
  assign_pin(4, m_DC);
  assign_pin(5, m_RW);
  assign_pin(6, m_E);
  char text[] = "d0";

  for (int i = 0; i < 8; i++) {
    text[1] = '0' + i;
    lcd_bus[i] = new IO_bi_directional(text);
    addSymbol(lcd_bus[i]);
    assign_pin(i + 7, m_dataBus->addPin(lcd_bus[i], i));
  }

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


//--------------------------------------------------------------
TraceType *LcdDisplay::getWriteTT()
{
  if (!writeTT) {
    writeTT = new LcdWriteTT(this, 1);
    gTrace->allocateTraceType(writeTT);
  }

  return writeTT;
}


TraceType *LcdDisplay::getReadTT()
{
  if (!readTT) {
    readTT = new LcdReadTT(this, 1);
    gTrace->allocateTraceType(readTT);
  }

  return readTT;
}


//--------------------------------------------------------------
// construct

Module * LcdDisplay::construct(const char *new_name = nullptr)
{
  LcdDisplay *lcdP = new LcdDisplay(new_name, 2, 20);
  lcdP->set_pixel_resolution(5, 8);
  return lcdP;
}


LcdDisplay::LcdDisplay(const char *_name, int aRows, int aCols, unsigned aType)
  : Module(_name), data_latch(0), data_latch_phase(1),
    debug(0), rows(aRows), cols(aCols), disp_type(aType), contrast(1.0),
    fontP(nullptr), readTT(new LcdReadTT(this, 1)), writeTT(new LcdWriteTT(this, 1)),
    m_controlState(0), cgram_updated(false)
{
  if (verbose) {
    std::cout << "LcdDisplay constructor\n";
  }

  new_name(_name);
  m_dataBus = new PortRegister(this, "data", "LCD Data Port", 8, 0);
  m_hd44780 = new HD44780();
  //  mode_flag = _8BIT_MODE_FLAG;
  last_event = eWC;
  set_pixel_resolution();
  set_crt_resolution();
  cursor.row = 0;
  cursor.col = 0;

  // If you want to get diagnostic info, change debug to non-zero.
  if (getenv("GPSIM_LCD_DEBUG")) {
    debug = atoi(getenv("GPSIM_LCD_DEBUG"));
  }

  gTrace = &get_trace();
  interface_seq_number = get_interface().add_interface(new LCD_Interface(this));
  addSymbol(m_dataBus);
  m_dataBus->setEnableMask(0xff);
  CreateGraphics();
  create_iopin_map();
}


LcdDisplay::~LcdDisplay()
{
  if (verbose) {
    std::cout << "LcdDisplay destructor\n";
  }

  removeSymbol(m_E);
  removeSymbol(m_RW);
  removeSymbol(m_DC);

  for (int i = 0; i < 8; i++) {
    removeSymbol(lcd_bus[i]);
  }

  removeSymbol(m_dataBus);
  delete m_dataBus;
  delete m_hd44780;
  get_interface().remove_interface(interface_seq_number);
  gtk_widget_destroy(window);

  delete fontP;
}


//------------------------------------------------------------------------
bool LcdDisplay::dataBusDirection()
{
  return m_hd44780->dataBusDirection();
}


//------------------------------------------------------------------------

void LcdDisplay::UpdatePinState(ePins pin, char cState)
{
  // One of the control lines has changed states. So refresh the
  // hd44780 with the most current data bus value.
  // If the data bus I/O's are inputs, then copy the I/O pin
  // data bus state to the chip data bus:
  if (m_hd44780->dataBusDirection()) {
    m_hd44780->driveDataBus(m_dataBus->get());
  }

  bool bState = (cState == '1') || (cState == 'W');

  switch (pin) {
  case eDC:
    m_hd44780->setDC(bState);
    break;

  case eE:
    m_hd44780->setE(bState);
    break;

  case eRW:
    m_hd44780->setRW(bState);
    break;
  }

  // If the hd44780 is driving, then place that
  // data onto the data bus.
  if (m_hd44780->dataBusDirection()) {
    m_dataBus->put(m_hd44780->getDataBus());
  }

  m_dataBus->updatePort();
  Dprintf(("Control pin:%d is %c and Databus is 0x%02X\n", pin, cState, m_dataBus->get_value()));
}


//------------------------------------------------------------------------
void LcdDisplay::testHD44780()
{
  m_hd44780->test();
}


//-----------------------------------------------------------------
// Displaytech 161A, added by Salvador E. Tropea <set@computer.org>
// construct

Module * LcdDisplayDisplaytech161A::construct(const char *new_name = NULL)
{
  if (verbose) {
    std::cout << " LCD 161A display constructor\n";
  }

  LcdDisplayDisplaytech161A *lcdP = new LcdDisplayDisplaytech161A(new_name, 2, 8, TWO_ROWS_IN_ONE);
  return lcdP;
}


LcdDisplayDisplaytech161A::LcdDisplayDisplaytech161A(const char *pN, int aRows, int aCols,
    unsigned aType) :
  LcdDisplay(pN, aRows, aCols, aType)
{
}


LcdDisplayDisplaytech161A::~LcdDisplayDisplaytech161A()
{
}


//-----------------------------------------------------------------
// HD44780 controller with 20x2 display
//

Module * LcdDisplay20x2::construct(const char *new_name = nullptr)
{
  if (verbose) {
    std::cout << " LCD 20x2 display constructor\n";
  }

  LcdDisplay20x2 *lcdP = new LcdDisplay20x2(new_name, 2, 20);
  lcdP->set_pixel_resolution(5, 8);
  return lcdP;
}


//-----------------------------------------------------------------
// HD44780 controller with 20x4 display
//

Module * LcdDisplay20x4::construct(const char *new_name = nullptr)
{
  if (verbose) {
    std::cout << " LCD 20x4 display constructor\n";
  }

  LcdDisplay20x4 *lcdP = new LcdDisplay20x4(new_name, 4, 20);
  lcdP->set_pixel_resolution(5, 8);
  return lcdP;
}


#endif // HAVE_GUI
