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


#ifndef EXTRAS_GRAPHIC_LCD_SRC_SSD0323_H_
#define EXTRAS_GRAPHIC_LCD_SRC_SSD0323_H_

class SSD0323 {
  enum {
    e8080Mode = 6,
    e6800Mode = 4,
    eSPIMode  = 0,
    eInvalidMode = 0xff
  };

  enum {                 // 8080   6800    SPI
    eCS    = 1,          // /CS    /CS    /CS
    eRES   = 2,          // /Reset /Reset /Reset
    eE_RD  = 4,          // /Rd    Enable /Enable
    eRW    = 8,          // /Wr    Rd /Wr /Wr
    eDC    = 0x10        // D /C   D /C   D /C   --- Data /Command
  };

public:
  SSD0323();

  void setBS(unsigned int BSpin, bool newBS);
  void setCS(bool);
  void setRES(bool);
  void setDC(bool);
  void setE_RD(bool);
  void setRW(bool);
  void setData(unsigned int);
  void setSCLK(bool);
  void setSDIN(bool);

  void driveDataBus(unsigned int);
  bool dataBusDirection();
  unsigned int getDataBus();


  void storeData();
  void executeCommand();

  void randomizeRAM();

  unsigned int getData();
  unsigned int getStatus();

  inline unsigned int &operator[](unsigned int index)
  {
    return (index < 128 * 80 / 2) ?  m_ram[index] : prBadRam(index);
  }

  void showState();

private:
  void advanceColumnAddress();
  void advanceRowAddress();

  // called if an illegal access is made to the RAM
  unsigned int &prBadRam(unsigned int);

  // Display state
  unsigned int m_controlState;
  unsigned int m_dataBus;

  // Communication -
  // The commMode selects one of three possible communication modes
  // The commState holds the communication state for that mode.

  unsigned int m_commMode;
  unsigned int m_commState;
  unsigned int m_SPIData;

  // Command processing
  unsigned int m_commandIndex;
  unsigned int m_expectedCommandWords;
  unsigned char cmdWords[20];

  // Display ram + one for bad memory access
  unsigned int m_ram[(128 * 80 / 2) + 1];

  // State information
  unsigned int m_colAddr;
  unsigned int m_rowAddr;

  // helper functions
  bool bEnabled()
  {
    return (m_controlState & (eCS | eRES)) == eRES;
  }
  bool bBitState(unsigned int b)
  {
    return (m_controlState & b) == b;
  }
  void abortCurrentTransaction();

  // configuration data controlled by the SSD0323 commands
  unsigned int m_colStartAddr;
  unsigned int m_colEndAddr;
  unsigned int m_rowStartAddr;
  unsigned int m_rowEndAddr;
  unsigned int m_Remap;
  unsigned int m_ContrastControl;
};

#endif 
