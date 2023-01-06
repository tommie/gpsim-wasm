/*
   Copyright (C) 2006 T. Scott Dattalo

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


#define IN_MODULE

#include "config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>

#include <src/packages.h>
#include <src/stimuli.h>
#include <src/symbol.h>
#include <src/gpsim_interface.h>
#include <src/gpsim_time.h>


#include "hd44780.h"

#include <iostream>

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


/****************************************************************
 *
 * HD44780 - gpsim module
 */

/*
 * LCD Command "Set Display Data RAM Address" = 10000000
 */

static const unsigned int LCD_CMD_SET_DDRAM  = 0x80;
static const unsigned int LCD_MASK_SET_DDRAM = 0x80;

/*
 * LCD Command "Set Display Character Generator RAM Address" = 01aaaaaa
 */

static const unsigned int LCD_CMD_SET_CGRAM  = 0x40;
static const unsigned int LCD_MASK_SET_CGRAM = 0xc0;


/*
 * LCD Command Function Set =  001dnfxx
 *  d = 1 for 8-bit interface or 0 for 4-bit interface
 *  n = for 2 line displays, n=1 allows both lines to be displayed
 *      while n=0 only allows the first.
 * f = font size. f=1 is for 5x11 dots while f=0 is for 5x8 dots.
 */

static const unsigned int LCD_CMD_FUNC_SET  = 0x20;    // LCD Command "Function Set"
static const unsigned int LCD_MASK_FUNC_SET = 0xe0;    //
static const unsigned int LCD_4bit_MODE     = 0x00;    // d=0
static const unsigned int LCD_8bit_MODE     = 0x10;    // d=1
static const unsigned int LCD_1_LINE        = 0x00;    // n=0
static const unsigned int LCD_2_LINES       = 0x08;    // n=1
static const unsigned int LCD_SMALL_FONT    = 0x00;    // f=0
static const unsigned int LCD_LARGE_FONT    = 0x04;    // f=1

/*
 * LCD Command "Cursor Display" = 0001sdxx
 *  s = 1 Sets cursor-move or display-shift
 *  d = 1 Shift right 0 = shift left
 */

static const unsigned int LCD_CMD_CURSOR_DISPLAY   = 0x10;   // LCD Command "Cursor Display"
static const unsigned int LCD_MASK_CURSOR_DISPLAY  = 0xf0;   //

/*
 * LCD Command Display Control = 00001dcb
 *  d = 1 turn display on or 0 to turn display off
 *  c = 1 turn cursor on or 0 to turn cursor off
 *  b = 1 blinking cursor or 0 non-blinking cursor
 */

static const unsigned int LCD_CMD_DISPLAY_CTRL  = 0x08;    // LCD Command "Display Control"
static const unsigned int LCD_MASK_DISPLAY_CTRL = 0xf8;    //
static const unsigned int LCD_DISPLAY_OFF       = 0x00;    // d=0
static const unsigned int LCD_DISPLAY_ON        = 0x04;    // d=1
static const unsigned int LCD_CURSOR_OFF        = 0x00;    // c=0
static const unsigned int LCD_CURSOR_ON         = 0x02;    // c=1
static const unsigned int LCD_BLINK_OFF         = 0x00;    // b=0
static const unsigned int LCD_BLINK_ON          = 0x01;    // b=1


/*
 * LCD Command "Entry Mode" = 000001is
 *  i = 1 to increment or 0 to decrement the DDRAM address after each DDRAM access.
 *  s = 1 to scroll the display in the direction specified by the i-bit when the
 *       cursor reaches the edge of the display window.
 */

static const unsigned int LCD_CMD_ENTRY_MODE  = 0x04;    // LCD Command "Entry Mode"
static const unsigned int LCD_MASK_ENTRY_MODE = 0xfc;    //
static const unsigned int LCD_DEC_CURSOR_POS  = 0x00;    // i=0
static const unsigned int LCD_INC_CURSOR_POS  = 0x02;    // i=1
static const unsigned int LCD_NO_SCROLL       = 0x00;    // s=0
static const unsigned int LCD_SCROLL          = 0x01;    // s=1

/*
 * LCD Command "Cursor Home" = 0000001x
 */

static const unsigned int LCD_CMD_CURSOR_HOME   = 0x02;   // LCD Command "Cursor Home"
static const unsigned int LCD_MASK_CURSOR_HOME  = 0xfe;   //

// LCD Command Clear Display = 00000001
static const unsigned int LCD_CMD_CLEAR_DISPLAY  = 0x01;
static const unsigned int LCD_MASK_CLEAR_DISPLAY = 0xff;


//========================================================================
// Busy - a class to handle the HD44780's busy flag
//
// This class is essentially a timer. When the LCD requests that the
// busy flag be set for a period of time, it will call the set() method
// and pass the amount of time it wishes the flag to be busy. When
// this time expires, the flag will get cleared.

class HD44780Busy : public TriggerObject {
public:
  HD44780Busy()
    : bBusyState(false) {}
  void set(double waitTime);
  void clear();
  inline bool isBusy()
  {
    return bBusyState;
  }
  virtual void callback();
private:
  bool bBusyState;
};


//--------------------------------------------------------------
void HD44780Busy::set(double waitTime)
{
  if (!bBusyState) {
    bBusyState = true;
    get_cycles().set_break(get_cycles().get(waitTime), this);
  }
}


void HD44780Busy::clear()
{
  clear_trigger();
  bBusyState = false;
}


//--------------------------------------------------------------
void HD44780Busy::callback()
{
  bBusyState = false;
}


//------------------------------------------------------------------------
HD44780::HD44780()
  : m_bE(true),
    m_controlState(0),
    m_chipState(ePowerON),
    m_dataBus(0),
    m_phasedData(0),
    m_bBitMode(true),    // 8-bit mode
    m_bLineMode(false),  // 1-line mode
    m_bFontMode(false),  // small font
    m_bDisplayOn(false), // display is off
    m_bCursorBlink(false),// no cursor blink
    m_bCursorOn(false),   // cursor is off
    m_bDataBusPhase(false),
    m_DDRamAdd(0),
    m_CGRamAdd(0),
    m_bInCGRam(false),
    m_CGRamupdate(false)
{
  m_busyTimer = new HD44780Busy();
  memset(&m_CGRam[0], 0xff, sizeof(m_CGRam));
  memset(&m_DDRam[0], 0xff, sizeof(m_DDRam) / 2);
  memset(&m_DDRam[DDRAM_SIZE / 2], 0, sizeof(m_DDRam) / 2);
  row_offset[0] = 0;
  row_offset[1] = 0x40;
  row_offset[2] = 0x14;
  row_offset[3] = 0x54;
}


HD44780::~HD44780()
{
  delete m_busyTimer;
}


//------------------------------------------------------------------------
void HD44780::setDC(bool newDC)
{
  m_controlState &= ~eDC;
  m_controlState |= (newDC ? eDC : 0);
}


void HD44780::setRW(bool newRW)
{
  m_controlState &= ~eRW;
  m_controlState |= (newRW ? eRW : 0);
}


void HD44780::setE(bool newE)
{
  bool bRW = (m_controlState & eRW) != 0;

  if (m_bE != newE  && m_bE ^ bRW) {
    // Only act if there was a change in E
    // FIXME: what happens on the real chip if DC or RW change while E is high?

    // There are only 4 states corresponding to the combinations of RW and DC.
    // Note. For reads, we act on the rising edge of E while for writes on
    // the falling edge.
    switch (m_controlState) {
    case eDataRead:
      driveDataBus(getData());
      advanceColumnAddress();
      break;

    case eCommandRead:
      driveDataBus(getStatus());
      break;

    case eDataWrite:
      storeData();
      advanceColumnAddress();
      break;

    case eCommandWrite:
      executeCommand();
      break;

    default:
      Dprintf((" unhandled control state:%u\n", m_controlState));
    }
  }

  m_bE = newE;
}


//------------------------------------------------------------------------
// driveDataBus - place an 8-bit value on the data bus.
// Note that this routine doesn't directly affect the I/O pins of an LCD module.
// The Pin control code resides elsewhere (lcd.cc) and will read the HD44780
// data bus to determine how the pins should be driven
void HD44780::driveDataBus(unsigned int d)
{
  //RRR Dprintf(("driveDataBus 0x%02x\n",d));
  m_dataBus = d;
}


//------------------------------------------------------------------------
// advanceColumnAddress() - A read or write from the DDRAM increments
// the column address. If we're in 4-bit mode, then only increment after
// the 2nd read or write.
void HD44780::advanceColumnAddress()
{
  if (b8BitMode() || m_bDataBusPhase) {
    if (m_bInCGRam) {
      m_CGRamAdd = (m_CGRamAdd + 1) & CGRAM_MASK;

    } else {
      m_DDRamAdd = (m_DDRamAdd + 1) & DDRAM_MASK;
    }
  }
}


//------------------------------------------------------------------------
// phasedDataWrite - returns true if a write operation complements. The data
// written is returned by reference.
// Write operations always complete in 8-bit mode, but take two phases in
// 4-bit mode.
bool HD44780::phasedDataWrite(unsigned int &data)
{
  if (b8BitMode()) {
    data = m_dataBus & 0xff;
    return true;
  }

  // In 4-bit mode, the upper nibble is written first.
  // First, move the last nibble written to the upper half of phasedData
  m_phasedData &= 0x0f;
  m_phasedData <<= 4;
  // Next, get the nibble on the data bus and put it in the lower half of phasedData.
  m_phasedData |= ((m_dataBus >> 4) & 0x0f);
  data = m_phasedData;
  // Toggle the phase.
  // FIXME - The phase logic needs more attention. There should be a method for
  // setting the phase.
  m_bDataBusPhase = !m_bDataBusPhase;
  return m_bDataBusPhase;
}


//------------------------------------------------------------------------
// storeData - write a byte to the DDRAM or CGRAM
//
void HD44780::storeData()
{
  unsigned int d;

  if (phasedDataWrite(d)) {
    if (m_bInCGRam) {
      m_CGRam[m_CGRamAdd] = d;
      m_CGRamupdate = true;

    } else {
      m_DDRam[m_DDRamAdd] = d;
    }
  }
}


//------------------------------------------------------------------------
// getData - get a byte from tehe DDRAM
unsigned int HD44780::getData()
{
  if (m_bInCGRam) {
    return m_CGRam[m_CGRamAdd];

  } else {
    return m_DDRam[m_CGRamAdd];
  }
}


void HD44780::executeCommand()
{
  unsigned int command;

  // command is set in the following call
  if (!phasedDataWrite(command)) {
    return;
  }

  //
  // Determine the command type
  //
  Dprintf(("Execute Command:0x%u\n", command));

  if ((command & LCD_MASK_SET_DDRAM) ==  LCD_CMD_SET_DDRAM) {
    Dprintf(("LCD_CMD_SET_DDRAM\n"));
    writeDDRamAddress(command & DDRAM_MASK);
    m_busyTimer->set(39e-6);	// busy for 39 usec after set DDRAM addr

  } else if ((command & LCD_MASK_SET_CGRAM) ==  LCD_CMD_SET_CGRAM) {
    Dprintf(("LCD_CMD_SET_CGRAM\n"));
    writeCGRamAddress(command & CGRAM_MASK);

  } else if ((command & LCD_MASK_FUNC_SET) == LCD_CMD_FUNC_SET) {
    Dprintf(("LCD_CMD_FUNC_SET\n"));

    //
    // Check the bits in the command
    //

    if (command & LCD_8bit_MODE) {
      set8bitMode();

    } else {
      set4bitMode();
      // If the lcd is in '4-bit' mode, then this flag
      // will tell us which four bits are being written.
      m_bDataBusPhase = true;
    }

    if (command & LCD_2_LINES) {
      set2LineMode();

    } else {
      set1LineMode();
    }

    if (command & LCD_LARGE_FONT) {
      setLargeFontMode();

    } else {
      setSmallFontMode();
    }

    m_busyTimer->set(39e-6);	// busy for 39 usec after DDRAM write

  } else if ((command & LCD_MASK_CURSOR_DISPLAY) ==  LCD_CMD_CURSOR_DISPLAY) {
    printf("LCD_CMD_CURSOR_DISPLAY\n");
    printf("NOT SUPPORTED\n");

  } else if ((command & LCD_MASK_DISPLAY_CTRL) == LCD_CMD_DISPLAY_CTRL) {
    Dprintf(("LCD_CMD_DISPLAY_CTRL\n"));

    if (command & LCD_DISPLAY_ON) {
      setDisplayOn();

    } else {
      setDisplayOff();
    }

    if (command & LCD_CURSOR_ON) {
      setCursorOn();

    } else {
      setCursorOff();
    }

    if (command & LCD_BLINK_ON) {
      setBlinkOn();

    } else {
      setBlinkOff();
    }

  } else if ((command & LCD_MASK_ENTRY_MODE) == LCD_CMD_ENTRY_MODE) {
    if ((command & ~LCD_MASK_ENTRY_MODE) != LCD_INC_CURSOR_POS) {
      std::cout << "LCD_CMD_ENTRY_MODE\nNOT SUPPORTED\n";

    } else {
      Dprintf(("LCD_CMD_ENTRY_MODE cursorpos=inc scroll=no\n"));
    }

  } else if ((command & LCD_MASK_CURSOR_HOME) ==  LCD_CMD_CURSOR_HOME) {
    Dprintf(("LCD_CMD_CURSOR_HOME\n"));
    m_DDRamAdd = 0;

  } else if ((command & LCD_MASK_CLEAR_DISPLAY) == LCD_CMD_CLEAR_DISPLAY) {
    Dprintf(("LCD_CMD_CLEAR_DISPLAY\n"));
    clearDisplay();
    m_busyTimer->set(1350e-6);	// busy for 1.3 msec after clear screen

  } else {
    Dprintf(("UNKOWN command : 0x%x\n", command));
  }

  debugChipState(__FUNCTION__);
}


//------------------------------------------------------------------------
// dataBusDirection() - returns true if the data bus is an input
bool HD44780::dataBusDirection()
{
  return !((m_controlState & eRW) && m_bE);
}


//------------------------------------------------------------------------
unsigned int HD44780::getDataBus()
{
  return m_dataBus;
}


//------------------------------------------------------------------------
//
// send_status

unsigned int HD44780::getStatus()
{
  unsigned short status;

  if (m_bInCGRam) {
    status = m_CGRamAdd;

  } else {
    status = m_DDRamAdd;
  }

  if (m_busyTimer->isBusy()) {
    status |= 0x80;
  }

  return dataPhase(status);
}


//------------------------------------------------------------------------
//
unsigned int HD44780::dataPhase(unsigned int d)
{
  if (b8BitMode()) {
    return d;
  }

  m_bDataBusPhase = !m_bDataBusPhase;
  return !m_bDataBusPhase ? ((d << 4) & 0xf0) : d;
}


//------------------------------------------------------------------------
void HD44780::writeDDRamAddress(int data)
{
  //
  // The first 0x40 memory locations are mapped to
  // row 0 and the second 0x40 to row 1. Now only
  // the first 40 (decimal not hex) locations are
  // valid RAM. And of course, only the first 20
  //of these can be displayed in a 2x20 display.
  //
  data &= DDRAM_MASK;
  m_DDRamAdd = data;
  m_bInCGRam = false;
}


unsigned char HD44780::getDDRam(unsigned int r, unsigned int c)
{
  int add;

  if (r >= 4) {
    fprintf(stderr, "%s row %u not supported\n", __FUNCTION__, r);
    return 0;
  }

  add = row_offset[r] + c;
  return m_DDRam[add & DDRAM_MASK];
}


//------------------------------------------------------------------------
void HD44780::writeCGRamAddress(int data)
{
  data &= CGRAM_MASK;
  m_CGRamAdd = data;
  m_bInCGRam = true;
}


//------------------------------------------------------------------------
void HD44780::clearDisplay()
{
  memset(&m_DDRam[0], ' ', sizeof(m_DDRam));
  m_DDRamAdd = 0;
}


//------------------------------------------------------------------------
void HD44780::debugChipState(const char *pCFrom)
{
#ifdef DEBUG
  printf("Chip state from %s\n", pCFrom);
  printf(" ControlState: %u  dataBus:0x%x phase:%d\n",
         m_controlState, m_dataBus, m_bDataBusPhase);
  printf(" Mode: %dbit %dLine Display-%s\n",
         (b8BitMode() ? 8 : 4),
         (b1LineMode() ? 1 : 2),
         (bDisplayOn() ? "ON" : "OFF"));
  printf(" DDRam Address:0x%02x  CGRam Address:0x%02x CGRam active %d\n",
         m_DDRamAdd,
         m_CGRamAdd, m_bInCGRam);
#else
(void)pCFrom;
#endif
}


static void printTestResult(bool b, const char * testName)
{
  printf(" %s:%s\n", testName, (b ? "PASSED" : "FAILED"));
}


//------------------------------------------------------------------------
void HD44780::test()
{
  printf("HD44780 self test\n");
  set8bitMode();
  setRW(false);
  setDC(false);
  driveDataBus(LCD_CMD_FUNC_SET | LCD_8bit_MODE);
  setE(true);
  setE(false);
  printTestResult(b8BitMode(), "setting 8-bit mode");
  driveDataBus(LCD_CMD_FUNC_SET | LCD_4bit_MODE);
  setE(true);
  setE(false);
  printTestResult(b4BitMode(), "setting 4-bit mode");
  driveDataBus(LCD_CMD_FUNC_SET | LCD_4bit_MODE | LCD_2_LINES | LCD_SMALL_FONT);
  setE(true);
  setE(false);
  driveDataBus((LCD_CMD_FUNC_SET | LCD_4bit_MODE | LCD_2_LINES | LCD_SMALL_FONT) << 4);
  setE(true);
  setE(false);
  printTestResult(b2LineMode(), "setting small font & 2-line modes");
  driveDataBus(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON);
  setE(true);
  setE(false);
  driveDataBus((LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON) << 4);
  setE(true);
  setE(false);
  printTestResult(bDisplayOn(), "turning on display");
  driveDataBus(LCD_CMD_CLEAR_DISPLAY);
  setE(true);
  setE(false);
  driveDataBus(LCD_CMD_CLEAR_DISPLAY << 4);
  setE(true);
  setE(false);
  const char *s = "ASHLEY & AMANDA";
  int l = strlen(s);
  int i;
  setDC(true);

  for (i = 0; i < l; i++) {
    driveDataBus(s[i]);
    setE(true);
    setE(false);
    driveDataBus(s[i] << 4);
    setE(true);
    setE(false);
  }

  printf("DDRam contents:\n");

  for (i = 0; i < DDRAM_SIZE; i++) {
    char ch = m_DDRam[i];

    if (i == 40) {
      printf("\n");
    }

    printf("%c", (ch >= 0x20 ? ch : '.'));
  }

  printf("\n");
  set8bitMode();
#if 0
  set_8bit_mode();
  viewInternals(0xff);
#endif
}


#endif // HAVE_GUI
