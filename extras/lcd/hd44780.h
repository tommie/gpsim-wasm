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


#ifndef EXTRAS_LCD_HD44780_H_
#define EXTRAS_LCD_HD44780_H_

//========================================================================
//
class HD44780Busy;

class HD44780 {
  enum {
    eDC = 1,
    eRW = 2,
    eE  = 4,
  };

  enum eControlStates {
    eDataRead =     eDC | eRW,
    eDataWrite =    eDC,
    eCommandRead =  eRW,
    eCommandWrite = 0     //DC and RW low
  };

  enum eChipStates {
    ePowerON,
    eInitialized,
    eCommandPH0,
    eStatusRead,
    eDatabusRead,
  };

public:
  HD44780();
  ~HD44780();

  void setDC(bool);
  void setE(bool);
  void setRW(bool);
  void driveDataBus(unsigned int);
  bool dataBusDirection();
  unsigned int getDataBus();
  void storeData();
  void advanceColumnAddress();
  void executeCommand();

  unsigned int getData();
  unsigned int getStatus();
  //  void advanceState(eControlStates newControlState);

  void set8bitMode()      { m_bBitMode = true;};
  void set4bitMode()      { m_bBitMode = false; }
  void set2LineMode()     { m_bLineMode = true; }
  void set1LineMode()     { m_bLineMode = false;}
  void setLargeFontMode() { m_bFontMode = true; }
  void setSmallFontMode() { m_bFontMode = false; }
  void setDisplayOn()     { m_bDisplayOn = true;}
  void setDisplayOff()    { m_bDisplayOn = false;}
  void setBlinkOn()       { m_bCursorBlink = true; }
  void setBlinkOff()      { m_bCursorBlink = false;}
  void setCursorOn()      { m_bCursorOn = true; }
  void setCursorOff()     { m_bCursorOn = false; }

  bool b8BitMode() {return m_bBitMode; }
  bool b4BitMode() {return !m_bBitMode; }
  bool b2LineMode() {return m_bLineMode; }
  bool b1LineMode() {return !m_bLineMode; }
  bool bLargeFontMode() {return m_bFontMode; }
  bool bSmallFontMode() {return !m_bFontMode; }
  bool bDisplayOn() {return m_bDisplayOn; }
  bool bDisplayOff() {return !m_bDisplayOn; }
  bool CGRamupdate() {return m_CGRamupdate;}
  void setCGRamupdate(bool state) { m_CGRamupdate = state;}

  // Memory in a HD44780U 128 bytes of display RAM,
  // and 64 bytes of character RAM
#define DDRAM_SIZE 0x80
#define DDRAM_MASK (DDRAM_SIZE - 1)
#define CGRAM_ADDR_BITS 6
#define CGRAM_SIZE (1 << CGRAM_ADDR_BITS)
#define CGRAM_MASK (CGRAM_SIZE - 1)

  unsigned char getDDRam(unsigned int r, unsigned int c);
  unsigned char getCGRam(unsigned int i) {return i < CGRAM_SIZE ? m_CGRam[i] : 0; }
  void writeCGRamAddress(int addr);
  void writeDDRamAddress(int addr);

  void clearDisplay();

  void test();

protected:
  unsigned int dataPhase(unsigned int);
  bool phasedDataWrite(unsigned int &data);

private: //Data
  /// I/O pin interface
  bool m_bE;
  unsigned int m_controlState;
  eChipStates  m_chipState;
  unsigned int m_dataBus;      // Data placed onto the I/O pins
  unsigned int m_phasedData;   // Data constructed during 4-bit mode.

  /// Various modes
  bool m_bBitMode;     // true == 8-bit mode, false == 4-bit mode
  bool m_bLineMode;    // true == 2-line mode, false == 1-line mode
  bool m_bFontMode;    // true == Large Font, false == small font
  bool m_bDisplayOn;
  bool m_bCursorBlink;
  bool m_bCursorOn;

  /// Data phase
  ///  In 4-bit mode, data reads and writes happen in 2 phases
  ///  m_bDataPhase is true if this is the first phase, false for the second.
  bool m_bDataBusPhase;

  HD44780Busy *m_busyTimer;

  /// Display Data RAM
  unsigned char m_DDRam[DDRAM_SIZE];
  unsigned char m_DDRamAdd;

  /// Character Generator RAM
  unsigned char m_CGRam[CGRAM_SIZE];
  unsigned char m_CGRamAdd;
  bool m_bInCGRam;           // true when a write to CGRAM is selected
  bool m_CGRamupdate;

  int row_offset[4];		// support up to 4 rows

  /// Debug
  void debugChipState(const char *);
};


#endif // EXTRAS_LCD_HD44780_H_
