/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2013,2022 Roy R. Rankin


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


#ifndef SRC_14_BIT_REGISTERS_H_
#define SRC_14_BIT_REGISTERS_H_

#include <string>

#include "gpsim_classes.h"
#include "processor.h"
#include "registers.h"
#include "stimuli.h"
#include "trace.h"
#include "trigger.h"
#include "a2dconverter.h"
#include "wdt.h"

#include "rcon.h"
#include "pir.h"

class INTCON;
class InterruptSource;
class PeripheralSignalSource;
class PinModule;
class TMR0;
class pic_processor;
class _14bit_e_processor;

//---------------------------------------------------------
// BORCON register
//

class BORCON : public sfr_register
{
public:
    BORCON(Processor *, const char *pName, const char *pDesc = nullptr);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
};


//---------------------------------------------------------
// BSR register
//

class BSR : public sfr_register
{
public:
    BSR(Processor *, const char *pName, const char *pDesc = nullptr);

    unsigned int register_page_bits = 0;

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
};


//---------------------------------------------------------
// FSR register
//

class FSR : public sfr_register
{
public:
    FSR(Processor *, const char *pName, const char *pDesc = nullptr);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
};


//---------------------------------------------------------
// FSR_12 register - FSR for the 12-bit core processors.
//
//
class FSR_12 : public FSR
{
public:
    unsigned int valid_bits;
    unsigned int register_page_bits;   /* Only used by the 12-bit core to define
                                        the valid paging bits in the FSR. */
    FSR_12(Processor *, const char *pName,
           unsigned int _register_page_bits, unsigned int _valid_bits);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
};


//---------------------------------------------------------
// Status register
//

class Status_register : public sfr_register
{
public:

#define STATUS_Z_BIT   2
#define STATUS_C_BIT   0
#define STATUS_DC_BIT  1
#define STATUS_PD_BIT  3
#define STATUS_TO_BIT  4
#define STATUS_OV_BIT  3     //18cxxx
#define STATUS_N_BIT   4     //18cxxx
#define STATUS_FSR0_BIT 4     //17c7xx
#define STATUS_FSR1_BIT 6     //17c7xx
#define STATUS_Z       (1<<STATUS_Z_BIT)
#define STATUS_C       (1<<STATUS_C_BIT)
#define STATUS_DC      (1<<STATUS_DC_BIT)
#define STATUS_PD      (1<<STATUS_PD_BIT)
#define STATUS_TO      (1<<STATUS_TO_BIT)
#define STATUS_OV      (1<<STATUS_OV_BIT)
#define STATUS_N       (1<<STATUS_N_BIT)
#define STATUS_FSR0_MODE (3<<STATUS_FSR0_BIT)     //17c7xx
#define STATUS_FSR1_MODE (3<<STATUS_FSR1_BIT)     //17c7xx
#define BREAK_Z_ACCESS 2
#define BREAK_Z_WRITE  1
#define ZCDC_mask STATUS_Z|STATUS_C|STATUS_DC

#define RP_MASK        0x20
    unsigned int break_point = 0;
    unsigned int break_on_z = 0, break_on_c = 0;
    unsigned int rp_mask = RP_MASK;
    unsigned int write_mask;    // Bits that instructions can modify
    RCON *rcon = nullptr;

    Status_register(Processor *, const char *pName, const char *pDesc = nullptr);
    void reset(RESET_TYPE r) override;

    void set_rcon(RCON *p_rcon)
    {
        rcon = p_rcon;
    }

    void put(unsigned int new_value) override;

    inline unsigned int get() override
    {
        get_trace().raw(read_trace.get() | value.get());
        return value.get();
    }

    inline unsigned int put_ZCDC_masked(unsigned int new_value)
    {

	new_value = (new_value & ~(ZCDC_mask)) | (value.get() & (ZCDC_mask));
	put(new_value);
    	return (value.get());
    }

    // Special member function to control just the Z bit

    inline void put_Z(unsigned int new_z)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~STATUS_Z) | ((new_z) ? STATUS_Z : 0));
    }

    inline unsigned int get_Z()
    {
        get_trace().raw(read_trace.get() | value.get());
        return ((value.get() & STATUS_Z) == 0) ? 0 : 1;
    }


    // Special member function to control just the C bit
    void put_C(unsigned int new_c)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~STATUS_C) | ((new_c) ? STATUS_C : 0));
    }

    unsigned int get_C()
    {
        get_trace().raw(read_trace.get() | value.get());
        return ((value.get() & STATUS_C) == 0) ? 0 : 1;
    }

    // Special member function to set Z, C, and DC

    inline void put_Z_C_DC(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_Z | STATUS_C | STATUS_DC)) |
                  ((new_value & 0xff)   ? 0 : STATUS_Z)   |
                  ((new_value & 0x100)  ? STATUS_C : 0)   |
                  (((new_value ^ src1 ^ src2) & 0x10) ? STATUS_DC : 0));
    }

    inline void put_Z_C_DC_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_Z | STATUS_C | STATUS_DC)) |
                  ((new_value & 0xff)   ? 0 : STATUS_Z)   |
                  ((new_value & 0x100)  ? 0 : STATUS_C)   |
                  (((new_value ^ src1 ^ src2) & 0x10) ? 0 : STATUS_DC));
    }

    inline void put_PD(unsigned int new_pd)
    {
        if (rcon)
        {
            rcon->put_PD(new_pd);

        }
        else
        {
            get_trace().raw(write_trace.get() | value.get());
            value.put((value.get() & ~STATUS_PD) | ((new_pd) ? STATUS_PD : 0));
        }
    }

    inline unsigned int get_PD()
    {
        if (rcon)
        {
            return rcon->get_PD();

        }
        else
        {
            get_trace().raw(read_trace.get() | value.get());
            return ((value.get() & STATUS_PD) == 0) ? 0 : 1;
        }
    }

    inline void put_TO(unsigned int new_to)
    {
        if (rcon)
        {
            rcon->put_TO(new_to);

        }
        else
        {
            get_trace().raw(write_trace.get() | value.get());
            value.put((value.get() & ~STATUS_TO) | ((new_to) ? STATUS_TO : 0));
        }
    }

    inline unsigned int get_TO()
    {
        if (rcon)
        {
            return rcon->get_TO();

        }
        else
        {
            get_trace().raw(read_trace.get() | value.get());
            return (((value.get() & STATUS_TO) == 0) ? 0 : 1);
        }
    }

    // Special member function to set Z, C, DC, OV, and N for the 18cxxx family

    // Special member function to control just the N bit
    void put_N_Z(unsigned int new_value)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_Z | STATUS_N)) |
                  ((new_value & 0xff)  ? 0 : STATUS_Z)   |
                  ((new_value & 0x80) ? STATUS_N : 0));
    }

    void put_Z_C_N(unsigned int new_value)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_Z | STATUS_C | STATUS_N)) |
                  ((new_value & 0xff)  ? 0 : STATUS_Z)   |
                  ((new_value & 0x100)  ? STATUS_C : 0)   |
                  ((new_value & 0x80) ? STATUS_N : 0));
    }

    inline void put_Z_C_DC_OV_N(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_Z | STATUS_C | STATUS_DC | STATUS_OV | STATUS_N)) |
                  ((new_value & 0xff)  ? 0 : STATUS_Z)   |
                  ((new_value & 0x100)  ? STATUS_C : 0)   |
                  (((new_value ^ src1 ^ src2) & 0x10) ? STATUS_DC : 0) |
                  (((new_value ^ src1) & 0x80) ? STATUS_OV : 0) |
                  ((new_value & 0x80) ? STATUS_N : 0));
    }

    inline void put_Z_C_DC_OV_N_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_Z | STATUS_C | STATUS_DC | STATUS_OV | STATUS_N)) |
                  ((new_value & 0xff)   ? 0 : STATUS_Z)   |
                  ((new_value & 0x100)  ? 0 : STATUS_C)   |
                  (((new_value ^ src1 ^ src2) & 0x10) ? 0 : STATUS_DC) |
                  ((((src1 & ~src2 & ~new_value) | (new_value & ~src1 & src2)) & 0x80) ? STATUS_OV : 0) |
                  ((new_value & 0x80)   ? STATUS_N : 0));
    }

    // Special member function to control just the FSR mode
    void put_FSR0_mode(unsigned int new_value)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_FSR0_MODE)) |
                  (new_value & 0x03));
    }

    unsigned int get_FSR0_mode(unsigned int /* new_value */ )
    {
        get_trace().raw(write_trace.get() | value.get());
        return (value.get() >> STATUS_FSR0_BIT) & 0x03;
    }

    void put_FSR1_mode(unsigned int new_value)
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~(STATUS_FSR1_MODE)) |
                  (new_value & 0x03));
    }

    unsigned int get_FSR1_mode(unsigned int /* new_value */ )
    {
        get_trace().raw(read_trace.get() | value.get());
        return (value.get() >> STATUS_FSR1_BIT) & 0x03;
    }
};


//---------------------------------------------------------
// Stack
//

class Stack
{
public:
    unsigned int contents[32];         // the stack array
    int pointer = 0;                   // the stack pointer
    unsigned int stack_mask = 7;       // 1 for 12bit, 7 for 14bit, 31 for 16bit
    bool stack_warnings_flag = false;  // Should over/under flow warnings be printed?
    bool break_on_overflow = false;    // Should over flow cause a break?
    bool break_on_underflow = false;   // Should under flow cause a break?

    explicit Stack(Processor *);
    virtual ~Stack() {}

    virtual bool push(unsigned int);
    virtual bool stack_overflow();
    virtual bool stack_underflow();
    virtual unsigned int pop();
    virtual void reset(RESET_TYPE )
    {
        pointer = 0;
    }  // %%% FIX ME %%% reset may need to change
    // because I'm not sure how the stack is affected by a reset.
    virtual bool set_break_on_overflow(bool clear_or_set);
    virtual bool set_break_on_underflow(bool clear_or_set);
    virtual unsigned int get_tos();
    virtual void put_tos(unsigned int);

    bool STVREN = false;
    Processor *cpu;
};


class STKPTR : public sfr_register
{
public:
    enum
    {
        STKUNF = 1 << 6,
        STKOVF = 1 << 7
    };
    STKPTR(Processor *, const char *pName, const char *pDesc = nullptr);

    Stack *stack = nullptr;
    void put_value(unsigned int new_value) override;
    void put(unsigned int new_value) override;
};


class TOSL : public sfr_register
{
public:
    TOSL(Processor *, const char *pName, const char *pDesc = nullptr);

    Stack *stack = nullptr;

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
};


class TOSH : public sfr_register
{
public:
    TOSH(Processor *, const char *pName, const char *pDesc = nullptr);

    Stack *stack = nullptr;

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
};


//
// Stack for enhanced 14 bit porcessors
//
class Stack14E : public Stack
{
public:
    STKPTR stkptr;
    TOSL   tosl;
    TOSH   tosh;

    explicit Stack14E(Processor *);
    ~Stack14E();

    void reset(RESET_TYPE r) override;
    unsigned int pop() override;
    bool push(unsigned int address) override;
    bool stack_overflow() override;
    bool stack_underflow() override;

private:
    _14bit_e_processor* cpu_14e();

#define NO_ENTRY 0x20
};


//---------------------------------------------------------
// W register
class WTraceType;

class WREG : public sfr_register
{
public:
    WREG(Processor *, const char *pName, const char *pDesc = nullptr);
    WREG(const WREG &) = delete;
    WREG& operator = (const WREG &) = delete;
    ~WREG();

protected:
    WTraceType *m_tt;
};


//---------------------------------------------------------
// INDF

class INDF : public sfr_register
{
public:
    unsigned int fsr_mask;
    unsigned int base_address_mask1;
    unsigned int base_address_mask2;

    INDF(Processor *, const char *pName, const char *pDesc = nullptr);
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
    void initialize() override;
};


//---------------------------------------------------------
//
// Indirect_Addressing
//
// This class coordinates the indirect addressing on the 18cxxx
// parts. Each of the registers comprising the indirect addressing
// subsystem: FSRnL,FSRnH, INDFn, POSTINCn, POSTDECn, PREINCn, and
// PLUSWn are each individually defined as sfr_registers AND included
// in the Indirect_Addressing class. So accessing these registers
// is the same as accessing any register: through the core cpu's
// register memory. The only difference for these registers is that
// the

class Indirect_Addressing14;   // Forward reference

//---------------------------------------------------------
// FSR registers

class FSRL14 : public sfr_register
{
public:
    FSRL14(Processor *, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM);
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

protected:
    Indirect_Addressing14  *iam;
};


class FSRH14 : public sfr_register
{
public:
    FSRH14(Processor *, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

protected:
    Indirect_Addressing14  *iam;
};


class INDF14 : public sfr_register
{
public:
    INDF14(Processor *, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;

protected:
    Indirect_Addressing14  *iam;
};


class Indirect_Addressing14
{
public:
    Indirect_Addressing14(pic_processor *cpu, const std::string &n);

    pic_processor *cpu;

    unsigned int fsr_value = 0;     // 16bit concatenation of fsrl and fsrh
    unsigned int fsr_state = 0;     /* used in conjunction with the pre/post incr
			       * and decrement. This is mainly needed for
			       * those instructions that perform read-modify-
			       * write operations on the indirect registers
			       * eg. btg POSTINC1,4 . The post increment must
			       * occur after the bit is toggled and not during
			       * the read operation that's determining the
			       * current state.
			       */
    int     fsr_delta = 0;          /* If there's a pending update to the fsr register
			       * pair, then the magnitude of that update is
			       * stored here.
			       */
    uint64_t current_cycle;      /* Stores the cpu cycle when the fsr was last
			       * changed.
			       */
    FSRL14    fsrl;
    FSRH14    fsrh;
    INDF14    indf;

    //void init(_16bit_processor *new_cpu);
    void put(unsigned int new_value);
    unsigned int get();
    unsigned int get_value();
    void put_fsr(unsigned int new_fsr);
    unsigned int get_fsr_value()
    {
        return fsr_value & 0xfff;
    }
    void update_fsr_value();

    /* bool is_indirect_register(unsigned int reg_address)
     *
     * The purpose of this routine is to determine whether or not the
     * 'reg_address' is the address of an indirect register. This is
     * used by the 'put' and 'get' functions of the indirect registers.
     * Indirect registers are forbidden access to other indirect registers.
     * (Although double indirection in a single instruction cycle would
     * be powerful!).
     */

    inline bool is_indirect_register(unsigned int reg_address)
    {
        unsigned int bank_address = reg_address % 0x80;

        if (bank_address == 0 || bank_address == 1 || bank_address == 4 ||
                bank_address == 5 || bank_address == 6 || bank_address == 7)
        {
            return 1;
        }

        return 0;
    }
};


//---------------------------------------------------------
// PCL - Program Counter Low
//

class PCL : public sfr_register
{
public:
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
    void reset(RESET_TYPE r) override;

    PCL(Processor *, const char *pName, const char *pDesc = nullptr);
};


//---------------------------------------------------------
// PCLATH - Program Counter Latch High
//

class PCLATH : public sfr_register
{
public:
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;

    PCLATH(Processor *, const char *pName, const char *pDesc = nullptr);
};


//---------------------------------------------------------
// PCON - Power Control/Status Register
//
class PCON : public sfr_register
{
public:
    enum
    {
        BOR = 1 << 0, // clear on Brown Out Reset
        POR = 1 << 1,  // clear on Power On Reset
        RI  = 1 << 2,	 // clear on Reset instruction
        RMCLR = 1 << 3, // clear if hardware MCLR occurs
        SBOREN = 1 << 4, //  Software BOR Enable bit
	RWDT   = 1 << 4, //clear on WDT reset
        ULPWUE = 1 << 5, // Ultra Low-Power Wake-up Enable bit
        WDTWV  = 1 << 5, // clear on WDT window violation
        STKUNF = 1 << 6, // Stack undeflow
        STKOVF = 1 << 7 // Stack overflow
    };

    unsigned int valid_bits;

    void put(unsigned int new_value) override;

    PCON(Processor *, const char *pName, const char *pDesc = nullptr,
         unsigned int bitMask = 0x03);
};


class OSCCON;
// The OSCTUNE class is now only a base class - it has a pure virtual method
// Instantiate it using one of the derived specialisations below
class OSCTUNE : public  sfr_register
{
public:
    void put(unsigned int new_value) override;
    virtual void set_osccon(OSCCON *new_osccon)
    {
        osccon = new_osccon;
    }
    void set_ValidBits(unsigned int validbits) { mValidBits = validbits; }

    virtual bool isPLLEn(void)  { return false; }
    virtual float get_freq_trim(void) = 0;

    enum
    {
        TUN0 = 1 << 0,
        TUN1 = 1 << 1,
        TUN2 = 1 << 2,
        TUN3 = 1 << 3,
        TUN4 = 1 << 4,
        TUN5 = 1 << 5,
        PLLEN = 1 << 6,
        INTSRC = 1 << 7
    };
    OSCCON *osccon = nullptr;

    OSCTUNE(Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc)
    {
    }
};

// OSCTUNE with only 5 bits
class OSCTUNE5 : public  OSCTUNE
{
public:
    virtual float get_freq_trim(void);

    OSCTUNE5(Processor *pCpu, const char *pName, const char *pDesc)
        : OSCTUNE(pCpu, pName, pDesc)
    {
        mValidBits = 0x1F; // Can't use initialiser for parent class members
    }
};

// OSCTUNE with 6 bits, no PLL
class OSCTUNE6 : public  OSCTUNE
{
public:
    virtual float get_freq_trim(void);

    OSCTUNE6(Processor *pCpu, const char *pName, const char *pDesc)
        : OSCTUNE(pCpu, pName, pDesc)
    {
        mValidBits = 0x3F; // Can't use initialiser for parent class members
    }
};

// OSCTUNE with 6 bits trim and PLLEN
class OSCTUNEPLL : public  OSCTUNE6
{
public:
    virtual bool isPLLEn(void)  { return value.get() & mValidBits & PLLEN; }

    OSCTUNEPLL(Processor *pCpu, const char *pName, const char *pDesc)
        : OSCTUNE6(pCpu, pName, pDesc)
    {
        mValidBits = 0xFF; // Can't use initialiser for parent class members
    }
};

// OSCTUNE without OSCCON ie p16f610
class OSCTUNE_2 : public  sfr_register
{
public:
    void put(unsigned int new_value) override;
    unsigned int valid_bits = 0x1f;
    void set_freq(double frequency);
    void adjust_freq();
    float base_freq = 0.0;

    enum
    {
        TUN0 = 1 << 0,
        TUN1 = 1 << 1,
        TUN2 = 1 << 2,
        TUN3 = 1 << 3,
        TUN4 = 1 << 4,
    };

    OSCTUNE_2(Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc)
    {
    }
};


// This class is used to trim the frequency of the internal RC clock
//  111111 - Max freq
//  100000 - no adjustment
//  000000 - min freq
class OSCCAL : public  sfr_register
{
public:
    void put(unsigned int new_value) override;
    void set_freq(float base_freq);
    float base_freq = 0.0;

    OSCCAL(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask)
        : sfr_register(pCpu, pName, pDesc)
    {
        mValidBits = bitMask; // Can't use initialiser for parent class members
    }
};


class OSCCON : public sfr_register, public TriggerObject
{
public:
    void put(unsigned int new_value) override;
    void callback() override;
    virtual bool set_rc_frequency(bool override = false);
    virtual void set_osctune(OSCTUNE *new_osctune)
    {
        osctune = new_osctune;
    }
    virtual void set_config_irc(unsigned int cfg_irc)
    {
        config_irc = cfg_irc;
    }
    virtual void set_config_xosc(unsigned int cfg_xosc)
    {
        config_xosc = cfg_xosc;
    }
    virtual void set_config_ieso(unsigned int cfg_ieso)
    {
        config_ieso = cfg_ieso;
    }
    void reset(RESET_TYPE r) override;
    virtual void sleep();
    virtual void wake();
    virtual void por_wake();
    virtual bool internal_RC();
    virtual void clear_irc_stable_bits()
    {
        value.put(value.get() & ~(HTS | LTS));
    }
    virtual uint64_t irc_por_time(); // time to stable intrc after power on reset
    virtual uint64_t irc_lh_time(); // time to stable intrc after tran low to high range
    unsigned int write_mask;
    unsigned int clock_state;
    uint64_t      future_cycle = 0;
    bool         config_irc = false;     // FOSC bits select internal RC oscillator
    bool         config_ieso = false;    //internal/external switchover bit from config word
    bool         config_xosc = false;    // FOSC bits select crystal/resonator
    bool         has_iofs_bit = false;
    bool	       is_sleeping = false;

    OSCTUNE *osctune = nullptr;

    enum MODE
    {
        UNDEF = 0,
        EXCSTABLE, 	// external source
        LFINTOSC,       // Low Freq RC osc
        MFINTOSC,	// Med Freq rc osc
        HFINTOSC,       // High Freq RC osc
        INTOSC,         // IOFS set
        T1OSC,		// T1 OSC
        EC,		// external clock, always stable
        OST,      	// startup
        PLL = 0x10
    };

    enum
    {
        SCS0 = 1 << 0,
        SCS1 = 1 << 1,
        LTS  = 1 << 1,
        HTS  = 1 << 2,
        IOFS = 1 << 2,
        OSTS = 1 << 3,
        IRCF0 = 1 << 4,
        IRCF1 = 1 << 5,
        IRCF2 = 1 << 6,
        IDLEN = 1 << 7
    };

    OSCCON(Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), write_mask(0x71),
          clock_state(OST)
    {
    }
};


/* OSCCON_1 IOFS bit takes 4 ms to stablize
 */

class OSCCON_1 : public OSCCON
{
public:
    //  virtual void callback();
    //  virtual void put(unsigned int new_value);
    uint64_t irc_por_time() override; // time to stable intrc after power on reset
    uint64_t irc_lh_time() override;

    OSCCON_1(Processor *pCpu, const char *pName, const char *pDesc)
        : OSCCON(pCpu, pName, pDesc)
    {
    }
};


class OSCCON2 : public  sfr_register
{
public:
    void put(unsigned int new_value) override;
    void set_osccon(OSCCON *new_osccon)
    {
        osccon = new_osccon;
    }
    OSCCON2(Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), write_mask(0x1c)
    {}

    unsigned int write_mask;
    enum
    {
        LFIOFS  = 1 << 0,		// LFINTOSC Frequency Stable bit
        MFIOFS  = 1 << 1,		// MFINTOSC Frequency Stable bit
        PRISD   = 1 << 2,		// Primary Oscillator Drive Circuit Shutdown bit
        SOSCGO  = 1 << 3,		// Secondary Oscillator Start Control bit
        MFIOSEL = 1 << 4,		// MFINTOSC Select bit
        SOSCRUN = 1 << 6,		// SOSC Run Status bit
        PLLRDY  = 1 << 7		// PLL Run Status bit
    };

    OSCCON  *osccon = nullptr;
};


/* RC clock 16MHz with pll to 64MHz
 */
class OSCCON_HS : public OSCCON
{
public:
    bool set_rc_frequency(bool override = false) override;
    bool internal_RC() override;
    void callback() override;
    void por_wake() override;

    OSCCON_HS(Processor *pCpu, const char *pName, const char *pDesc) :
        OSCCON(pCpu, pName, pDesc), minValPLL(5) {}

    OSCCON2  *osccon2 = nullptr;

    enum
    {
        SCS0  	= 1 << 0,
        SCS1 	= 1 << 1,
        HFIOFS 	= 1 << 2,
        OSTS	= 1 << 3,
        IRCF0	= 1 << 4,
        IRCF1	= 1 << 5,
        IRCF2	= 1 << 6,
        IDLEN	= 1 << 7
    };

    unsigned char minValPLL;
};


/* RC clock 16MHz with no SCS0 or osccon2
 */
class OSCCON_HS2 : public OSCCON
{
public:
    void put(unsigned int new_value) override;
    bool set_rc_frequency(bool override = false) override;
    bool internal_RC() override;
    void callback() override;
    void por_wake() override;

    OSCCON_HS2(Processor *pCpu, const char *pName, const char *pDesc) :
        OSCCON(pCpu, pName, pDesc)
    {
        write_mask = 0x70;
    }

    enum
    {
        HFIOFS  = 1 << 0,
        LFIOFR 	= 1 << 1,
        HFIOFR	= 1 << 3,
        IRCF0	= 1 << 4,
        IRCF1	= 1 << 5,
        IRCF2	= 1 << 6,
    };
};


class OSCSTAT : public  sfr_register
{
public:
    void put(unsigned int /* new_value */ ) override {}

    enum
    {
        HFIOFS = 1 << 0,
        LFIOFR = 1 << 1,
        MFIOFR = 1 << 2,
        HFIOFL = 1 << 3,
        HFIOFR = 1 << 4,
        OSTS   = 1 << 5,
        PLLR   = 1 << 6,
        T1OSCR = 1 << 7
    };
    OSCSTAT(Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc) {}
};


/*
 * OSC status in OSCSTAT register
 */
class OSCCON_2 : public  OSCCON
{
public:
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    void callback() override;
    bool set_rc_frequency(bool override = false) override;
    virtual void set_oscstat(OSCSTAT *_oscstat)
    {
        oscstat = _oscstat;
    }
    virtual void  set_callback();
    void  por_wake() override;
    OSCSTAT *oscstat = nullptr;

    enum
    {
        SCS0   = 1 << 0,
        SCS1   = 1 << 1,
        IRCF0  = 1 << 3,
        IRCF1  = 1 << 4,
        IRCF2  = 1 << 5,
        IRCF3  = 1 << 6,
        SPLLEN = 1 << 7
    };

    OSCCON_2(Processor *pCpu, const char *pName, const char *pDesc)
        : OSCCON(pCpu, pName, pDesc)
    {
    }
};




// Interrupt-On-Change GPIO Register
class IOC :  public sfr_register
{
public:
    IOC(Processor *pCpu, const char *pName, const char *pDesc, unsigned int _valid_bits = 0xff)
        : sfr_register(pCpu, pName, pDesc)
    {
        mValidBits = _valid_bits;
    }

    void put(unsigned int new_value) override
    {
        unsigned int masked_value = new_value & mValidBits;
        get_trace().raw(write_trace.get() | value.get());
        value.put(masked_value);
    }
};


// Interrupt-On-Change  Register
class IOCxF : public IOC
{
public:
    IOCxF(Processor *pCpu, const char *pName, const char *pDesc, unsigned int _valid_bits = 0xff)
        : IOC(pCpu, pName, pDesc, _valid_bits)
    {
    }

    void set_intcon(INTCON *new_value)
    {
        intcon = new_value;
    }
    void put(unsigned int new_value) override;

protected:
    INTCON  *intcon = nullptr;
};


class PicPortRegister;
// WPU set weak pullups on pin by pin basis
//
class WPU  : public  sfr_register
{
public:
    PicPortRegister *wpu_gpio;
    bool wpu_pu = false;

    void put(unsigned int new_value) override;
    void set_wpu_pu(bool pullup_enable);

    WPU(Processor *pCpu, const char *pName, const char *pDesc, PicPortRegister* gpio, unsigned int mask = 0x37)
        : sfr_register(pCpu, pName, pDesc), wpu_gpio(gpio)
    {
        mValidBits = mask; // Can't use initialiser for parent class members
    }
};

// ODCON  Open Drain Control Pin will only sink current if bit is set
//
class ODCON  : public  sfr_register
{
public:
    PicPortRegister *gpio;

    void put(unsigned int new_value) override;

    ODCON(Processor *pCpu, const char *pName, const char *pDesc, PicPortRegister* _gpio, unsigned int mask = 0x37)
        : sfr_register(pCpu, pName, pDesc), gpio(_gpio)
    {
        mValidBits = mask; // Can't use initialiser for parent class members
    }
};


// INLVL  Logic level of input pins, 1=schmitt, 0=ttl
//
class INLVL : public sfr_register
{
public:
    PicPortRegister *gpio;

    void put(unsigned int new_value) override;

    INLVL(Processor *pCpu, const char *pName, const char *pDesc, PicPortRegister* _gpio, unsigned int mask = 0x37)
        : sfr_register(pCpu, pName, pDesc), gpio(_gpio)
    {
        mValidBits = mask; // Can't use initialiser for parent class members
    }
};

class T1CON_G;
class CPS_stimulus;

// Capacitance Sensing Control Register 0
class CPSCON0 : public sfr_register, public TriggerObject, public FVR_ATTACH, public DAC_ATTACH
{
public:
    enum
    {
        T0XCS	= 1 << 0,		// Timer0 External Clock Source Select bit
        CPSOUT	= 1 << 1,		// Capacitive Sensing Oscillator Status bit
        CPSRNG0	= 1 << 2,		// Capacitive Sensing Current Range bits
        CPSRNG1	= 1 << 3,
        CPSRM	= 1 << 6,		// Capacitive Sensing Reference Mode bit
        CPSON	= 1 << 7		// CPS Module Enable bit
    };

    void put(unsigned int new_value) override;
    void set_chan(unsigned int _chan);
    void calculate_freq();
    void set_pin(unsigned int _chan, PinModule *_pin) { pin[_chan] = _pin; }
    void set_FVR_volt(double, unsigned int) override;
    void callback() override;
    void callback_print() override;
    void set_DAC_volt(double voltage, unsigned int chan) override;


    CPSCON0(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~CPSCON0();

    TMR0	*m_tmr0 = nullptr;
    T1CON_G  *m_t1con_g = nullptr;

private:
    unsigned int chan = 0;
    PinModule 	 *pin[16];
    double	 DAC_voltage = 0.0;
    double	 FVR_voltage = 0.0;
    uint64_t	 future_cycle = 0;
    int		 period = 0;
    CPS_stimulus *cps_stimulus = nullptr;
};


// Capacitance Sensing Control Register 1
class CPSCON1 : public sfr_register
{
public:
    void put(unsigned int new_value) override;

    CPSCON1(Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc)
    {
        mValidBits = 0x03;
    }

    CPSCON0	*m_cpscon0 = nullptr;
};


class CPS_stimulus : public stimulus
{
public:
    explicit CPS_stimulus(CPSCON0 *arg, const char *n = nullptr,
                 double _Vth = 0.0, double _Zth = 1e12
                );

    CPSCON0 *m_cpscon0;

    void set_nodeVoltage(double v) override;
};


class SR_MODULE;


// SR LATCH CONTROL 0 REGISTER
class SRCON0 : public sfr_register
{
public:
    enum
    {
        SRPR   = 1 << 0,	// Pulse Reset Input of the SR Latch bit
        SRPS   = 1 << 1,	// Pulse Set Input of the SR Latch bit
        SRNQEN = 1 << 2,	// Latch Not Q Output Enable bit
        SRQEN  = 1 << 3,	// Latch Q Output Enable bit
        SRCLK0 = 1 << 4,  // Latch Clock Divider bits
        SRCLK1 = 1 << 5,  // Latch Clock Divider bits
        SRCLK2 = 1 << 6,  // Latch Clock Divider bits
        SRLEN  = 1 << 7,	// Latch Enable bit
        CLKMASK = SRCLK0 | SRCLK1 | SRCLK2,
        CLKSHIFT = 4
    };

    SRCON0(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *_sr_module);
    void put(unsigned int new_value) override;

private:
    SR_MODULE 	*m_sr_module;
};

class SRCON0_V2 : public sfr_register
{
public:
    enum
    {
	SRCLKEN	= (1 << 0),	//Set Clock Enable bit
	PULSR   = (1 << 2),	//Pulse the Reset Input of the SR Latch bit
	PULSS   = (1 << 3),	//Pulse the Set Input of the SR Latch bit
	C2REN   = (1 << 4),	//C2 Reset Enable bit
	C1SEN   = (1 << 5),	//C1 Set Enable bit
	SR0     = (1 << 6),	//C1OUT pin is the latch Q output
	SR1     = (1 << 7),	//C2OUT pin is the latch NQ output
    };
    SRCON0_V2(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *_sr_module);
    void put(unsigned int new_value) override;
    void set_ValidBits(unsigned int validbits) { mValidBits = validbits; }

private:
    unsigned int    mValidBits = 0xfd;
    SR_MODULE 	    *m_sr_module;
};

class SRCON0_V3 : public sfr_register
{
public:
    enum
    {
	FVREN	= (1 << 0),	//Set Clock Enable bit
	PULSR   = (1 << 2),	//Pulse the Reset Input of the SR Latch bit
	PULSS   = (1 << 3),	//Pulse the Set Input of the SR Latch bit
	C2REN   = (1 << 4),	//C2 Reset Enable bit
	C1SEN   = (1 << 5),	//C1 Set Enable bit
	SR0     = (1 << 6),	//C1OUT pin is the latch Q output
	SR1     = (1 << 7),	//C2OUT pin is the latch NQ output
    };
    SRCON0_V3(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *_sr_module);
    void put(unsigned int new_value) override;
    void set_ValidBits(unsigned int validbits) { mValidBits = validbits; }

private:
    unsigned int    mValidBits = 0xfd;
    SR_MODULE 	    *m_sr_module;
};

//  SR LATCH CONTROL 1 REGISTER
//
class SRCON1 : public sfr_register
{
public:
    enum
    {
        SRRC1E  = 1 << 0,	// Latch C1 Reset Enable bit
        SRRC2E  = 1 << 1,	// Latch C2 Reset Enable bit
        SRRCKE  = 1 << 2, // Latch Reset Clock Enable bit
        SRRPE   = 1 << 3,	// Latch Peripheral Reset Enable bit
        SRSC1E  = 1 << 4,	// Latch C1 Set Enable bit
        SRSC2E  = 1 << 5,	// Latch C2 Set Enable bit
        SRSCKE  = 1 << 6, // Latch Set Clock Enable bit
        SRSPE   = 1 << 7  // Latch Peripheral Set Enable bit
    };

    SRCON1(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *m_sr_module);
    void put(unsigned int new_value) override;
    void set_ValidBits(unsigned int validbits) { mValidBits = validbits; }

private:
    SR_MODULE 	    *m_sr_module;
    unsigned int    mValidBits = 0xdd;
};

//  SR LATCH CONTROL 1 REGISTER
//
class SRCON1_V2 : public sfr_register
{
public:
    enum
    {
        CRCS1 = 1<<7,
	CRCS0 = 1<<6,
        CLKMASK = CRCS1 | CRCS0,
        CLKSHIFT = 6
    };

    SRCON1_V2(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *m_sr_module);
    void put(unsigned int new_value) override;
    void set_ValidBits(unsigned int validbits) { mValidBits = validbits; }

private:
    SR_MODULE 	    *m_sr_module;
    unsigned int    mValidBits = 0xc0;
};


class SRinSink;

class SR_MODULE : public TriggerObject
{
public:
    explicit SR_MODULE(Processor *);
    ~SR_MODULE();

    void	update();
    SRCON0 	*srcon0;
    SRCON1 	*srcon1;

    void 	pulse_reset()
    {
        state_reset = true;
    }
    void 	pulse_set()
    {
        state_set = true;
    }
    void	clock_diff(unsigned int);
    void	clock_enable();
    void	clock_disable();
    void	syncC1out(bool val);
    void	syncC2out(bool val);
    void	setPins(PinModule *, PinModule *, PinModule *);
    void	setState(char);
    void	Qoutput();
    void	NQoutput();
    void	releasePin(int);
    void	set_cxoen(int cm, bool cxoe);

    bool	srlen = false;		//SR latch output enabled
    bool	srqen = false;		//Q Output Enable bit
    bool	srnqen = false;		//Not Q Output Enable bit
    bool	srsc1e = false;		//C1 output does set
    bool	srsc2e = false;		//C2 output does set
    bool	srspe = false;		//Peripheral Set Enable bit
    bool	srscke = false;		//Set Clock Enable bit
    bool	srrc1e = false;		//C1 output does reset
    bool	srrc2e = false;		//C2 output does reset
    bool	srrpe = false;		//Peripheral Reset Enable bit
    bool	srrcke = false;		//Reset Clock Enable bit
    bool	c1oen = false;		//C1 output enabled
    bool	c2oen = false;		//C2 output enabled
    bool	cxoutput = false;	// Comparator output via sr module
    bool	sr0 = false;		// SR0 0 - C1OUT, 1 - SRQ
    bool	sr1 = false;		// SR1 0 - C2OUT, 1 - SRNQ

protected:
    void   callback() override;

    Processor   *cpu;
    uint64_t	future_cycle = 0;
    bool	state_set = false;
    bool	state_reset = false;
    bool	state_Q = false;
    unsigned int srclk = 0;		//Clock Divider
    bool	syncc1out = false;	// Synced output from comparator 1
    bool	syncc2out = false;	// Synced output from comparator 2
    PinModule 	*SRI_pin = nullptr;
    PinModule 	*SRQ_pin = nullptr;
    PinModule 	*SRNQ_pin = nullptr;
    bool	SRI = false;		// state of input pin
    SRinSink	*m_SRinSink = nullptr;
    PeripheralSignalSource *m_SRQsource = nullptr;
    PeripheralSignalSource *m_SRNQsource = nullptr;
    bool                    m_SRQsource_active = false;
    bool                    m_SRNQsource_active = false;
};


class LVDCON_14 : public  sfr_register, public TriggerObject
{
public:
    unsigned int valid_bits = 0;

    enum
    {
        LVDL0 = 1 << 0,
        LVDL1 = 1 << 1,
        LVDL2 = 1 << 2,
        LVDEN = 1 << 4,
        IRVST = 1 << 5,
    };

    LVDCON_14(Processor *, const char *pName, const char *pDesc = nullptr);
    void check_lvd();
    unsigned int 	write_mask = 0x17;
    InterruptSource *IntSrc = nullptr;
    void callback() override;
    void put(unsigned int new_value) override;
    virtual void setIntSrc(InterruptSource *_IntSrc)
    {
        IntSrc = _IntSrc;
    }
};


#endif
