/*
   Copyright (C) 2010,2015	   Roy Rankin

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

#ifndef SRC_P16F88X_H_
#define SRC_P16F88X_H_

#include <assert.h>

#include "14bit-processors.h"
#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "a2dconverter.h"
#include "comparator.h"
#include "eeprom.h"
#include "intcon.h"
#include "pic-processor.h"
#include "pie.h"
#include "pir.h"
#include "registers.h"
#include "ssp.h"
#include "uart.h"

class PicPSP_PortRegister;
class PicPSP_TrisRegister;
class PicPortGRegister;
class PicPortRegister;
class PicTrisRegister;
class Processor;


/***************************************************************************
 *
 * Include file for:  P16F887, P16F88
 *
 *
 *
 ***************************************************************************/

class P16F88x : public _14bit_processor
{
public:
    INTCON_14_PIR    intcon_reg;
    INT_pin	     int_pin;

    PicPortRegister  *m_porta;
    PicTrisRegister  *m_trisa;

    PicPortGRegister *m_portb;
    PicTrisRegister  *m_trisb;
    WPU              *m_wpu;
    IOC              *m_ioc;

    PicPortRegister  *m_portc;
    PicTrisRegister  *m_trisc;

    PicPSP_PortRegister  *m_portd;
    PicTrisRegister  *m_trisd;

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
    CCPRL   ccpr1l;
    CCPRH   ccpr1h;
    CCPRL   ccpr2l;
    CCPRH   ccpr2h;
    PCON    pcon;
    SSP_MODULE   ssp;
    PIR1v2 *pir1_2_reg;
    PIR2v3 *pir2_2_reg;
    PIR_SET_2 pir_set_2_def;

    OSCCON       *osccon;
    OSCTUNE5     osctune;          // with 5-bit trim, no PLLEN
    WDTCON       wdtcon;
    USART_MODULE usart;
    ComparatorModule2 comparator;
    VRCON   vrcon;
    SR_MODULE sr_module;
    ANSEL  ansel;
    ANSEL_H  anselh;
    ADCON0 adcon0;
    ADCON1 adcon1;
    ECCPAS	eccpas;
    PWM1CON	pwm1con;
    PSTRCON	pstrcon;


    sfr_register  adresh;
    sfr_register  adresl;

    CCPCON  *m_ccp1con;
    CCPCON  *m_ccp2con;
    PicPortRegister  *m_porte;
    PicPSP_TrisRegister  *m_trise;


    P16F88x(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F88x();

    virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

    //  virtual PROCESSOR_TYPE isa(){return _P16F88x_;};
    virtual void create_symbols();
    virtual unsigned int register_memory_size() const
    {
        return 0x200;
    }

    virtual unsigned int program_memory_size()
    {
        return 0;
    }
    virtual void option_new_bits_6_7(unsigned int bits);

    virtual void create_sfr_map();

    // The f628 (at least) I/O pins depend on the Fosc Configuration bits.
    virtual bool set_config_word(unsigned int address, unsigned int cfg_word);


    virtual void create(int eesize);
    virtual void create_iopin_map();
    virtual void create_config_memory();

    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
        assert(0);
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual EEPROM_WIDE *get_eeprom()
    {
        return (EEPROM_WIDE *)eeprom;
    }


    virtual PIR *get_pir1()
    {
        return pir1;
    }
    virtual PIR *get_pir2()
    {
        return pir2;
    }
    virtual PIR_SET *get_pir_set()
    {
        return &pir_set_2_def;
    }
};


class P16F884 : public P16F88x
{
public:
    P16F884(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F884();

    static Processor *construct(const char *name);
    virtual void create_symbols();
    virtual void create_sfr_map();
    virtual void create_iopin_map();
    virtual PROCESSOR_TYPE isa()
    {
        return _P16F884_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 4096;
    }

};


class P16F887 : public P16F884
{
public:
    P16F887(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F887();

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F887_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 8192;
    }
    static Processor *construct(const char *name);
    virtual void create_symbols();
    virtual void create_sfr_map();
};


class P16F882 : public P16F88x
{
public:
    P16F882(const char *_name = nullptr, const char *desc = nullptr);

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F882_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 2048;
    }

    static Processor *construct(const char *name);
    virtual void create_symbols();
    virtual void create_sfr_map();
    virtual void create_iopin_map();
};


class P16F883 : public P16F882
{
public:
    P16F883(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F883();

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F883_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 4096;
    }

    static Processor *construct(const char *name);
    virtual void create_symbols();
    virtual void create_sfr_map();
};


class P16F886 : public P16F882
{
public:
    P16F886(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F886();

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F886_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 8192;
    }

    static Processor *construct(const char *name);
    virtual void create_symbols();
    virtual void create_sfr_map();
};


class P16F631 :  public _14bit_processor
{
public:
    P16F631(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P16F631();

    T1CON   t1con;
    PIR    *pir1;
    PIR    *pir2;
    PIE     pie1;
    PIE     pie2;
    TMRL    tmr1l;
    TMRH    tmr1h;
    OSCTUNE5 osctune;          // with 5-bit trim, no PLLEN
    PCON    pcon;
    WDTCON  wdtcon;
    OSCCON  *osccon;
    VRCON_2   vrcon;
    SRCON   srcon;
    ANSEL  ansel;
    ComparatorModule2 comparator;
    ADCON0_12F  adcon0;
    ADCON1_16F  adcon1;

    EEPROM_WIDE *e;
    PIR1v2 *pir1_2_reg;
    PIR2v3 *pir2_3_reg;

    INTCON_14_PIR    intcon_reg;
    PIR_SET_2    pir_set_2_def;
    WPU              *m_wpua;
    WPU              *m_wpub;
    IOC              *m_ioca;
    IOC              *m_iocb;
    INT_pin	   int_pin;

    virtual PIR *get_pir2()
    {
        return pir2;
    }
    virtual PIR *get_pir1()
    {
        return pir1;
    }
    virtual PIR_SET *get_pir_set()
    {
        return &pir_set_2_def;
    }

    PicPortGRegister  	*m_porta;
    PicTrisRegister  	*m_trisa;

    PicPortGRegister  	*m_portb;
    PicTrisRegister  	*m_trisb;

    PicPortRegister 	*m_portc;
    PicTrisRegister  	*m_trisc;

    a2d_stimulus 		*m_cvref;
    a2d_stimulus 		*m_v06ref;

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F631_;
    }
    static Processor *construct(const char *name);
    void create(int);
    virtual void create_symbols();
    virtual void create_sfr_map();
    virtual void create_iopin_map();
    virtual void option_new_bits_6_7(unsigned int bits);

    virtual unsigned int program_memory_size() const
    {
        return 0x400;
    }
    virtual unsigned int register_memory_size() const
    {
        return 0x200;
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual void create_config_memory();
    virtual bool set_config_word(unsigned int address, unsigned int cfg_word);
};


class P16F677 : public P16F631
{
public:
    P16F677(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F677();

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F677_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 4096;
    }
    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
        assert(0);
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual EEPROM_WIDE *get_eeprom()
    {
        return (EEPROM_WIDE *)eeprom;
    }

    static Processor *construct(const char *name);

    SSP_MODULE   ssp;

    ANSEL_H  anselh;
    sfr_register  adresh;
    sfr_register  adresl;
    virtual void create_symbols();
    virtual void create_sfr_map();
};


class P16F687 : public P16F677
{
public:
    P16F687(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F687();

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F687_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 2048;
    }
    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
        assert(0);
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual EEPROM_WIDE *get_eeprom()
    {
        return (EEPROM_WIDE *)eeprom;
    }

    static Processor *construct(const char *name);

    TMRL    tmr1l;
    TMRH    tmr1h;
    PCON    pcon;
    USART_MODULE usart;

    virtual void create_symbols();
    virtual void create_sfr_map();
};


class P16F684 : public  _14bit_processor
{
public:
    P16F684(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~P16F684();

    ComparatorModule comparator;
    virtual PROCESSOR_TYPE isa()
    {
        return _P16F684_;
    }
    virtual unsigned int program_memory_size() const
    {
        return 2048;
    }
    virtual unsigned int register_memory_size() const
    {
        return 0x100;
    }
    virtual void create(int eesize);
    virtual void create_iopin_map();

    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
        assert(0);
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual EEPROM_WIDE *get_eeprom()
    {
        return (EEPROM_WIDE *)eeprom;
    }

    virtual void option_new_bits_6_7(unsigned int bits);

    virtual void create_config_memory();
    virtual bool set_config_word(unsigned int, unsigned int);

    static Processor *construct(const char *name);
    PicPortGRegister  *m_porta;
    PicTrisRegister  *m_trisa;

    PicPortRegister *m_portc;
    PicTrisRegister  *m_trisc;

    WPU              *m_wpua;
    IOC              *m_ioca;

    T1CON   t1con;
    T2CON   t2con;
    PIR1v3    *pir1;
    PIE     pie1;
    PR2     pr2;
    TMR2    tmr2;
    TMRL    tmr1l;
    TMRH    tmr1h;
    OSCTUNE5 osctune;          // with 5-bit trim, no PLLEN
    PCON    pcon;
    WDTCON  wdtcon;
    OSCCON  *osccon;
    ANSEL  ansel;
    ADCON0_12F  adcon0;
    ADCON1_16F  adcon1;
    sfr_register  adresh;
    sfr_register  adresl;

    CCPCON  ccp1con;
    CCPRL   ccpr1l;
    CCPRH   ccpr1h;
    ECCPAS	eccpas;
    PWM1CON	pwm1con;
    PSTRCON	pstrcon;
    PIR1v3 	*pir1_3_reg;
    INTCON_14_PIR    intcon_reg;
    INT_pin	int_pin;
    PIR_SET_1     pir_set_def;
    IOC              *m_iocc;
    EEPROM_WIDE *e;

    virtual PIR_SET *get_pir_set()
    {
        return &pir_set_def;
    }
    virtual void create_symbols();
    virtual void create_sfr_map();
};


class P16F685 : public P16F677
{
public:
    P16F685(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F685();

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F685_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 4096;
    }
    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
        assert(0);
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual EEPROM_WIDE *get_eeprom()
    {
        return ((EEPROM_WIDE *)eeprom);
    }

    static Processor *construct(const char *name);
    T2CON   t2con;
    PR2     pr2;
    TMR2    tmr2;
    TMRL    tmr1l;
    TMRH    tmr1h;
    CCPCON  ccp1con;
    CCPRL   ccpr1l;
    CCPRH   ccpr1h;
    PCON    pcon;
    ECCPAS	eccpas;
    PWM1CON	pwm1con;
    PSTRCON	pstrcon;

    virtual void create_symbols();
    virtual void create_sfr_map();
};


class P16F689 : public P16F687
{
public:
    P16F689(const char *_name = nullptr, const char *desc = nullptr);

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F689_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 4096;
    }
    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
        assert(0);
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual EEPROM_WIDE *get_eeprom()
    {
        return (EEPROM_WIDE *)eeprom;
    }

    static Processor *construct(const char *name);
};


class P16F690 : public P16F685
{
public:
    P16F690(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F690();

    virtual PROCESSOR_TYPE isa()
    {
        return _P16F690_;
    }

    virtual unsigned int program_memory_size() const
    {
        return 4096;
    }
    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
        assert(0);
    }

    virtual void set_eeprom_wide(EEPROM_WIDE *ep)
    {
        eeprom = ep;
    }
    virtual EEPROM_WIDE *get_eeprom()
    {
        return (EEPROM_WIDE *)eeprom;
    }

    static Processor *construct(const char *name);
    CCPCON  ccp2con;
    CCPRL   ccpr2l;
    CCPRH   ccpr2h;

    USART_MODULE usart;

    virtual void create_symbols();
    virtual void create_sfr_map();
};


#endif
