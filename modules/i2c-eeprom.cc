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


#include <config.h>    // get the definition for HAVE_GUI
#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

class Processor;
#include "../src/i2c-ee.h"
#include "i2c-eeprom.h"
#include "../src/stimuli.h"
#include "../src/packages.h"

#include <algorithm>
#include <string>

namespace I2C_EEPROM_Modules {

class I2C_ENABLE : public IOPIN {
public:
  I2C_ENABLE(const char *name, unsigned int bit, I2C_EE_Module *pParent);

  virtual void setDrivenState(bool);

private:
  I2C_EE_Module *m_pParent;
  unsigned int m_bit;
};


I2C_ENABLE::I2C_ENABLE(const char *name, unsigned int bit,
                       I2C_EE_Module *pParent)
  : IOPIN(name), m_pParent(pParent), m_bit(bit)
{
}


void I2C_ENABLE::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);

  if (m_pParent) {
    m_pParent->setEnable(bNewState, m_bit);
  }
}


I2C_EE_Module::I2C_EE_Module(const char *_name)
  : Module(_name, "EEProm"),
    m_eeprom(nullptr), m_wp(nullptr), chip_select(0), att_eeprom(nullptr)
{
  std::fill_n(m_A, 3, nullptr);
}


I2C_EE_Module::~I2C_EE_Module()
{
  removeSymbol(m_wp);
  removeSymbol(m_A[0]);
  removeSymbol(m_A[1]);
  removeSymbol(m_A[2]);
  removeSymbol((IOPIN *)(m_eeprom->sda));
  removeSymbol((IOPIN *)(m_eeprom->scl));
  m_eeprom->sda = nullptr;
  m_eeprom->scl = nullptr;
  delete att_eeprom;
  delete m_eeprom;
}


Module *I2C_EE_Module::construct_2k(const char *_new_name)
{
// FIXME: att_name is unused, unlike in the other constructors

  std::string att_name = _new_name;
  I2C_EE_Module *pEE = new I2C_EE_Module(_new_name);
  // I2C_EE size in bytes prom size in bits
  (pEE->m_eeprom) = new I2C_EE((Processor *)pEE, 256, 16, 1, 0xe, 0, 0);
  pEE->create_iopin_map();
  att_name += ".eeprom";
  pEE->att_eeprom = new PromAddress(pEE->m_eeprom, "eeprom", "Address I2C_EE");
  pEE->addSymbol(pEE->att_eeprom);
  return pEE;
}


Module *I2C_EE_Module::construct_16k(const char *_new_name)
{
  std::string att_name = _new_name;
  I2C_EE_Module *pEE = new I2C_EE_Module(_new_name);
  // I2C_EE size in bytes prom size in bits
  (pEE->m_eeprom) = new I2C_EE((Processor *)pEE, 2048, 16, 1, 0, 0xe, 1);
  pEE->create_iopin_map();
  att_name += ".eeprom";
  pEE->att_eeprom = new PromAddress(pEE->m_eeprom, att_name.c_str(), "Address I2C_EE");
  pEE->addSymbol(pEE->att_eeprom);
  return pEE;
}


Module *I2C_EE_Module::construct_256k(const char *_new_name)
{
  std::string att_name = _new_name;
  I2C_EE_Module *pEE = new I2C_EE_Module(_new_name);
  // I2C_EE size in bytes prom size in bits
  (pEE->m_eeprom) = new I2C_EE((Processor *)pEE, 32768, 64, 2, 0xe, 0, 0);
  pEE->create_iopin_map();
  att_name += ".eeprom";
  pEE->att_eeprom = new PromAddress(pEE->m_eeprom, att_name.c_str(), "Address I2C_EE");
  pEE->addSymbol(pEE->att_eeprom);
  return pEE;
}


void I2C_EE_Module::create_iopin_map()
{
  m_wp  =  new I2C_ENABLE("WP", 0, this);
  addSymbol(m_wp);
  m_A[0] = new I2C_ENABLE("A0", 1, this);
  addSymbol(m_A[0]);
  m_A[1] = new I2C_ENABLE("A1", 2, this);
  addSymbol(m_A[1]);
  m_A[2] = new I2C_ENABLE("A2", 3, this);
  addSymbol(m_A[2]);
  addSymbol((IOPIN *)(m_eeprom->sda));
  addSymbol((IOPIN *)(m_eeprom->scl));
  package = new Package(8);
  package->assign_pin(1, m_A[0]);
  package->assign_pin(2, m_A[1]);
  package->assign_pin(3, m_A[2]);
  package->assign_pin(5, (IOPIN *)(m_eeprom->sda));
  package->assign_pin(6, (IOPIN *)(m_eeprom->scl));
  package->assign_pin(7, m_wp);
}


// WP or A0-A2 has changed
void I2C_EE_Module::setEnable(bool NewState, unsigned int bit)
{
  if (NewState) {
    chip_select |= 1 << bit;

  } else {
    chip_select &= ~(1 << bit);
  }

  m_eeprom->set_chipselect(chip_select);
}

} // end of namespace I2C_EEEPROM_Modules
