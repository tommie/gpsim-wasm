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



#ifndef SRC_16_BIT_PROCESSORS_H_
#define SRC_16_BIT_PROCESSORS_H_

#include <glib.h>
#include <string>
#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "16bit-registers.h"
#include "pic-processor.h"
#include "intcon.h"
#include "pie.h"
#include "pir.h"
#include "rcon.h"
#include "registers.h"
#include "ssp.h"
#include "uart.h"
#include "value.h"


// forward references
class ADCON0;
class ADCON0_V2;
class ADCON1;
class ADCON1_V2;
class ADCON2_V2;
class PicLatchRegister;
class PicPortBRegister;
class PicPortRegister;
class PicTrisRegister;
class instruction;

extern instruction *disasm16(pic_processor *cpu, unsigned int address, unsigned int inst);


//------------------------------------------------------------------------
//
//    pic_processor
//        |
//        \__ _16bit_processor
//
// Base class for the 16bit PIC processors
//
class _16bit_processor : public pic_processor
{
public:

    static const unsigned int CONFIG1L = 0x300000;
    static const unsigned int CONFIG1H = 0x300001;
    static const unsigned int CONFIG2L = 0x300002;
    static const unsigned int CONFIG2H = 0x300003;
    static const unsigned int CONFIG3L = 0x300004;
    static const unsigned int CONFIG3H = 0x300005;
    static const unsigned int CONFIG4L = 0x300006;
    static const unsigned int CONFIG4H = 0x300007;
    static const unsigned int CONFIG5L = 0x300008;
    static const unsigned int CONFIG5H = 0x300009;
    static const unsigned int CONFIG6L = 0x30000A;
    static const unsigned int CONFIG6H = 0x30000B;
    static const unsigned int CONFIG7L = 0x30000C;
    static const unsigned int CONFIG7H = 0x30000D;

    // The early 18xxx parts all contain ports A,B,C
    PicPortRegister  *m_porta;
    PicTrisRegister  *m_trisa;
    PicLatchRegister *m_lata;

    PicPortBRegister *m_portb;
    PicTrisRegister  *m_trisb;
    PicLatchRegister *m_latb;

    PicPortRegister  *m_portc;
    PicTrisRegister  *m_trisc;
    PicLatchRegister *m_latc;


    sfr_register adresl;
    sfr_register adresh;
    INTCON_16    intcon;
    INTCON2      intcon2;
    INTCON3      intcon3;
    BSR          bsr;
    TMR0_16      tmr0l;
    TMR0H        tmr0h;
    T0CON        t0con;
    RCON         rcon;
    PIR1v2       pir1;
    sfr_register ipr1;
    sfr_register ipr2;
    T1CON        *t1con;
    PIE          pie1;
    PIR2v2       *pir2;
    PIE          pie2;
    T2CON        t2con;
    PR2          pr2;
    TMR2         tmr2;
    TMRL         tmr1l;
    TMRH         tmr1h;
    CCPCON       ccp1con;
    CCPRL        ccpr1l;
    CCPRH        ccpr1h;
    CCPCON       ccp2con;
    CCPRL        ccpr2l;
    CCPRH        ccpr2h;
    TMRL         tmr3l;
    TMRH         tmr3h;
    T3CON        *t3con;
    PIR_SET_2    pir_set_def;

    OSCCON      *osccon;
    LVDCON       lvdcon;
    WDTCON       wdtcon;

    sfr_register prodh, prodl;

    sfr_register pclatu;

    Fast_Stack   fast_stack;
    Indirect_Addressing  ind0;
    Indirect_Addressing  ind1;
    Indirect_Addressing  ind2;
    USART_MODULE         usart;
    //Stack16              stack16;
    TBL_MODULE           tbl;
    TMR2_MODULE          tmr2_module;
    TMR3_MODULE          tmr3_module;
    SSP_MODULE           ssp;

    // Some configuration stuff for stripping down where needed
    virtual bool HasPortC() { return true; }
    virtual bool HasCCP2() { return true; }
    virtual bool MovedReg() { return false; }
    virtual bool T3HasCCP() { return true; }

    virtual void create_base_ports();

    virtual OSCCON * getOSCCON() { return new OSCCON(this, "osccon", "OSC Control"); }

    void create_symbols() override;

    void interrupt() override;
    void create() override;
    PROCESSOR_TYPE isa() override { return _PIC17_PROCESSOR_; }
    PROCESSOR_TYPE base_isa() override { return _PIC17_PROCESSOR_; }
    unsigned int access_gprs() override { return 0x80; }
    instruction * disasm(unsigned int address, unsigned int inst) override
    {
        return disasm16(this, address, inst);
    }
    virtual void create_sfr_map();
    virtual void delete_sfr_map();
    void create_config_memory() override;

    // Return the portion of pclath that is used during branching instructions
    unsigned int get_pclath_branching_jump() override
    {
        return ((pclatu.value.get() << 16) | ((pclath->value.get() & 0xf8) << 8));
    }

    // Return the portion of pclath that is used during modify PCL instructions
    unsigned int get_pclath_branching_modpcl() override
    {
        return ((pclatu.value.get() << 16) | ((pclath->value.get() & 0xff) << 8));
    }

    void option_new_bits_6_7(unsigned int) override;

    // Declare a set of functions that will allow the base class to
    // get information about the derived classes. NOTE, the values returned here
    // will cause errors if they are used (in some cases)
    // -- the derived classes must define their parameters appropriately.

    unsigned int register_memory_size() const override { return 0x1000; }
    virtual unsigned int last_actual_register() const { return 0x0F7F; }
    void set_out_of_range_pm(unsigned int address, unsigned int value) override;

    virtual void create_iopin_map();

    int  map_pm_address2index(int address) const override { return address / 2; }
    int  map_pm_index2address(int index) const override { return index * 2; }
    unsigned int get_program_memory_at_address(unsigned int address) override;
    unsigned int get_config_word(unsigned int address) override;
    virtual unsigned int get_device_id() { return 0; }
    bool set_config_word(unsigned int address, unsigned int cfg_word) override;
    virtual unsigned int configMemorySize() { return CONFIG7H - CONFIG1L + 1; }
    virtual unsigned int IdentMemorySize() const { return 4;}  // four words default (18F)
    void enter_sleep() override;
    void exit_sleep() override;
    void osc_mode(unsigned int) override;
    virtual void set_extended_instruction(bool);
    virtual bool extended_instruction() { return extended_instruction_flag; }

    static pic_processor *construct();

    _16bit_processor(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~_16bit_processor();

    unsigned int getCurrentDisasmAddress() { return m_current_disasm_address; }
    unsigned int getCurrentDisasmIndex() { return m_current_disasm_address / 2; }
    void setCurrentDisasmAddress(unsigned a) { m_current_disasm_address = a; }
    virtual void init_pir2(PIR *pir2, unsigned int bitMask);

protected:
    unsigned int m_current_disasm_address;  // Used only when .hex/.cod files are loaded

    unsigned int idloc[4];    ///< ID locations - not all 16-bit CPUs have 8 bytes
    bool	extended_instruction_flag;	// Instruction set extension and Indexed Addressing

    unsigned int last_register;
};


class _16bit_compat_adc : public _16bit_processor
{
public:
    _16bit_compat_adc(const char *_name = nullptr, const char *desc = nullptr);
    ~_16bit_compat_adc();

    void create_symbols() override;
    void create() override;
    void create_sfr_map() override;
    virtual void a2d_compat();

    ADCON0 *adcon0 = nullptr;
    ADCON1 *adcon1 = nullptr;
};


class _16bit_v2_adc : public _16bit_processor
{
public:
    _16bit_v2_adc(const char *_name = nullptr, const char *desc = nullptr);
    ~_16bit_v2_adc();

    virtual void create(int nChannels);

    ADCON0_V2 *adcon0 = nullptr;
    ADCON1_V2 *adcon1 = nullptr;
    ADCON2_V2 *adcon2 = nullptr;
};


#define cpu16 ( (_16bit_processor *)cpu)

#define FOSC0   (1<<0)
#define FOSC1   (1<<1)
#define FOSC2   (1<<2)
// FOSC3 may not be used
#define FOSC3   (1<<3)
#define PLLCFG  (1<<4)
#define OSCEN   (1<<5)
//RRR#define IESO    (1<<7)

//------------------------------------------------------------------------
// Config1H - default 3 bits FOSC

#define CONFIG1H_default (OSCEN | FOSC2 | FOSC1 | FOSC0)

class Config1H : public ConfigWord
{
public:
    Config1H(_16bit_processor *pCpu, unsigned int addr)
        : ConfigWord("CONFIG1H", CONFIG1H_default, "Oscillator configuration", pCpu, addr)
    {
        set(CONFIG1H_default);
    }

    void set(gint64 v) override
    {
        Integer::set(v);

        if (m_pCpu)
        {
            //RRRm_pCpu->osc_mode(v & ( FOSC2 | FOSC1 | FOSC0));
            m_pCpu->osc_mode(v);
        }
    }

    std::string toString() override;
};


//------------------------------------------------------------------------
// Config1H -  4 bits FOSC

class Config1H_4bits : public ConfigWord
{
public:
    Config1H_4bits(_16bit_processor *pCpu, unsigned int addr, unsigned int def_val)
        : ConfigWord("CONFIG1H", def_val, "Oscillator configuration", pCpu, addr)
    {
        set(def_val);
    }

    void set(gint64 v) override
    {
        Integer::set(v);

        if (m_pCpu)
        {
            //m_pCpu->osc_mode(v & ( FOSC3 | FOSC2 | FOSC1 | FOSC0));
            m_pCpu->osc_mode(v);
        }
    }

    std::string toString() override;
};


//------------------------------------------------------------------------
// Config2H - WDTEN has 4 possible values such as 18f26k22
//  The default Config2H register controls the 18F series WDT.
class Config2H_WDTEN : public ConfigWord
{
#define WDTEN_MASK  0x3
#define WDTPSN_MASK  0x3C
#define WDTPSN_SHIFT  2


public:
    Config2H_WDTEN(_16bit_processor *pCpu, unsigned int addr)
        : ConfigWord("CONFIG2H", WDTEN_MASK|WDTPSN_MASK, "WatchDog configuration", pCpu, addr)
    {
        set(WDTEN_MASK|WDTPSN_MASK);
    }
    void set(gint64 v) override
    {
        Integer::set(v);
        if (m_pCpu)
        {
            int postscale = (int)((v & WDTPSN_MASK) >> WDTPSN_SHIFT);
            m_pCpu->wdt->set_postscale(postscale);
            m_pCpu->wdt->initialize(((int)(v & WDTEN_MASK)));
        }
    }

    std::string toString() override
    {
        gint64 i64;
        get(i64);
        int i = i64 & 0xfff;
        char buff[256];
 	const char *en;
	switch(i & WDTEN_MASK)
        {
	case 0:
	    en = "disabled";
	    break;
	case 1:
	    en = "enabled when active";
	    break;
	case 2:
	    en = "SWDTEN active";
	    break;
	case 3:
	    en = "enabled";
	    break;
	}

        snprintf(buff, sizeof(buff),
                 "$%04x\n"
                 " WDTEN=%d - WDT is %s, prescale=1:%d\n",
                 i,
                 (i & WDTEN_MASK), en,
                 1 << ((i & WDTPSN_MASK) >> WDTPSN_SHIFT));
        return buff;   
    }
};

class Config3H : public ConfigWord
{
public:
    Config3H(_16bit_processor *pCpu, unsigned int addr, unsigned int def_val)
        : ConfigWord("CONFIG3H", def_val, "Configuration Register 3 High", pCpu, addr)
    {
        set(def_val);
    }

    void set(gint64 v) override
    {
        Integer::set(v);

        if (m_pCpu)
        {
            m_pCpu->set_config3h(v);
        }
    }

    std::string toString() override
    {
        gint64 i64;
        get(i64);

        if (m_pCpu)
        {
            return m_pCpu->string_config3h(i64);

        }
        else
        {
            return "m_PCpu not defined";
        }
    }
};


#endif
