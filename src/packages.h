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

#ifndef SRC_PACKAGES_H_
#define SRC_PACKAGES_H_

#include <memory>
#include <string>

#include "stimuli.h"


class Package {
public:
  explicit Package(unsigned int number_of_pins);

  void assign_pin(unsigned int pin_number, IOPIN *pin);

  int get_pin_count() const { return pins.size(); }
  std::string get_pin_name(unsigned int pin_number) const;
  int get_pin_state(unsigned int pin_number) const;
  IOPIN *get_pin(unsigned int pin_number) const;

protected:
  bool pin_exists(unsigned int pin_number) const
  {
    return pin_number > 0 && pin_number <= pins.size() && pins[pin_number - 1];
  }

private:
  /* An vector containing all of the package's pins. The index into
   * the array is the package's pin # minus one. If pins[i] is NULL
   * then gpsim does not provide any resources for simulating the pin.
   */
  std::vector<std::unique_ptr<IOPIN>> pins;
};

#endif // SRC_PACKAGES_H_
