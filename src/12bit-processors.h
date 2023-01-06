/*
   Copyright (C) 1998 T. Scott Dattalo

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

#include "pic-processor.h"

#ifndef SRC_12_BIT_PROCESSORS_H_
#define SRC_12_BIT_PROCESSORS_H_

#include "14bit-registers.h"
#include "gpsim_classes.h"

#include "registers.h"

// forward references
class OptionTraceType;
class OPTION_REG;
class instruction;

extern instruction *disasm12(pic_processor *cpu, unsigned int address, unsigned int inst);


class _12bit_processor : public pic_processor
{
public:

#define WDTE                     4

enum _12BIT_DEFINITIONS
{
  PA0 = 1 << 5,     /* Program page preselect bits (in status) */
  PA1 = 1 << 6,
  PA2 = 1 << 7,

  RP0 = 1 << 5,     /* Register page select bits (in fsr) */
  RP1 = 1 << 6
};

  unsigned int pa_bits;   /* a CPU dependent bit-mask defining which of the program
                           * page bits in the status register are significant. */
  OPTION_REG   *option_reg;


  virtual void reset(RESET_TYPE r) override;
  virtual void save_state() override;

  virtual void create_symbols() override;
  #define FILE_REGISTERS  0x100
  virtual unsigned int register_memory_size() const override { return FILE_REGISTERS;}
  virtual void dump_registers() override;
  virtual void tris_instruction(unsigned int) override {}
  virtual void create() override;
  virtual void create_config_memory() override;
  virtual PROCESSOR_TYPE isa() override { return _12BIT_PROCESSOR_; }
  virtual PROCESSOR_TYPE base_isa() override { return _12BIT_PROCESSOR_; }
  virtual instruction * disasm(unsigned int address, unsigned int inst) override
    {
      return disasm12(this, address, inst);
    }
  void interrupt() override {}

  // Declare a set of functions that will allow the base class to
  // get information about the derived classes. NOTE, the values returned here
  // will cause errors if they are used -- the derived classes must define their
  // parameters appropriately.
  virtual unsigned int program_memory_size() const override { return 3; } // A bogus value that will cause errors if used
  // The size of a program memory bank is 2^11 bytes for the 12-bit core
  virtual void create_sfr_map() {}

  // Return the portion of pclath that is used during branching instructions
  // Actually, the 12bit core has no pclath. However, the program counter class doesn't need
  // to know that. Thus this virtual function really just returns the code page for the
  // 12bit cores.

  virtual unsigned int get_pclath_branching_jump() override
    {
      return (status->value.get() & pa_bits) << 4;
    }

  // The modify pcl type instructions execute exactly like call instructions

  virtual unsigned int get_pclath_branching_modpcl() override
    {
      return (status->value.get() & pa_bits) << 4;
    }

  // The valid bits in the FSR register vary across the various 12-bit devices

  virtual unsigned int fsr_valid_bits()
    {
      return 0x1f;  // Assume only 32 register addresses
    }

  virtual unsigned int fsr_register_page_bits()
    {
      return 0;     // Assume only one register page.
    }

  virtual void put_option_reg(unsigned int) override;
  virtual void option_new_bits_6_7(unsigned int) override;

  virtual unsigned int config_word_address() const override { return 0xfff; }
  virtual bool set_config_word(unsigned int address, unsigned int cfg_word) override;
  virtual void enter_sleep() override;
  virtual void exit_sleep() override;

  _12bit_processor(const char *_name = nullptr, const char *desc = nullptr);
  _12bit_processor(const _12bit_processor &) = delete;
  _12bit_processor& operator = (const _12bit_processor &) = delete;
  virtual ~_12bit_processor();

protected:
  OptionTraceType *mOptionTT;
};

#define cpu12 ( (_12bit_processor *)cpu)

#endif

