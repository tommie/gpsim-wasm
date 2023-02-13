/*
   Copyright (C) 1998,1999 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

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


#include <config.h>

#include "stimuli.h"
#include "packages.h"
#include "ui.h"

#include <algorithm>
#include <iostream>
#include <math.h>

Package::Package(unsigned int number_of_pins)
  : pins(number_of_pins)
{
}


void Package::assign_pin(unsigned int pin_number, IOPIN *pin)
{
  // If the pin already existed, the owner takes ownership.
  //
  // TODO: this is unclear ownership. Perhaps the parent Module should
  // always retain ownership, and Package just borrows.
  pins[pin_number - 1].release();
  pins[pin_number - 1] = std::unique_ptr<IOPIN>(pin);
}


IOPIN *Package::get_pin(unsigned int pin_number) const
{
  return (pin_exists(pin_number) ? pins[pin_number - 1].get() : nullptr);
}


std::string Package::get_pin_name(unsigned int pin_number) const
{
  auto *pin = get_pin(pin_number);
  return (pin ? pin->name() : "NC");
}


int Package::get_pin_state(unsigned int pin_number) const
{
  auto *pin = get_pin(pin_number);
  return (pin ? pin->getDrivingState() : 0);
}
