/*
   Copyright (C) 2017 Roy R. Rankin

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

#ifndef SRC_P16F91X_H_
#define SRC_P16F91X_H_

#include <assert.h>

#include "14bit-processors.h"
#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "a2dconverter.h"
#include "comparator.h"
#include "eeprom.h"
#include "intcon.h"
#include "lcd_module.h"
#include "pic-processor.h"
#include "pie.h"
#include "pir.h"
#include "registers.h"
#include "ssp.h"
#include "uart.h"

class PicPortBRegister;
class PicPortRegister;
class PicTrisRegister;
class Processor;

class P16F91X : public  _14bit_processor {
public:
  P16F91X(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F91X();

  virtual PIR *get_pir1()
  {
    return pir1_2_reg;
  }
  virtual PIR_SET *get_pir_set()
  {
    return &pir_set_2_def;
  }
  virtual PIR *get_pir2()
  {
    return pir2_2_reg;
  }

  void create_symbols() override;
  void create_sfr_map() override;
  void option_new_bits_6_7(unsigned int bits) override;
  virtual void set_eeprom_wide(EEPROM_WIDE *ep)
  {
    eeprom = ep;
  }
  EEPROM_WIDE *get_eeprom() override
  {
    return (EEPROM_WIDE *)eeprom;
  }
  void update_vdd() override;
  bool set_config_word(unsigned int address, unsigned int cfg_word) override;
  void enter_sleep() override;
  void exit_sleep() override;

  PIR2v2 *pir2_2_reg;

  INTCON_14_PIR    intcon_reg;
  T1CON   t1con;
  PIR    *pir1;
  PIE     pie1;
  PIR    *pir2;
  PIE     pie2;
  T2CON   t2con;
  PR2     pr2;
  TMR2    tmr2;
  TMRL    tmr1l;
  TMRH    tmr1h;
  CCPCON  ccp1con;
  CCPRL   ccpr1l;
  CCPRH   ccpr1h;
  CCPCON  ccp2con;
  CCPRL   ccpr2l;
  CCPRH   ccpr2h;
  PCON    pcon;
  LVDCON_14  lvdcon;
  SSP_MODULE   ssp;
  PIR1v2 *pir1_2_reg;
  PIR_SET_2 pir_set_2_def;

  ADCON0_91X adcon0;
  ADCON1_16F adcon1;
  sfr_register  adres;
  sfr_register  adresl;
  ANSEL_P  ansel;

  USART_MODULE usart;

  LCD_MODULE  lcd_module;

  WDTCON	wdtcon;
  OSCCON  	*osccon;
  OSCTUNE5	osctune;           // with 5-bit trim, no PLLEN

  ComparatorModule comparator;

  PicPortRegister  *m_porta;
  PicTrisRegister  *m_trisa;

  PicPortBRegister *m_portb;
  PicTrisRegister  *m_trisb;
  WPU              *m_wpub;
  IOC              *m_iocb;

  PicPortRegister  *m_portc;
  PicTrisRegister  *m_trisc;

  PicPortRegister  *m_porte;
  PicTrisRegister  *m_trise;
};


class P16F91X_40 : public P16F91X {
public:
  P16F91X_40(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F91X_40();

  void set_out_of_range_pm(unsigned int address, unsigned int value) override;

  void create_symbols() override;
  void create_sfr_map() override;
  virtual void create_iopin_map();
  void create() override;
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }

  void set_eeprom(EEPROM *) override
  {
    // use set_eeprom_wide as P16F917 expect a wide EEPROM
    assert(0);
  }

  PicPortRegister  *m_portd;
  PicTrisRegister  *m_trisd;
};


class P16F91X_28 : public P16F91X {
public:
  P16F91X_28(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F91X_28();

  void set_out_of_range_pm(unsigned int address, unsigned int value) override;

  void create_symbols() override;
  void create_sfr_map() override;
  virtual void create_iopin_map();
  void create() override;
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }
  void set_eeprom(EEPROM *) override
  {
    // use set_eeprom_wide as P16F917 expect a wide EEPROM
    assert(0);
  }
};


class P16F917 : public P16F91X_40 {
public:
  P16F917(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F917();

  PROCESSOR_TYPE isa() override
  {
    return _P16F917_;
  }
  unsigned int program_memory_size() const override
  {
    return 8192;
  }
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }

  static Processor *construct(const char *name);
  void create() override;
  void create_sfr_map() override;
  void create_symbols() override;
};


class P16F916 : public P16F91X_28 {
public:
  P16F916(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F916();

  PROCESSOR_TYPE isa() override
  {
    return _P16F916_;
  }
  unsigned int program_memory_size() const override
  {
    return 8192;
  }
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }

  static Processor *construct(const char *name);
  void create() override;
  void create_sfr_map() override;
};


class P16F914 : public P16F91X_40 {
public:
  P16F914(const char *_name = nullptr, const char *desc = nullptr)
    : P16F91X_40(_name, desc) {}

  PROCESSOR_TYPE isa() override
  {
    return _P16F914_;
  }
  unsigned int program_memory_size() const override
  {
    return 4096;
  }
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }

  static Processor *construct(const char *name);
  void create() override;
  //RRRvirtual void create_symbols();
};


class P16F913 : public P16F91X_28 {
public:
  P16F913(const char *_name = nullptr, const char *desc = nullptr)
    : P16F91X_28(_name, desc) {}

  PROCESSOR_TYPE isa() override
  {
    return _P16F913_;
  }
  unsigned int program_memory_size() const override
  {
    return 4096;
  }
  unsigned int register_memory_size() const override
  {
    return 0x200;
  }

  static Processor *construct(const char *name);
  void create() override;
};


#endif
