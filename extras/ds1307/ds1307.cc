/*
   Copyright (C) 2010 Roy R Rankin
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


#include <stdio.h>
#include <time.h>

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include <config.h>    // get the definition for HAVE_GUI
#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "ds1307.h"
#include "../../src/gpsim_time.h"
#include "../../src/stimuli.h"
#include "../../src/i2c-ee.h"
#include "../../src/ioports.h"
#include "../../src/symbol.h"
#include "../../src/value.h"
#include "../../src/packages.h"
#include "../../src/gpsim_interface.h"

class Processor;

I2C_RTC::I2C_RTC(Processor *pCpu, unsigned int _rom_size,
                 unsigned int _write_page_size,
                 unsigned int _addr_bytes, unsigned int _CSmask,
                 unsigned int _BSmask, unsigned int _BSshift)
  : I2C_EE(pCpu,  _rom_size,  _write_page_size,
           _addr_bytes,  _CSmask, _BSmask,  _BSshift)
{
  pEE = (DS1307_Modules::ds1307 *)pCpu;
  i2c_slave_address = 0xd0;
}


bool I2C_RTC::match_address()
{
  Dprintf(("match_address I2C got address=0x%x (w=%d) want 0x%x\n", (xfr_data & 0xfe), (xfr_data & 0x01), i2c_slave_address));
  return ((xfr_data & 0xfe) == i2c_slave_address);
}


class SQW_PIN : public IO_open_collector {
public:
  explicit SQW_PIN(const char *_name)
    : IO_open_collector(_name)
  {
    bDrivingState = true;
    bDrivenState = true;
    // Make the pin an output.
    update_direction(IO_bi_directional::DIR_OUTPUT, true);
  }

  void setDrivingState(bool new_state)
  {
    bDrivingState = new_state;
    bDrivenState = new_state;

    if (snode) {
      snode->update();
    }
  }
};


namespace DS1307_Modules {

ds1307::ds1307(const char *_name)
  : Module(_name, "BS1307"),
    m_eeprom(nullptr), m_sqw(nullptr),
    chip_select(0),
    att_eeprom(nullptr),
    next_clock_tick(0), next_sqw_edge(0), sqw_interval(0),
    out(false)
{
}


ds1307::~ds1307()
{
  removeSymbol(m_sqw);
  removeSymbol((IOPIN *)(m_eeprom->sda));
  removeSymbol((IOPIN *)(m_eeprom->scl));
  m_eeprom->sda = 0;
  m_eeprom->scl = 0;
  delete att_eeprom;
  delete m_eeprom;
}


Module *ds1307::construct_ds1307(const char *_new_name)
{
  // std::string att_name = _new_name;        ??? Does not appear to have a use
  ds1307 *pEE = new ds1307(_new_name);
  // I2C_EE size in bytes prom size in bits
  (pEE->m_eeprom) = new I2C_RTC((Processor *)pEE, 64, 16, 1, 0xe, 0, 0);
  pEE->create_iopin_map();
  // att_name += ".ds1307";
  pEE->att_eeprom = new PromAddress(pEE->m_eeprom, "eeprom", "Address I2C_RTC");
  pEE->addSymbol(pEE->att_eeprom);
#ifdef LOCAL_TIME
  struct tm *tm;
  time_t t = time(nullptr);
  int val;
  tm = localtime(&t);
  val = tm->tm_sec;
  (pEE->m_eeprom->get_register(0))->put((val % 10) + ((val / 10) << 4));
  val = tm->tm_min;
  (pEE->m_eeprom->get_register(1))->put((val % 10) + ((val / 10) << 4));
  val = tm->tm_hour;
  (pEE->m_eeprom->get_register(2))->put((val % 10) + ((val / 10) << 4) + 0x40);
  (pEE->m_eeprom->get_register(3))->put(tm->tm_wday + 1);
  val = tm->tm_mday;
  (pEE->m_eeprom->get_register(4))->put((val % 10) + ((val / 10) << 4));
  val = tm->tm_mon + 1;
  (pEE->m_eeprom->get_register(5))->put((val % 10) + ((val / 10) << 4));
  val = tm->tm_year % 100;
  (pEE->m_eeprom->get_register(6))->put((val % 10) + ((val / 10) << 4));
  (pEE->m_eeprom->get_register(7))->put(0x10);
  pEE->controlWritten(0x10);
#else
  // factory defaults
  (pEE->m_eeprom->get_register(0))->put(0x80);
  (pEE->m_eeprom->get_register(1))->put(0x00);
  (pEE->m_eeprom->get_register(2))->put(0x00);
  (pEE->m_eeprom->get_register(3))->put(0x01);
  (pEE->m_eeprom->get_register(4))->put(0x01);
  (pEE->m_eeprom->get_register(5))->put(0x01);
  (pEE->m_eeprom->get_register(6))->put(0x00);
  (pEE->m_eeprom->get_register(7))->put(0x00);
#endif
  return pEE;
}


void ds1307::create_iopin_map()
{
  m_sqw = new SQW_PIN("SQW");
  addSymbol(m_sqw);
  addSymbol((IOPIN *)(m_eeprom->sda));
  addSymbol((IOPIN *)(m_eeprom->scl));
  package = new Package(8);
  package->assign_pin(1, 0);
  package->assign_pin(2, 0);
  package->assign_pin(3, 0);
  package->assign_pin(5, (IOPIN *)(m_eeprom->sda));
  package->assign_pin(6, (IOPIN *)(m_eeprom->scl));
  package->assign_pin(7, m_sqw);
  m_sqw->update_direction(1, true);
}


void ds1307::controlWritten(unsigned int cntl)
{
  if (cntl & SQWE) {
    unsigned int new_sqw_interval = 0;

    switch (cntl & (RS1 | RS0)) {
    case 0:
      new_sqw_interval = 0.5 / (get_cycles().seconds_per_cycle());
      break;

    case 1:
      new_sqw_interval = 0.5 / (4096.0 * get_cycles().seconds_per_cycle());
      break;

    case 2:
      new_sqw_interval = 0.5 / (8192.0 * get_cycles().seconds_per_cycle());
      break;

    case 3:
      new_sqw_interval = 0.5 / (32768.0 * get_cycles().seconds_per_cycle());
      break;
    }

    if (!new_sqw_interval) {
      fprintf(stderr, "DS1307 SQW faster than can be simulated\n");
      new_sqw_interval = 1;
    }

    int sec = (m_eeprom->get_register(0))->get();

    if (!(sec & CH)) {
      if (next_sqw_edge) {
        if (new_sqw_interval != sqw_interval) {
          get_cycles().clear_break(next_sqw_edge);
          next_sqw_edge = new_sqw_interval - sqw_interval;
          get_cycles().set_break(next_sqw_edge, this);
        }

      } else {
        out = false;
        m_sqw->setDrivingState(out);
        next_sqw_edge = get_cycles().get() + new_sqw_interval;
        get_cycles().set_break(next_sqw_edge, this);
      }

      if (!next_clock_tick) {
        next_clock_tick = get_cycles().get() + get_cycles().instruction_cps();
        get_cycles().set_break(next_clock_tick, this);
      }
    }

    sqw_interval = new_sqw_interval;

  } else {
    sqw_interval = 0;

    if (next_sqw_edge) { // stop the SQW output
      get_cycles().clear_break(next_sqw_edge);
      next_sqw_edge = 0;
    }

    m_sqw->setDrivingState((cntl & OUT) == OUT);
  }
}


void ds1307::secWritten(unsigned int sec)
{
  if (sec & CH) {	// Turn off clock functions
    Dprintf(("DS1307 Turn off clock\n"));

    if (next_sqw_edge) { // stop the SQW output
      get_cycles().clear_break(next_sqw_edge);
      next_sqw_edge = 0;
    }

    if (next_clock_tick) { // stop the SQW output
      get_cycles().clear_break(next_clock_tick);
      next_clock_tick = 0;
    }

  } else {	// Clock functions on
    Dprintf(("Clock functions on\n"));

    if (next_clock_tick) {	// resync ticks
      get_cycles().clear_break(next_clock_tick);
    }

    next_clock_tick = get_cycles().get() + get_cycles().instruction_cps();
    get_cycles().set_break(next_clock_tick, this);

    if (next_sqw_edge) {
      get_cycles().clear_break(next_sqw_edge);
    }

    if (sqw_interval) {
      next_sqw_edge = get_cycles().get() + sqw_interval;
      get_cycles().set_break(next_sqw_edge, this);
    }
  }
}


void ds1307::incrementRTC()
{
  int i;
  int value;
  bool is12h;
  unsigned char dom;
  unsigned char month;
  unsigned int year;
  unsigned char dim[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  value = (m_eeprom->get_register(0))->get();

  if (value & 0x80) {	// return if clock halt set
    return;
  }

  i = (value >> 4) * 10 + (value & 0xf) + 1;

  if (i < 60) {
    value = i % 10;
    value = (i >= 10) ? value | ((i / 10) << 4) : value;
    (m_eeprom->get_register(0))->put(value);
    return;
  }

  (m_eeprom->get_register(0))->put(0);
  value = (m_eeprom->get_register(1))->get();	// Get minutes
  i = (value >> 4) * 10 + (value & 0xf) + 1;

  if (i < 60) {
    value = i % 10;
    value = (i >= 10) ? value | ((i / 10) << 4) : value;
    (m_eeprom->get_register(1))->put(value);
    return;
  }

  (m_eeprom->get_register(1))->put(0);
  value = (m_eeprom->get_register(2))->get();	// Get Hours
  is12h = (value & 0x40) == 0x40;

  if (is12h) {
    bool isPM;
    isPM = (value & 0x20) == 0x20;
    i = ((value & 1) >> 4) * 10 + (value & 0xf) + 1;

    if (i == 12 && ! isPM) {
      isPM = true;
      i = 0;
    }

    if (i < 12) {
      value = i % 10;
      value = (i >= 10) ? value | ((i / 10) << 4) : value;
      value |= isPM ? 0x60 : 0x40;
      (m_eeprom->get_register(2))->put(value);
      return;
    }

    value = 0x40;
    (m_eeprom->get_register(2))->put(value);

  } else {
    i = (value  >> 4) * 10 + (value & 0xf) + 1;

    if (i < 24) {
      value = i % 10;
      value = (i >= 10) ? value | ((i / 10) << 4) : value;
      (m_eeprom->get_register(2))->put(value);
      return;
    }

    (m_eeprom->get_register(2))->put(0);
  }

  value = (m_eeprom->get_register(3))->get();	// Get DOW
  value = (value + 1) % 7;
  (m_eeprom->get_register(3))->put(value);
  value = (m_eeprom->get_register(4))->get();	// Get DOM
  dom = (value >> 4) * 10 + (value & 0xf) + 1;
  value = (m_eeprom->get_register(5))->get();	// Get month
  month = (value >> 4) * 10 + (value & 0xf);
  value = (m_eeprom->get_register(6))->get();	// Get year
  year = (value >> 4) * 10 + (value & 0xf) + 2000;

  if ((year % 400) == 0 || ((year % 4) == 0 && (year % 100) != 0)) {
    dim[1] = 29;
  }

  if (dom <= dim[month - 1]) {
    value = dom % 10;
    value = (dom >= 10) ? value | ((dom / 10) << 4) : value;
    (m_eeprom->get_register(4))->put(value);
    return;
  }

  (m_eeprom->get_register(4))->put(1);

  if (++month <= 12) {
    value = month % 10;
    value = (month >= 10) ? value | ((month / 10) << 4) : value;
    (m_eeprom->get_register(5))->put(value);
    return;
  }

  (m_eeprom->get_register(5))->put(1);
  i = ++year % 100;
  value = i % 10;
  value = (i >= 10) ? value | ((i / 10) << 4) : value;
  (m_eeprom->get_register(6))->put(value);
  return;
}


void ds1307::callback()
{
  uint64_t now = get_cycles().get();
  Dprintf(("ds1307 now=%ld tick= %ld sqw= %ld\n",
           (long)now, (long)next_clock_tick, (long)next_sqw_edge));

  if (now == next_clock_tick) {
    incrementRTC();
    next_clock_tick = get_cycles().get() + get_cycles().instruction_cps();
    get_cycles().set_break(next_clock_tick, this);
  }

  if (now == next_sqw_edge) {
    long diff;
    out = !out;
    next_sqw_edge = get_cycles().get() + sqw_interval;
    // syncronize with clock ticks
    diff = labs((long)(next_sqw_edge - next_clock_tick));

    if (diff < (long)sqw_interval / 2) {
      if (!out) {
        fprintf(stderr, "DS1307 SQW phase issue\n");
      }

      next_sqw_edge = next_clock_tick;
    }

    m_sqw->setDrivingState(out);
    get_cycles().set_break(next_sqw_edge, this);
  }
}


} // end of namespace DS1307_Modules
