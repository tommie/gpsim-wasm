/*
  Copyright (C) 2014 Dr. David Alan Gilbert (dave@treblig.org)

This file is part of the libgpsim_modules library of gpsim
and was originally based on the usart module

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

/*

  dht11.cc

  The dht11 is a cheap temperature and humidity sensor, this
  model is based vaguely on the datasheet at:
  http://akizukidenshi.com/download/ds/aosong/DHT11.pdf
  It expects the data pin to have a resistor pulling it up.

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <errno.h>
#include <stdlib.h>
#include <string>

#include "dht11.h"


#include <cstdio>
#include <src/gpsim_time.h>
#include <src/interface.h>
#include <src/ioports.h>
#include <src/modules.h>
#include <src/packages.h>
#include <src/pir.h>
#include <src/stimuli.h>
#include <src/symbol.h>
#include <src/trace.h>
#include <src/value.h>

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("module-%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

static const double host_start_sig_len = 0.018; /* 18ms minimum low */

/* High nybble used for major states, bottom nybble for the current bit
 * OR with STATE_TEMP_DATA_LOW for the first (low) half of each bit that's
 * always the same length
 */
enum {
  STATE_IDLE         = 0,
  STATE_INTRO        = 0x10,
  STATE_HUMIDITY_INT = 0x20,
  STATE_HUMIDITY_DEC = 0x30,
  STATE_TEMP_INT     = 0x40,
  STATE_TEMP_DEC     = 0x50,
  STATE_CHECKSUM     = 0x60,
  STATE_END          = 0x70,

  STATE_TEMP_DATA_LOW = 8
};

//--------------------------------------------------------------
class dht11Module::Pin : public IO_open_collector
{
  public:
    explicit Pin(dht11Module* new_parent)
        : IO_open_collector("data")
    {
      parent = new_parent;
      parent->addSymbol(this);
      lastState = true;
      lastLowTransition = 0;
      bDrivingState = true;
      bDrivenState = true;
      update_direction(IO_bi_directional::DIR_OUTPUT,true);

      // Based on the code from modules/i2c.cc
      // Set the pullup resistance to 10k ohms:
      set_Zpullup(10e3);

      update_pullup('1',    // Turn on the pullup resistor.
                    false); // Don't update the node. (none is attached).
    }

    ~Pin() {}

    /* This gets called whenever the pin voltage changes */
    virtual void set_nodeVoltage(double newvolts)
    {
      bool newState = newvolts > 1.5;
      uint64_t now = get_cycles().get();

      Dprintf(("Times: instruction-cps=%lf seconds-per-cycle=%lf\n", get_cycles().instruction_cps(), get_cycles().seconds_per_cycle()));
      Dprintf(("dht11 Pin/set_nodeVoltage %lf / %d at %" PRINTF_GINT64_MODIFIER "d\n", newvolts, newState, now));

      if (lastState != newState) {
        if (lastState && !newState) {
          /* High->Low transition */
          lastLowTransition = now;
        }
        if (!lastState && newState) {
          /* Low->High transition */
          uint64_t delta = now - lastLowTransition;
          double delta_s = delta*get_cycles().seconds_per_cycle();
          Dprintf(("dht11 l->h low period=%" PRINTF_GINT64_MODIFIER "d=%lf s\n", delta, delta_s));

          if (delta_s > host_start_sig_len) {
            parent->start();
          }
        }

        lastState = newState;
      }
    }

    void setDrivingState(bool new_state) {
      Dprintf(("new_state=%d\n",new_state));
      bDrivingState = new_state;
      //bDrivenState = new_state;
      //
      if (snode)
        snode->update();
    }

  private:
    dht11Module* parent;
    uint64_t lastLowTransition;
    bool lastState;
};

class dht11Module::IntegerAttribute : public Integer
{
public:
  IntegerAttribute(const char *_name,  int64_t newValue = 0,
                   const char *desc = nullptr)
    : Integer(_name, newValue, desc)
  {
  }
};

//--------------------------------------------------------------
// Handler for the intro states
// Initially (0) we're just after the start signal and we're high
//                 so then we pull low for 80us
//            1  we've been low, now pull high
void dht11Module::callback_intro()
{
  switch (state & 0xf) {
    case 0:
      set_state_callback(STATE_INTRO + 1, 80.0, 0);
      break;

    case 1:
      set_state_callback(STATE_HUMIDITY_INT | STATE_TEMP_DATA_LOW, 80.0, 1);

    default:
      Dprintf(("Bad state\n"));
      break;
  }
}

//--------------------------------------------------------------
// Handler for the end states - 50us low pulse at the end
//   0 - Where we pull it low
//   1 - We're done, release and no more callbacks
void dht11Module::callback_end()
{
  switch (state & 0xf) {
    case 0:
      set_state_callback(STATE_END + 1, 50.0, 0);
      break;

    case 1:
      state = STATE_IDLE;
      m_pin->setDrivingState(true);
      Dprintf(("All done\n"));
      break;

    default:
      Dprintf(("Bad state\n"));
      break;
  }
}

//--------------------------------------------------------------
// Gets called on 'set_break' timeouts
void dht11Module::callback()
{
  Dprintf(("at %" PRINTF_GINT64_MODIFIER "d in 0x%x\n", get_cycles().get(), state));

  /* Handle the intro sequence separately */
  if ((state & 0xf0) == STATE_INTRO) {
    callback_intro();
    return;
  }
  if ((state & 0xf0) == STATE_END) {
    callback_end();
    return;
  }

  /* In that case it's data */

  if (state & STATE_TEMP_DATA_LOW) {
    /* The first half of each bit is always the same, 50us low */
    set_state_callback(state & ~STATE_TEMP_DATA_LOW, 50.0, 0);
    return;
  } else {
    /* The 'high' part of each bit - the length determines the data */
    uint8_t next_state = (state+1) | STATE_TEMP_DATA_LOW;
    int64_t tmp;
    bool bit_value;


    if ((state & 7) == 0) {
      /* First bit of the byte - figure out what to send */
      switch (state & 0xf0) {
        case STATE_HUMIDITY_INT:
          checksum = 0; /* 1st Byte */
          m_humidityAttribute->get_as(tmp); /* e.g. 1234 */
          tmp /= 100; /* e.g. 12 */
          byte = tmp & 255;
          break;

        case STATE_HUMIDITY_DEC:
          byte = 0; /* DHT11 doesn't really do dec */
          break;

        case STATE_TEMP_INT:
          m_tempAttribute->get_as(tmp); /* e.g. 1234 */
          tmp /= 100; /* e.g. 12 */
          byte = tmp & 255;
          break;

        case STATE_TEMP_DEC:
          m_tempAttribute->get_as(tmp); /* e.g. 1234 */
          byte = 0; /* DHT11 doesn't really do dec */
          break;

        case STATE_CHECKSUM:
          byte = checksum;
          break;
      }
      checksum += byte;
    }

    bit_value = (byte & 0x80) != 0;
    byte = (byte<<1) & 0xff;
    if ((state & 7) == 7) {
      /* Last bit of the byte - figure out what happens next */
      next_state = state & 0xf0;
      next_state += 0x10;
      if (next_state != STATE_END) {
        next_state |= STATE_TEMP_DATA_LOW;
      }
    }
    set_state_callback(next_state, bit_value?70.0:27.0, 1);

  }
}

//--------------------------------------------------------------
// Set the state, set the pin and register a callback for sometime in the future
void dht11Module::set_state_callback(uint8_t new_state, double delay_us, bool level)
{
  uint64_t now = get_cycles().get();
  uint64_t future_time;
  future_time = now + 1 + ((delay_us / 1000000.0) * get_cycles().instruction_cps());
  Dprintf(("State: %d->%d wait %lf s pin->%d now=%" PRINTF_GINT64_MODIFIER "d future=%" PRINTF_GINT64_MODIFIER "d\n", state, new_state, delay_us, level, now, future_time));

  state = new_state;

  m_pin->setDrivingState(level);
  get_cycles().set_break(future_time, this);
}

//--------------------------------------------------------------
// Called by the pin to indicate reception of start signal
void dht11Module::start()
{
  if (state != STATE_IDLE) {
    /* I don't see anything to indicate what should happen here */
    Dprintf(("Start detected but not idle - ignoring (state=%d)\n", state));
    return;
  }

  Dprintf(("Start received!\n"));
  /* It takes some time for the dht to notice the START, so this is high for a while */
  set_state_callback(STATE_INTRO, 30.0, 1);
}

//--------------------------------------------------------------

Module * dht11Module::construct(const char *_new_name)
{

  Dprintf(("dht11 construct\n"));

  dht11Module *um = new dht11Module( (_new_name ?_new_name:"dht11"));
  um->create_iopin_map();
  um->state = STATE_IDLE;

  return um;

}

dht11Module::dht11Module(const char *_name)
    : TriggerObject(), Module(_name, "dht11"),
      m_pin(nullptr)
{

  /* DHT11's actually never do the decimal part of either the humidity or temperature, even
   * though the protocol allows it; I've seen reference to a DHT22 that does, but don't
   * have one. */
  m_tempAttribute = new IntegerAttribute("temperature", 1300, "Temperature in hundredths of degree C");
  addSymbol(m_tempAttribute);
  m_humidityAttribute = new IntegerAttribute("humidity", 4200, "Humidity in hundredths of percent");
  addSymbol(m_humidityAttribute);

  assert(m_tempAttribute);
  assert(m_humidityAttribute);
}

dht11Module::~dht11Module()
{
  removeSymbol(m_tempAttribute);
  removeSymbol(m_humidityAttribute);
  delete m_tempAttribute;
  delete m_humidityAttribute;
}

void dht11Module::create_iopin_map()
{
  create_pkg(1);

  m_pin = new dht11Module::Pin(this);
  package->setPinGeometry(0, 0.5, 0, 0 /* orientiation ? */, false /* Don't show name */);
  assign_pin(1, m_pin);

}
