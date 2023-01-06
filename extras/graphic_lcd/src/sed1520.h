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


#ifndef EXTRAS_GRAPHIC_LCD_SRC_GLCD_100X32_SED1520_H__SED1520_H_
#define EXTRAS_GRAPHIC_LCD_SRC_GLCD_100X32_SED1520_H__SED1520_H_

class SED1520 {
  enum {
    eNPAGES = 4,
    eNCOLUMNS_PER_PAGE = 80
  };

  enum {
    eA0 = 1,
    eRW = 2,
    eE  = 4,

    eDataRead = eA0 | eRW,
    eDataWrite = eA0,
    eStatusRead = eRW,
    eCommandWrite = 0
  };

public:
  SED1520();

  void setA0(bool);
  void setE(bool);
  void setRW(bool);
  void setData(unsigned int);
  void driveDataBus(unsigned int);
  bool dataBusDirection();
  unsigned int getDataBus();
  void advanceColumnAddress();
  void storeData();
  void executeCommand();

  void randomizeRAM();

  unsigned int getData();
  unsigned int getStatus();

  inline unsigned int &operator[](unsigned int index)
  {
    return (index < eNCOLUMNS_PER_PAGE * eNPAGES) ?  m_ram[index] : prBadRam(index);
  }

private:
  // called if an illegal access is made to the RAM
  unsigned int &prBadRam(unsigned int);

  /// I/O pin interface
  bool m_bE;
  unsigned int m_controlState;
  unsigned int m_dataBus;

  // Display ram
  unsigned int m_ram[eNCOLUMNS_PER_PAGE * eNPAGES];

  // State information
  unsigned int m_page;
  unsigned int m_columnAddress;

};

#endif 
