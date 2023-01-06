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

#ifndef SRC_P12X_H_
#define SRC_P12X_H_

#include "12bit-processors.h"
#include "a2dconverter.h"
#include "gpsim_classes.h"
#include "ioports.h"
#include "pic-ioports.h"
#include "pic-processor.h"
#include "processor.h"
#include "registers.h"

class CMCON0;
class IO_bi_directional_pu;
class P12_I2C_EE;
class P12bitBase;
class Stimulus_Node;

class P12_OSCCON : public sfr_register
{
public:
    enum
    {
        FOSC4 = 1 << 0
    };

    P12_OSCCON(Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc)
    {
    }

    void put(unsigned int new_value) override;
    void set_cpu(P12bitBase *pCPU) { m_CPU = pCPU; }

private:
    P12bitBase *m_CPU = nullptr;
};


class GPIO : public PicPortRegister
{
public:
    GPIO(P12bitBase *pCpu, const char *pName, const char *pDesc,
         unsigned int numIopins,
         unsigned int enableMask,
         unsigned int resetMask  = (1 << 3),
         unsigned int wakeupMask = 0x0b,
         unsigned int configMaskMCLRE = (1 << 4));

    void setbit(unsigned int bit_number, char new_value) override;
    void setPullUp(bool bNewPU, bool mclr);

private:
    P12bitBase *m_CPU;
    bool m_bPU = false;
    unsigned int m_resetMask;
    unsigned int m_wakeupMask;
    unsigned int m_configMaskMCLRE;
};


//--------------------------------------------------------
/*
 *         IN_SignalControl is used to set a pin as input
 *                 regardless of the setting to the TRIS register
 */
class IN_SignalControl : public SignalControl
{
public:
    IN_SignalControl() {}
    ~IN_SignalControl() {}

    char getState() override
    {
        return '1';
    }
    void release() override {}
};


//--------------------------------------------------------
/*
 *         OUT_SignalControl is used to set a pin as input
 *                 regardless of the setting to the TRIS register
 */
class OUT_SignalControl : public SignalControl
{
public:
    OUT_SignalControl() {}
    ~OUT_SignalControl() {}

    char getState() override { return '0'; }
    void release() override {}
};


//--------------------------------------------------------
/*
 *         OUT_DriveControl is used to override output
 *                 regardless of the setting to the GPIO register
 */
class OUT_DriveControl : public SignalControl
{
public:
    OUT_DriveControl() {}
    ~OUT_DriveControl() {}

    char getState() override { return '1'; }
    void release() override {}
};


class P12bitBase : public  _12bit_processor
{
public:
    P12bitBase(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P12bitBase();

    virtual PROCESSOR_TYPE isa() override { return _P12C508_; }
    virtual void create_symbols() override;

    virtual void enter_sleep() override;
    virtual void create_sfr_map() override;
    virtual void dump_registers() override;
    virtual void tris_instruction(unsigned int tris_register) override;
    virtual void reset(RESET_TYPE r) override;

    static Processor *construct(const char *name);
    virtual void create_iopin_map();
    virtual void create_config_memory() override;

    virtual unsigned int fsr_valid_bits() override
    {
        return 0x1f;  // Assume only 32 register addresses
    }

    virtual unsigned int fsr_register_page_bits() override
    {
        return 0;     // Assume only one register page.
    }

    virtual void option_new_bits_6_7(unsigned int) override;

    GPIO            *m_gpio = nullptr;
    PicTrisRegister *m_tris = nullptr;
    P12_OSCCON       osccal;

    IN_SignalControl *m_IN_SignalControl = nullptr;
    OUT_SignalControl *m_OUT_SignalControl = nullptr;
    OUT_DriveControl *m_OUT_DriveControl = nullptr;

    virtual void updateGP2Source();
    virtual void freqCalibration();
    virtual void setConfigWord(unsigned int val, unsigned int diff);

    unsigned int configWord = 0;

    // bits of Configuration word
    enum
    {
        FOSC0  = 1 << 0,
        FOSC1  = 1 << 1,
        WDTEN  = 1 << 2,
        CP     = 1 << 3,
        MCLRE  = 1 << 4
    };
};


class P12C508 : public  P12bitBase
{
public:
    P12C508(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P12C508();

    static Processor *construct(const char *name);
    virtual void create() override;
    virtual unsigned int program_memory_size() const override { return 0x200; }
};


class P12F508 : public P12C508
{
public:
    P12F508(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P12F508();

    static Processor *construct(const char *name);
    virtual PROCESSOR_TYPE isa() override { return _P12F508_; }
};


// A 12c509 is like a 12c508
class P12C509 : public P12C508
{
public:
    P12C509(const char *_name = nullptr, const char *desc = nullptr);
    ~P12C509();

    virtual PROCESSOR_TYPE isa() override { return _P12C509_; }

    virtual unsigned int program_memory_size() const override { return 0x400; }

    virtual void create_sfr_map() override;

    virtual unsigned int fsr_valid_bits() override
    {
        return 0x3f;  // 64 registers in all (some are actually aliased)
    }

    virtual unsigned int fsr_register_page_bits() override
    {
        return 0x20;  // 509 has 2 register banks
    }

    static Processor *construct(const char *name);
    virtual void create() override;
};


class P12F509 : public P12C509
{
public:
    P12F509(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P12F509();

    static Processor *construct(const char *name);
    virtual PROCESSOR_TYPE isa() override { return _P12F509_; }
};


// 12F510 - like a '509, but has an A2D and a comparator.
class P12F510 : public P12F509
{
public:
    P12F510(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P12F510();

    static Processor *construct(const char *name);
    virtual PROCESSOR_TYPE isa() override { return _P12F510_; }
};


// A 12CE518 is like a 12c508
class P12CE518 : public P12C508
{
public:
    P12CE518(const char *_name = nullptr, const char *desc = nullptr);
    ~P12CE518();

    virtual PROCESSOR_TYPE isa() override { return _P12CE518_; }
    virtual void tris_instruction(unsigned int tris_register) override;

    static Processor *construct(const char *name);
    virtual void create() override;
    virtual void create_iopin_map() override;
    virtual void freqCalibration() override;

private:
    P12_I2C_EE *m_eeprom;
    Stimulus_Node *scl;
    Stimulus_Node	*sda;
    IO_bi_directional_pu *io_scl;
    IO_bi_directional_pu *io_sda;
};


// A 12ce519 is like a 12ce518
class P12CE519 : public P12CE518
{
public:
    P12CE519(const char *_name = nullptr, const char *desc = nullptr);
    ~P12CE519();

    virtual PROCESSOR_TYPE isa() override { return _P12CE519_; }

    virtual unsigned int program_memory_size() const override { return 0x400; }

    virtual void create_sfr_map() override;

    virtual unsigned int fsr_valid_bits() override
    {
        return 0x3f;  // 64 registers in all (some are actually aliased)
    }

    virtual unsigned int fsr_register_page_bits() override
    {
        return 0x20;  // 519 has 2 register banks
    }

    static Processor *construct(const char *name);
    virtual void create() override;
};


//  10F200
class P10F200 : public P12bitBase
{
public:
    P10F200(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P10F200();

    virtual PROCESSOR_TYPE isa() override { return _P10F200_; }
    virtual unsigned int program_memory_size() const override { return 0x100; }

    static Processor *construct(const char *name);
    virtual void create() override;
    virtual void create_iopin_map() override;
    // GP2 can be driven by either FOSC/4, TMR 0, or the GP I/O driver
    virtual void updateGP2Source() override;
    virtual void freqCalibration() override;
    // WDT causes reset on sleep
    virtual bool exit_wdt_sleep() override { return false; }
};


// A 10F202 is like a 10f200
class P10F202 : public P10F200
{
public:
    P10F202(const char *_name = nullptr, const char *desc = nullptr);
    ~P10F202();

    virtual PROCESSOR_TYPE isa() override { return _P10F202_; }
    virtual unsigned int program_memory_size() const override { return 0x200; }

    static Processor *construct(const char *name);
    virtual void create() override;
};


// A 10F204 is like a 10f200
class P10F204 : public P10F200
{
public:
    P10F204(const char *_name = nullptr, const char *desc = nullptr);
    ~P10F204();

    virtual PROCESSOR_TYPE isa() override { return _P10F204_; }

    static Processor *construct(const char *name);
    virtual void create() override;
    // GP2 can be driven by either FOSC/4, COUT, TMR 0, or the GP I/O driver
    virtual void updateGP2Source() override;

protected:
    CMCON0 *m_cmcon0;
};


// A 10F220 is based on 10f200
class P10F220 : public P10F200
{
public:
    P10F220(const char *_name = nullptr, const char *desc = nullptr);
    ~P10F220();

    virtual PROCESSOR_TYPE isa() override { return _P10F220_; }

    static Processor *construct(const char *name);
    virtual void create() override;
    virtual void enter_sleep() override;
    virtual void exit_sleep() override;
    virtual void setConfigWord(unsigned int val, unsigned int diff) override;

    // Bits of configuration word
    enum
    {
        IOSCFS  = 1 << 0,
        NOT_MCPU  = 1 << 1,
    };

    ADCON0_10 adcon0;
    ADCON1 adcon1;
    sfr_register  adres;
};


// A 10F220 is like a 10f220
class P10F222 : public P10F220
{
public:
    P10F222(const char *_name = nullptr, const char *desc = nullptr);
    ~P10F222();

    virtual PROCESSOR_TYPE isa() override { return _P10F222_; }

    virtual unsigned int program_memory_size() const override { return 0x200; }
    static Processor *construct(const char *name);
    virtual void create() override;
    // GP2 can be driven by either FOSC/4, TMR 0, or the GP I/O driver
    //virtual void updateGP2Source();
};


class P16F505 : public P12bitBase
{
public:
    P16F505(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P16F505();

    enum
    {
        FOSC0  = 1 << 0,
        FOSC1  = 1 << 1,
        FOSC2  = 1 << 2,
        WDTEN  = 1 << 3,
        CP     = 1 << 4,
        MCLRE  = 1 << 5
    };

    static Processor *construct(const char *name);
    virtual PROCESSOR_TYPE isa() override { return _P16F505_; }

    virtual void create() override;
    virtual void create_symbols() override;
    virtual void create_iopin_map() override;
    virtual void create_sfr_map() override;
    virtual void create_config_memory() override;
    virtual void tris_instruction(unsigned int tris_register) override;
    virtual void setConfigWord(unsigned int val, unsigned int diff) override;
    virtual void updateGP2Source() override;
    virtual void option_new_bits_6_7(unsigned int bits) override;
    virtual void reset(RESET_TYPE r) override;
    virtual void dump_registers() override;

    virtual unsigned int program_memory_size() const override { return 0x400; }
    virtual unsigned int fsr_valid_bits() override { return 0x7f; }
    virtual unsigned int fsr_register_page_bits() override { return 0x60; }

    GPIO            *m_portb;
    GPIO            *m_portc;
    PicTrisRegister *m_trisb;
    PicTrisRegister *m_trisc;
};

#endif //  SRC_P12X_H_

