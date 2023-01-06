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


#include "../config.h"

#include "stimuli.h"
#include "packages.h"
#include "ui.h"

#include <algorithm>
#include <iostream>
#include <math.h>

Package::Package()
{
  pins = nullptr;
  m_pinGeometry = nullptr;
}


Package::Package(unsigned int _number_of_pins)
{
  create_pkg(_number_of_pins);
}


Package::~Package()
{
  if (pins) {
    destroy_pin(0);  // delete all of the pins
  }

  delete [] pins;
  delete [] m_pinGeometry;
}


void Package::create_pkg(unsigned int _number_of_pins)
{
  if (number_of_pins) {
    std::cout << "error: Package::create_pkg. Package appears to already exist.\n";
    return;
  }

  number_of_pins = _number_of_pins;

  pins = new IOPIN *[number_of_pins];
  std::fill_n(pins, number_of_pins, nullptr);

  m_pinGeometry =  new PinGeometry[number_of_pins];

  unsigned int pins_per_side = number_of_pins / 2;

  if (number_of_pins & 1) { // If odd number of pins
    pins_per_side++;
  }

  for (unsigned int i = 0; i < number_of_pins; i++) {
    // Positions for DIL package
    if (i < pins_per_side) {
      m_pinGeometry[i].pin_position = (i) / ((float)(pins_per_side - 0.9999));

    } else {
      m_pinGeometry[i].pin_position = (i - pins_per_side) / ((float)(pins_per_side - 0.9999)) + (float)2.0;
    }
  }
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Package::pin_existance(unsigned int pin_number)
{
  if (!number_of_pins) {
    std::cout << "error: Package::assign_pin. No package.\n";
    return E_NO_PACKAGE;
  }

  if ((pin_number > number_of_pins) || (pin_number == 0)) {
    std::cout << "error: Package::assign_pin. Pin number is out of range.\n"
                 "Max pins " << number_of_pins << ". Trying to add " << pin_number << ".\n";
    return E_PIN_OUT_OF_RANGE;
  }

  if (pins[pin_number - 1]) {
    return E_PIN_EXISTS;
  }

  return E_NO_PIN;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
IOPIN *Package::get_pin(unsigned int pin_number)
{
  if (E_PIN_EXISTS == pin_existance(pin_number)) {
    return pins[pin_number - 1];

  } else {
    return nullptr;
  }
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
float Package::get_pin_position(unsigned int pin_number)
{
  return (bIsValidPinNumber(pin_number) ? m_pinGeometry[pin_number - 1].pin_position : 0.0);
}


void Package::set_pin_position(unsigned int pin_number, float position)
{
  if (bIsValidPinNumber(pin_number)) {
    m_pinGeometry[pin_number - 1].bNew = false;
    m_pinGeometry[pin_number - 1].pin_position = position;
  }
}


void Package::setPinGeometry(unsigned int pin_number, float x, float y,
                             int orientation, bool bShowName)
{
  if (bIsValidPinNumber(pin_number)) {
    m_pinGeometry[pin_number - 1].bNew = true;
    m_pinGeometry[pin_number - 1].m_x = x;
    m_pinGeometry[pin_number - 1].m_y = y;
    m_pinGeometry[pin_number - 1].m_orientation = orientation;
    m_pinGeometry[pin_number - 1].m_bShowPinname = bShowName;
  }
}


PinGeometry *Package::getPinGeometry(unsigned int pin_number)
{
  static PinGeometry BAD_PIN;

  if (bIsValidPinNumber(pin_number)) {
    m_pinGeometry[pin_number - 1].convertToNew();
    return &m_pinGeometry[pin_number - 1];
  }

  return &BAD_PIN;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Package::assign_pin(unsigned int pin_number, IOPIN *pin, bool warn)
{
  switch (pin_existance(pin_number)) {
  case E_PIN_EXISTS:
    if (pins[pin_number - 1] && warn) {
      std::cout << "warning: Package::assign_pin. Pin number " << pin_number << " already exists.\n";
    }

  case E_NO_PIN:
    pins[pin_number - 1] = pin;

    if ((bool)verbose && pin) {
      std::cout << "assigned pin " << pin->name() << " to package pin number " << std::dec << pin_number << '\n';
    }

    break;
  }
}


//-------------------------------------------------------------------
void Package::destroy_pin(unsigned int pin_number, IOPIN *)
{
  if (pin_number) {
    if (pin_number <= number_of_pins) {
      IOPIN *pPin = pins[pin_number - 1];

      delete pPin;

      pins[pin_number - 1] = nullptr;
    }

  } else {
    // Delete all pins
    for (pin_number = 1; pin_number <= number_of_pins; pin_number++) {
      destroy_pin(pin_number);
    }

    number_of_pins = 0;
  }
}


void Package::create_iopin_map()
{
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------

std::string &Package::get_pin_name(unsigned int pin_number)
{
  static std::string invalid;

  if (pin_existance(pin_number) == E_PIN_EXISTS) {
    return pins[pin_number - 1]->name();

  } else {
    return invalid;  //FIXME
  }
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Package::get_pin_state(unsigned int pin_number)
{
  if (pin_existance(pin_number) == E_PIN_EXISTS) {
    return pins[pin_number - 1]->getDrivingState();

  } else {
    return 0;
  }
}


//------------------------------------------------------------------------
void PinGeometry::convertToNew()
{
  if (!bNew) {
    m_orientation = (int) floor(pin_position);

    if (m_orientation) {
      m_x = 0.0;
      m_y = pin_position;

    } else {
      m_x = pin_position - m_orientation;
      m_y = 0.0;
    }

    m_bShowPinname = true;
  }
}

