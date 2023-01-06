/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2009,2013 Roy R. Rankin


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


#ifndef SRC_14_BIT_PROCESSORS_H_
#define SRC_14_BIT_PROCESSORS_H_

#include "14bit-registers.h"
#include "gpsim_classes.h"
#include "intcon.h"
#include "ioports.h"
#include "pic-processor.h"
#include "registers.h"
#include "value.h"

// forward references
class OPTION_REG;
class PicPortRegister;
class PicPortBRegister;
class PicTrisRegister;
class _14bit_processor;
class _14bit_e_processor;
class instruction;

extern instruction *disasm14(_14bit_processor *cpu, unsigned int addr, unsigned int inst);
extern instruction *disasm14E(_14bit_e_processor *cpu, unsigned int addr, unsigned int inst);

class CPU_Temp : public Float
{
public:
    CPU_Temp(const char  *_name, double temp, const char *desc) : Float(_name, temp, desc) {}
};


class _14bit_processor : public pic_processor
{
public:

#define EEPROM_SIZE              0x40
#define INTERRUPT_VECTOR         4
#define WDTE                     4

    unsigned int eeprom_size = 0;

    INTCON       *intcon = nullptr;

    void interrupt() override;
    void save_state() override;
    void create() override;
    PROCESSOR_TYPE isa() override { return _14BIT_PROCESSOR_; }
    PROCESSOR_TYPE base_isa() override { return _14BIT_PROCESSOR_; }
    instruction * disasm(unsigned int address, unsigned int inst) override
    {
        return disasm14(this, address, inst);
    }

    // Declare a set of functions that will allow the base class to
    // get information about the derived classes. NOTE, the values returned here
    // will cause errors if they are used -- the derived classes must define their
    // parameters appropriately.
    virtual void create_sfr_map() = 0;
    virtual void option_new_bits_6_7(unsigned int) override = 0;
    void put_option_reg(unsigned int) override;
    virtual void create_symbols() override = 0;

    bool set_config_word(unsigned int address, unsigned int cfg_word) override;
    void create_config_memory() override;
    virtual void oscillator_select(unsigned int mode, bool clkout);

    // Return the portion of pclath that is used during branching instructions
    unsigned int get_pclath_branching_jump() override
    {
        return (pclath->value.get() & 0x18) << 8;
    }

    // Return the portion of pclath that is used during modify PCL instructions
    unsigned int get_pclath_branching_modpcl() override
    {
        return (pclath->value.get() & 0x1f) << 8;
    }

    virtual unsigned int map_fsr_indf() { return this->fsr->value.get(); }

    virtual unsigned int eeprom_get_size() { return 0; }
    virtual unsigned int eeprom_get_value(unsigned int /* address */ ) { return 0; }
    virtual void eeprom_put_value(unsigned int /* value */,
                                  unsigned int /* address */ )
    {
    }

    virtual unsigned int program_memory_size() const override = 0;
    unsigned int get_program_memory_at_address(unsigned int address) override;
    void enter_sleep() override;
    void exit_sleep() override;
    virtual bool hasSSP() { return has_SSP; }
    virtual void set_hasSSP() { has_SSP = true; }
    virtual unsigned int get_device_id() { return 0xffffffff; }
    virtual unsigned int get_user_ids(unsigned int /* index */ ) { return 0xffffffff; }

    _14bit_processor(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~_14bit_processor();
    bool get_intedgx(int index) override;

    bool          two_speed_clock = false;
    unsigned int  config_clock_mode = 0;
    CPU_Temp      *m_cpu_temp = nullptr;


protected:
    bool has_SSP = false;
    OPTION_REG   *option_reg;
    unsigned int ram_top = 0;
    unsigned int wdt_flag = 0;
};


#define cpu14 ( (_14bit_processor *)cpu)


/***************************************************************************
 *
 * Include file for:  P16C84, P16F84, P16F83, P16CR83, P16CR84
 *
 * The x84 processors have a 14-bit core, eeprom, and are in an 18-pin
 * package. The class taxonomy is:
 *
 *   pic_processor
 *      |-> 14bit_processor
 *             |
 *             |----------\
 *                         |
 *                         |- P16C8x
 *                              |->P16C84
 *                              |->P16F84
 *                              |->P16C83
 *                              |->P16CR83
 *                              |->P16CR84
 *
 ***************************************************************************/

class Pic14Bit : public  _14bit_processor
{
public:
    Pic14Bit(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~Pic14Bit();

    INTCON_14_PIR    intcon_reg;
    INT_pin	   int_pin;

    PicPortRegister  *m_porta;
    PicTrisRegister  *m_trisa;

    PicPortBRegister *m_portb;
    PicTrisRegister  *m_trisb;

    PROCESSOR_TYPE isa() override
    {
        return _14BIT_PROCESSOR_;
    }
    void create_symbols() override;
    void create_sfr_map() override;
    void option_new_bits_6_7(unsigned int bits) override;
};


// 14 bit processors with extended instructions
//
class _14bit_e_processor : public _14bit_processor
{
public:
    unsigned int mclr_pin;

    INTCON_14_PIR    intcon_reg;
    BSR   bsr;
    PCON  pcon;
    WDTCON wdtcon;
    Indirect_Addressing14  ind0;
    Indirect_Addressing14  ind1;
    sfr_register		 status_shad;
    sfr_register		 wreg_shad;
    sfr_register		 bsr_shad;
    sfr_register		 pclath_shad;
    sfr_register		 fsr0l_shad;
    sfr_register		 fsr0h_shad;
    sfr_register		 fsr1l_shad;
    sfr_register		 fsr1h_shad;
    INT_pin		 int_pin;

    void set_mclr_pin(unsigned int pin)
    {
        mclr_pin = pin;
    }
    PROCESSOR_TYPE isa() override { return _14BIT_PROCESSOR_; }
    PROCESSOR_TYPE base_isa() override { return _14BIT_E_PROCESSOR_; }
    instruction * disasm(unsigned int address, unsigned int inst) override
    {
        return disasm14E(this, address, inst);
    }

    _14bit_e_processor(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~_14bit_e_processor();

    void create_symbols() override;
    void create_sfr_map() override;
    void interrupt() override;
    bool exit_wdt_sleep() override;
    bool swdten_active() override
    {
        return (wdt_flag == 1); // WDTCON can enable WDT
    }

    void enter_sleep() override;
    void exit_sleep() override;
    void reset(RESET_TYPE r) override;
    void create_config_memory() override;
    bool set_config_word(unsigned int address, unsigned int cfg_word) override;
    void oscillator_select(unsigned int mode, bool clkout) override;
    virtual void program_memory_wp(unsigned int mode);

    // Return the portion of pclath that is used during branching instructions
    unsigned int get_pclath_branching_jump() override
    {
        return (pclath->value.get() & 0x18) << 8;
    }

    // Return the portion of pclath that is used during modify PCL instructions
    unsigned int get_pclath_branching_modpcl() override
    {
        return (pclath->value.get() & 0x1f) << 8;
    }

    void Wput(unsigned int) override;
    unsigned int Wget() override;

protected:
    unsigned int wdt_flag = 0;
};


#define cpu14e ( (_14bit_e_processor *)cpu)

#endif
