/*
   Copyright (C) 1998-2002 T. Scott Dattalo
   Copyright (C) 2006	   Roy Rankin

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

#ifndef SRC_P16F8X_H_
#define SRC_P16F8X_H_

#include <assert.h>

#include "a2dconverter.h"
#include "comparator.h"
#include "eeprom.h"
#include "p16x6x.h"
#include "pic-processor.h"
#include "pir.h"
#include "registers.h"
#include "uart.h"

#include "14bit-registers.h"

class Processor;


/***************************************************************************
 *
 * Include file for:  P16F87, P16F88
 *
 *
 * The F8x devices are similar to 16F62x
 *
 *
 ***************************************************************************/

class P16F8x : public P16X6X_processor {
public:
  P16F8x(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F8x();

  void set_out_of_range_pm(unsigned int address, unsigned int value) override;

  PROCESSOR_TYPE isa() override
  {
    return _P16F87_;
  }
  void create_symbols() override;
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }

  unsigned int program_memory_size() const override
  {
    return 0;
  }

  void create_sfr_map() override;

  // The f628 (at least) I/O pins depend on the Fosc Configuration bits.
  bool set_config_word(unsigned int address, unsigned int cfg_word) override;

  virtual void create(int eesize);
  virtual void create_iopin_map();
  void create_config_memory() override;
  void enter_sleep() override;
  void exit_sleep() override;

  void set_eeprom(EEPROM *) override
  {
    // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
    assert(0);
  }

  virtual void set_eeprom_wide(EEPROM_WIDE *ep)
  {
    eeprom = ep;
  }
  EEPROM_WIDE *get_eeprom() override
  {
    return (EEPROM_WIDE *)eeprom;
  }

  PIR *get_pir1() override
  {
    return pir1;
  }
  PIR *get_pir2() override
  {
    return pir2;
  }
  PIR_SET *get_pir_set() override
  {
    return &pir_set_2_def;
  }

  PIR1v2 *pir1_2_reg;
  PIR2v2 *pir2_2_reg;
  PIR_SET_2 pir_set_2_def;

  WDTCON       wdtcon;
  OSCCON_1     *osccon;
  OSCTUNE6     osctune;          // with 6-bit trim, no PLLEN

  USART_MODULE usart;
  ComparatorModule comparator;

protected:
  using P16X6X_processor::create;
};


class P16F81x : public P16X6X_processor {
public:
  P16F81x(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F81x();

  void set_out_of_range_pm(unsigned int address, unsigned int value) override;

  PROCESSOR_TYPE isa() override
  {
    return _P16F818_;
  }
  void create_symbols() override;
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }

  unsigned int program_memory_size() const override
  {
    return 0;
  }

  void create_sfr_map() override;

  // The f628 (at least) I/O pins depend on the Fosc Configuration bits.
  bool set_config_word(unsigned int address, unsigned int cfg_word) override;
  void create_config_memory() override;

  virtual void create(int eesize);
  virtual void create_iopin_map();

  void set_eeprom(EEPROM *) override
  {
    // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
    assert(0);
  }

  virtual void set_eeprom_wide(EEPROM_WIDE *ep)
  {
    eeprom = ep;
  }
  EEPROM_WIDE *get_eeprom() override
  {
    return (EEPROM_WIDE *)eeprom;
  }
  PIR *get_pir1() override
  {
    return pir1;
  }
  PIR *get_pir2() override
  {
    return pir2;
  }
  PIR_SET *get_pir_set() override
  {
    return &pir_set_2_def;
  }

  PIR1v2 *pir1_2_reg;
  PIR2v2 *pir2_2_reg;
  PIR_SET_2 pir_set_2_def;

  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adresh;
  sfr_register  adresl;

  OSCCON_1       *osccon;
  OSCTUNE6      osctune;          // with 6-bit trim, no PLLEN

protected:
  using P16X6X_processor::create;
};


class P16F87 : public P16F8x {
public:
  P16F87(const char *_name = nullptr, const char *desc = nullptr);

  PROCESSOR_TYPE isa() override
  {
    return _P16F87_;
  }

  unsigned int program_memory_size() const override
  {
    return 0x1000;
  }

  static Processor *construct(const char *name);
  void create_sfr_map() override;
};


class P16F88 : public P16F87 {
public:
  P16F88(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F88();

  static Processor *construct(const char *name);

  PROCESSOR_TYPE isa() override
  {
    return _P16F88_;
  }

  unsigned int program_memory_size() const override
  {
    return 0x1000;
  }

  void create() override;
  void create_sfr_map() override;

  ANSEL  ansel;
  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adresh;
  sfr_register  adresl;

protected:
  using P16F87::create;
};


class P16F818 : public P16F81x {
public:
  P16F818(const char *_name = nullptr, const char *desc = nullptr);

  static Processor *construct(const char *name);

  PROCESSOR_TYPE isa() override
  {
    return _P16F818_;
  }

  unsigned int program_memory_size() const override
  {
    return 0x400;
  }

  void create() override;
  void create_sfr_map() override;

protected:
  using P16F81x::create;
};


class P16F819 : public P16F81x {
public:
  P16F819(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F819();

  static Processor *construct(const char *name);

  PROCESSOR_TYPE isa() override
  {
    return _P16F819_;
  }

  unsigned int program_memory_size() const override
  {
    return 0x800;
  }

  void create() override;
  void create_sfr_map() override;

protected:
  using P16F81x::create;
};


#endif
