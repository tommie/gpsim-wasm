/*
   Copyright (C) 2000,2001 T. Scott Dattalo, Daniel Schudel, Robert Pearce

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


//
// p16x5x
//
//  This file supports:
//    P16C54
//    P16C55
//    P16C56

#ifndef SRC_P16X5X_H_
#define SRC_P16X5X_H_

#include "pic-processor.h"
#include "packages.h"
#include "12bit-processors.h"

class PicPortRegister;
class PicTrisRegister;
class Processor;


class P16C54 : public  _12bit_processor {
public:
  P16C54(const char *_name = nullptr, const char *desc = nullptr);
  virtual ~P16C54();

  PROCESSOR_TYPE isa() override
  {
    return _P16C54_;
  }
  void create_symbols() override;

  unsigned int program_memory_size() const override
  {
    return 0x200;
  }
  unsigned int register_memory_size() const override
  {
    return 0x20;
  }
  unsigned int config_word_address() const override
  {
    return 0xFFF;
  }

  void create_sfr_map() override;

   void option_new_bits_6_7(unsigned int /* bits */ ) override {}

  void create() override;
  virtual void create_iopin_map();

  static Processor *construct(const char *name);
  void tris_instruction(unsigned int tris_register) override;

  unsigned int fsr_valid_bits() override
  {
    return 0x1f;  // Only 32 register addresses
  }

  unsigned int fsr_register_page_bits() override
  {
    return 0;     // Only one register page.
  }

  PicPortRegister  *m_porta;
  PicTrisRegister  *m_trisa;

  PicPortRegister  *m_portb;
  PicTrisRegister  *m_trisb;

#ifdef USE_PIN_MODULE_FOR_TOCKI
  PinModule    *m_tocki;
#else
  PicPortRegister  *m_tocki;
  PicTrisRegister  *m_trist0;
#endif
};

class P16C55 : public  P16C54 {
public:
  P16C55(const char *_name = nullptr, const char *desc = nullptr);
  virtual ~P16C55();

  PROCESSOR_TYPE isa() override
  {
    return _P16C55_;
  }
  void create_symbols() override;

  unsigned int program_memory_size() const override
  {
    return 0x200;
  }
  unsigned int register_memory_size() const override
  {
    return 0x20;
  }
  unsigned int config_word_address() const override
  {
    return 0xFFF;
  }

  void create_sfr_map() override;
  void create() override;
  void create_iopin_map() override;

  static Processor *construct(const char *name);
  void tris_instruction(unsigned int tris_register) override;

  PicPortRegister  *m_portc;
  PicTrisRegister  *m_trisc;
};


class P16C56 : public  P16C54 {
public:
  P16C56(const char *_name = nullptr, const char *desc = nullptr);

  PROCESSOR_TYPE isa() override
  {
    return _P16C56_;
  }

  unsigned int program_memory_size() const override
  {
    return 0x400;
  }
  unsigned int register_memory_size() const override
  {
    return 0x20;
  }
  unsigned int config_word_address() const override
  {
    return 0xFFF;
  }

  static Processor *construct(const char *name);
};

#endif
