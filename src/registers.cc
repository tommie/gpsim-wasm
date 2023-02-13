/*
   Copyright (C) 1998-2003 Scott Dattalo

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


#include <stdlib.h>

#include <cstring>
#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>

#include "errors.h"
#include "modules.h"
#include "processor.h"
#include "registers.h"
#include "trace.h"

unsigned int count_bits(unsigned int ui)
{
  unsigned int bits = 0;

  while (ui) {
    ui &= (ui - 1);
    bits++;
  }

  return bits;
}


//========================================================================
// toString
//
// Convert a RegisterValue type to a string.
//
// A RegisterValue type allows the bits of a register to take on three
// values: High, Low, or undefined. If all of the bits are defined,
// then this routine will convert register value to a hexadecimal string.
// Any undefined bits within a nibble will cause the associated nibble to
// be undefined and will get converted to a question mark.
//

char * RegisterValue::toString(char *str, int len, int regsize) const
{
  if (str && len) {
    RegisterValue rv = *this;
    char hex2ascii[] = "0123456789ABCDEF";
    char undefNibble = '?';
    int i;
    int m = regsize * 2 + 1;

    if (len < m) {
      m = len;
    }

    m--;

    for (i = 0; i < m; i++) {
      if (rv.init & 0x0f) {
        str[m - i - 1] = undefNibble;

      } else {
        str[m - i - 1] = hex2ascii[rv.data & 0x0f];
      }

      rv.init >>= 4;
      rv.data >>= 4;
    }

    str[m] = '\0';
  }

  return str;
}


//--------------------------------------------------
// Member functions for the file_register base class
//--------------------------------------------------
//
// For now, initialize the register with valid data and set that data equal to 0.
// Eventually, the initial value will be marked as 'uninitialized.

Register::Register(Module *_cpu, const char *pName, const char *pDesc, unsigned int address)
  : Value(pName, pDesc, _cpu),
    value(RegisterValue(0, 0)),
    address(address),
    por_value(RegisterValue(0, 0))
{
}


Register::~Register()
{
  if (get_module()) {
    //cout << "Removing register from ST:" << name_str <<  " addr "<< this << endl;
    get_module()->removeSymbol(this);
  }
}


//------------------------------------------------------------
// get_as()
//
//  Return the contents of the file register.
// (note - breakpoints on file register reads
//  are not checked here. Instead, a breakpoint
//  object replaces those instances of file
//  registers for which we wish to monitor.
//  So a file_register::get call will invoke
//  the breakpoint::get member function. Depending
//  on the type of break point, this get() may
//  or may not get called).

unsigned int Register::get()
{
  emplace_value_trace<trace::ReadRegisterEntry>();
  return value.get();
}


//------------------------------------------------------------
// put()
//
//  Update the contents of the register.
//  See the comment above in file_register::get()
//  with respect to break points
//

void Register::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  value.put(new_value);
}


//-----------------------------------------------------------
//  void Register::put_value(unsigned int new_value)
//
//  put_value is used by the gui to change the contents of
// file registers. We could've let the gui use the normal
// 'put' member function to change the contents, however
// there are instances where 'put' has a cascading affect.
// For example, changing the value of an i/o port's tris
// could cause i/o pins to change states. In these cases,
// we'd like the gui to be notified of all of the cascaded
// changes. So rather than burden the real-time simulation
// with notifying the gui, I decided to create the 'put_value'
// function instead.
//   Since this is a virtual function, derived classes have
// the option to override the default behavior.
//
// inputs:
//   unsigned int new_value - The new value that's to be
//                            written to this register
// returns:
//   nothing
//
//-----------------------------------------------------------

void Register::put_value(unsigned int new_value)
{
  // go ahead and use the regular put to write the data.
  // note that this is a 'virtual' function. Consequently,
  // all objects derived from a file_register should
  // automagically be correctly updated.
  value.put(new_value);
}


///
/// New accessor functions
//////////////////////////////////////////////////////////////

unsigned int Register::register_size() const
{
  Processor *pProc = static_cast<Processor*>(get_module());
  return pProc ? pProc->register_size() : 1;
}


//------------------------------------------------------------

std::string Register::toString()
{
  char buf[64];
  return getRV_notrace().toString(buf, sizeof(buf), register_size() * 2);
}


//-----------------------------------------------------------

void Register::new_name(const char *s)
{
  if (s) {
    std::string str = s;
    new_name(str);
  }
}


void Register::new_name(std::string &new_name)
{
  if (name_str != new_name) {
    if (name_str.empty()) {
      name_str = new_name;
      return;
    }

    name_str = new_name;

    if (get_module()) {
      gpsimObject::new_name(new_name);
      get_module()->addSymbol(this, &new_name);
    }
  }
}


//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
sfr_register::sfr_register(Module *pCpu, const char *pName, const char *pDesc)
  : Register(pCpu, pName, pDesc), wdtr_value(0, 0xff)
{
}


void sfr_register::reset(RESET_TYPE r)
{
  switch (r) {
  case POR_RESET:
    putRV(por_value);
    break;

  default:

    // Most registers simply retain their value across WDT resets.
    if (wdtr_value.initialized()) {
      putRV(wdtr_value);
    }

    break;
  }
}


//--------------------------------------------------
//--------------------------------------------------

//--------------------------------------------------
// member functions for the InvalidRegister class
//--------------------------------------------------
void InvalidRegister::put(unsigned int new_value)
{
  std::cout << "attempt write to invalid file register\n";

  if (address != AN_INVALID_ADDRESS) {
    std::cout << "    address 0x" << std::hex << address << ',';
  }

  std::cout << "   value 0x" << std::hex << new_value << '\n';

  emplace_value_trace<trace::WriteRegisterEntry>();
}


unsigned int InvalidRegister::get()
{
  std::cout << "attempt read from invalid file register\n";

  if (address != AN_INVALID_ADDRESS) {
    std::cout << "    address 0x" << std::hex << address << '\n';
  }

  emplace_value_trace<trace::ReadRegisterEntry>();

  return 0;
}
