/*
   Copyright (C) 2009 Roy R. Rankin

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

#ifndef SRC_P12F629_H_
#define SRC_P12F629_H_

#include "14bit-processors.h"
#include "14bit-tmrs.h"
#include "intcon.h"
#include "pir.h"
#include "pie.h"
#include "eeprom.h"
#include "comparator.h"
#include "a2dconverter.h"
#include "pm_rd.h"
#include "cwg.h"
#include "nco.h"
#include "clc.h"

class WPU;
class IOC;
class PicPortGRegister;

class P12F629 : public _14bit_processor
{
public:
    INTCON_14_PIR    intcon_reg;
    ComparatorModule comparator;
    PIR_SET_1 	pir_set_def;
    PIE     	pie1;
    PIR    	*pir1;
    T1CON   	t1con;
    TMRL    	tmr1l;
    TMRH    	tmr1h;
    PCON    	pcon;
    OSCCAL  	osccal;
    EEPROM_PIR 	*e;
    INT_pin	int_pin;

    PicPortGRegister  *m_gpio;
    PicTrisRegister  *m_trisio;
    WPU		   *m_wpu;
    IOC		   *m_ioc;

    P12F629(const char *_name = nullptr, const char *desc = nullptr);
    P12F629& operator = (const P12F629 &) = delete;
    P12F629(const P12F629 &) = delete;
    ~P12F629();

    virtual PIR *get_pir2()
    {
        return nullptr;
    }
    virtual PIR *get_pir1()
    {
        return pir1;
    }
    virtual PIR_SET *get_pir_set()
    {
        return &pir_set_def;
    }


    virtual PROCESSOR_TYPE isa() override
    {
        return _P12F629_;
    }

    static Processor *construct(const char *name);
    virtual void create_sfr_map() override;
    virtual void create_symbols() override;
    virtual void set_out_of_range_pm(unsigned int address, unsigned int value) override;
    virtual void create_iopin_map();
    virtual void create(int ram_top, int eeprom_size);
    virtual unsigned int register_memory_size() const override
    {
        return 0x100;
    }
    virtual void option_new_bits_6_7(unsigned int bits) override;
    virtual unsigned int program_memory_size() const override
    {
        return 0x400;
    }
    virtual void create_config_memory() override;
    virtual bool set_config_word(unsigned int address, unsigned int cfg_word) override;
    virtual void enter_sleep() override;
    virtual void exit_sleep() override;
};


class P12F675 : public P12F629
{
public:

    ANSEL_12F  ansel;
    ADCON0_12F adcon0;
    ADCON1 adcon1;
    sfr_register  adresh;
    sfr_register  adresl;


    virtual PROCESSOR_TYPE isa() override
    {
        return _P12F675_;
    }

    virtual void create(int ram_top, int eeprom_size) override;
    virtual unsigned int program_memory_size() const override
    {
        return 0x400;
    }

    P12F675(const char *_name = nullptr, const char *desc = nullptr);
    ~P12F675();
    static Processor *construct(const char *name);
    virtual void create_sfr_map() override;
};


class P12F683 : public P12F675
{
public:
    T2CON   t2con;
    PR2     pr2;
    TMR2    tmr2;
    CCPCON  ccp1con;
    CCPRL   ccpr1l;
    CCPRH   ccpr1h;
    WDTCON  wdtcon;
    OSCCON  *osccon;
    OSCTUNE5 osctune;          // with 5-bit trim, no PLLEN


    virtual PROCESSOR_TYPE isa() override
    {
        return _P12F683_;
    }

    virtual void create(int ram_top, int eeprom_size) override;
    virtual unsigned int program_memory_size() const override
    {
        return 0x800;
    }

    P12F683(const char *_name = nullptr, const char *desc = nullptr);
    ~P12F683();
    static Processor *construct(const char *name);
    virtual void create_sfr_map() override;
};


class P10F32X : public _14bit_processor
{
public:
    INTCON_14_PIR    intcon_reg;
    PIR_SET_1 pir_set_def;
    PIE     pie1;
    PIR    *pir1;
    T2CON   t2con;
    TMR2    tmr2;
    PR2     pr2;
    PCON    pcon;
    ANSEL_P ansela;
    FVRCON  fvrcon;
    BORCON  borcon;
    WDTCON  wdtcon;
    OSCCON  *osccon;
    ADCON0_32X  adcon0;
    ADCON1  adcon1;
    sfr_register  adres;
    PWMxCON       pwm1con;
    sfr_register  pwm1dcl;
    sfr_register  pwm1dch;
    PWMxCON       pwm2con;
    sfr_register  pwm2dcl;
    sfr_register  pwm2dch;
    PM_RW	  pm_rw;
    CWG		  cwg;
    NCO2	  nco;
    CLCDATA	  clcdata;
    CLC		  clc1;
    sfr_register  vregcon;
    OSC_SIM       lfintosc;

    PicPortIOCRegister  *m_porta;
    PicTrisRegister  *m_trisa;
    PicLatchRegister *m_lata;
    WPU		     *m_wpu;
    IOC              *m_iocap;
    IOC              *m_iocan;
    IOCxF            *m_iocaf;

    CLC_BASE::data_in lcxdx[8] =
    {
        CLC_BASE::LC1, CLC_BASE::CLCxIN0, CLC_BASE::CLCxIN1, CLC_BASE::PWM1,
        CLC_BASE::PWM2, CLC_BASE::NCOx, CLC_BASE::FOSCLK, CLC_BASE::LFINTOSC
    };

    P10F32X(const char *_name = nullptr, const char *desc = nullptr);
    P10F32X(const P10F32X &) = delete;
    P10F32X& operator = (const P10F32X &) = delete;
    ~P10F32X();

    virtual PIR *get_pir2() { return nullptr; }
    virtual PIR *get_pir1() { return pir1; }
    virtual PIR_SET *get_pir_set() { return &pir_set_def; }

    virtual void option_new_bits_6_7(unsigned int bits) override;
    virtual void create_symbols() override;
    virtual void create_sfr_map() override;
    virtual void create_iopin_map();
    virtual void create() override;
    virtual void create_config_memory() override;
    virtual bool set_config_word(unsigned int address, unsigned int cfg_word) override;
    virtual bool swdten_active() override
    {
        return wdt_flag == 1; // WDTCON can enable WDT
    }
    virtual void enter_sleep() override;
    virtual void exit_sleep() override;
    /*
      virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
    */
};


class P10F320 : public P10F32X
{
public:
    P10F320(const char *_name = nullptr, const char *desc = nullptr);
    ~P10F320();
    virtual PROCESSOR_TYPE isa() override
    {
        return _P10F320_;
    }
    static Processor *construct(const char *name);
    virtual unsigned int register_memory_size() const override
    {
        return 0x80;
    }
    virtual unsigned int program_memory_size() const override
    {
        return 0x100;
    }
    //RRRvirtual void create();
    virtual unsigned int get_device_id() override
    {
        return (0x29 << 8) | (0x5 << 5);
    }
};


class P10LF320 : public P10F320
{
public:
    P10LF320(const char *_name = nullptr, const char *desc = nullptr) : P10F320(_name, desc) {}
    ~P10LF320() {}
    virtual PROCESSOR_TYPE isa() override
    {
        return _P10LF320_;
    }
    static Processor *construct(const char *name);
    virtual unsigned int get_device_id() override
    {
        return (0x29 << 8) | (0x7 << 5);
    }
};


class P10F322 : public P10F32X
{
public:
    P10F322(const char *_name = nullptr, const char *desc = nullptr);
    ~P10F322();
    virtual PROCESSOR_TYPE isa() override
    {
        return _P10F322_;
    }
    static Processor *construct(const char *name);
    virtual unsigned int register_memory_size() const override
    {
        return 0x80;
    }
    virtual unsigned int program_memory_size() const override
    {
        return 0x200;
    }
    //RRRvirtual void create();
    virtual unsigned int get_device_id() override
    {
        return (0x29 << 8) | (0x4 << 5);
    }
};


class P10LF322 : public P10F322
{
public:
    P10LF322(const char *_name = nullptr, const char *desc = nullptr) : P10F322(_name, desc) {}
    ~P10LF322() {}
    virtual PROCESSOR_TYPE isa() override
    {
        return _P10LF322_;
    }
    static Processor *construct(const char *name);
    virtual unsigned int get_device_id() override
    {
        return (0x29 << 8) | (0x6 << 5);
    }
};


#endif
