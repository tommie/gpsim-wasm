/*
   Copyright (C) 2006 T. Scott Dattalo
   Copyright (C) 2006 Roy R Rankin

This file is part of the libgpsim_modules library of gpsim

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


#ifndef MODULES_I2C_EEPROM_H_
#define MODULES_I2C_EEPROM_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/modules.h"

class I2C_EE;
class PromAddress;

namespace I2C_EEPROM_Modules {

class I2C_ENABLE;

class I2C_EE_Module : public Module {
public:
  explicit I2C_EE_Module(const char *_name);
  ~I2C_EE_Module();

  static Module *construct_2k(const char *new_name);
  static Module *construct_16k(const char *new_name);
  static Module *construct_256k(const char *new_name);

  I2C_EE *eeprom() { return m_eeprom; }
  const I2C_EE *eeprom() const { return m_eeprom; }

  virtual void create_iopin_map();
  virtual void setEnable(bool bNewState, unsigned int m_bit);

protected:
  I2C_EE *m_eeprom;
  I2C_ENABLE *m_A[3];
  I2C_ENABLE *m_wp;
  unsigned int chip_select;	// Write Protect and A0 - A2 state
};

} // end of namespace I2C_EEEPROM_Modules

#endif // MODULES_I2C_EEPROM_H_
