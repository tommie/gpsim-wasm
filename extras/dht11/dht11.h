/*
   Copyright (C) 2014 Dr. David Alan Gilbert (dave@treblig.org)

This file is part of the libgpsim_modules library of gpsim
and was originally based on the usart module

The dht11 is a cheap temperature and humidity sensor, this
model is based vaguely on the datasheet at:
http://akizukidenshi.com/download/ds/aosong/DHT11.pdf
It expects the data pin to have a resistor pulling it up.

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


#ifndef EXTRAS_dht11_MODULE_H_
#define EXTRAS_dht11_MODULE_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <config.h>

#include <glib.h>

#include <src/modules.h>
#include <src/trigger.h>

class dht11Module : public TriggerObject, public Module
{
public:
  explicit dht11Module(const char *new_name);
  ~dht11Module();

  void CreateGraphics();

  // Inheritances from the Package class
  virtual void create_iopin_map();

  const virtual char *type() { return ("dht11"); }
  static Module *construct(const char *new_name);

  virtual void callback();


private:
  class    IntegerAttribute;
  guint8   state; /* Idle = 0 */
  guint8   byte;  /* current byte being transmitted */
  guint8   checksum;
  IntegerAttribute* m_tempAttribute;
  IntegerAttribute* m_humidityAttribute;
  class Pin;

  Pin* m_pin;
  void start(); /* Callback from Pin to indicate start sequence received */
  void set_state_callback(guint8 new_state, double delay, bool level);
  void callback_end();
  void callback_intro();
};

#endif //  EXTRAS_dht11_MODULE_H_
