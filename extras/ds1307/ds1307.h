/*
   Copyright (C) 2006 T. Scott Dattalo
   Copyright (C) 2010 Roy R Rankin

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


#ifndef EXTRAS_DS1307_DS1307_H_
#define EXTRAS_DS1307_DS1307_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE
#define LOCAL_TIME

#include "src/modules.h"
#include "src/ioports.h"
#include "src/stimuli.h"
#include "src/trigger.h"
#include "src/i2c-ee.h"


class PromAddress;
class I2C_RTC;
class SQW_PIN;

namespace DS1307_Modules {

class ds1307 : public Module, public TriggerObject {
public:
  explicit ds1307(const char *_name);
  ~ds1307();

  static Module *construct_ds1307(const char *new_name);
  virtual void create_iopin_map();
  virtual void setEnable(bool /* bNewState */ , unsigned int /* m_bit */ ) {}
  virtual void callback();
  void secWritten(unsigned int sec);
  void controlWritten(unsigned int cntl);

  enum control {
    RS0 = 1 << 0,
    RS1 = 1 << 1,
    SQWE = 1 << 4,
    OUT = 1 << 7
  };

  enum second {
    CH = 1 << 7
  };

protected:
  void incrementRTC();

  I2C_RTC *m_eeprom;
  SQW_PIN *m_sqw;
  unsigned int chip_select;	// Write Protect and A0 - A2 state
  PromAddress *att_eeprom;
  guint64 next_clock_tick;	// increment RTC here
  guint64 next_sqw_edge;	// change sqw edge here
  guint64 sqw_interval;	// cycles between edges
  bool    out;		// Output state
};


} // end of namespace DS1307_Modules

class I2C_RTC : public I2C_EE {
public:
  I2C_RTC(Processor *pCpu,
          unsigned int _rom_size, unsigned int _write_page_size = 1,
          unsigned int _addr_bytes = 1, unsigned int _CSmask = 0,
          unsigned int _BSmask = 0, unsigned int _BSshift = 0
         );
  virtual ~I2C_RTC() {}

protected:
  virtual bool match_address();
  DS1307_Modules::ds1307 *pEE;
};


#endif // EXTRAS_DS1307_DS1307_H_
