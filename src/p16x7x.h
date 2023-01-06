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

#ifndef SRC_P16X7X_H_
#define SRC_P16X7X_H_


#include "14bit-processors.h"
#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "a2dconverter.h"
#include "intcon.h"
#include "ioports.h"
#include "p16x6x.h"
#include "p16x8x.h"
#include "pic-processor.h"
#include "pie.h"
#include "pir.h"
#include "pm_rd.h"
#include "registers.h"

class PicPortGRegister;
class PicPortRegister;
class PicTrisRegister;
class Processor;

//---------------------------------------------------------

class P16C71 :  public P16X8X
{
public:
    P16C71(const char *_name = nullptr, const char *desc = nullptr);
    ~P16C71();

    PROCESSOR_TYPE isa() override { return _P16C71_; }
    void create_symbols() override;

    unsigned int program_memory_size() const override { return 0x400; }
    void create_sfr_map() override;
    void create();
    static Processor *construct(const char *name);

    ADCON0 adcon0;
    ADCON1 adcon1;
    sfr_register  adres;

private:
    // This is not a real PIR register, but only one that allows the A2D Interrupt
    // flag be processed in manner similar to other processors.
    class PIR_16C71;
    PIR_16C71 *m_pir;
};


class P16x71x :  public _14bit_processor
{
public:
    P16x71x(const char *_name = nullptr, const char *desc = nullptr);
    ~P16x71x();

    virtual void create_iopin_map();
    void create_sfr_map() override;
    void option_new_bits_6_7(unsigned int bits) override;
    //RRRvoid create();
    void create_symbols() override;
    virtual PIR *get_pir1() { return pir1; }
    virtual PIR_SET *get_pir_set() { return &pir_set_def; }

    //  static Processor *construct(const char *name){;}

    // virtual bool hasSSP() { return false; }

    INTCON_14_PIR    intcon_reg;
    IOC		   *m_ioc;
    PicPortRegister  *m_porta;
    PicTrisRegister  *m_trisa;

    PicPortGRegister *m_portb;
    PicTrisRegister  *m_trisb;

    T1CON   	t1con;
    PIR    	*pir1;
    PIE     	pie1;
    T2CON   	t2con;
    PR2     	pr2;
    TMR2    	tmr2;
    TMRL    	tmr1l;
    TMRH    	tmr1h;
    CCPCON  	ccp1con;
    CCPRL   	ccpr1l;
    CCPRH   	ccpr1h;
    PCON    	pcon;
    PIR_SET_1 pir_set_def;
    ADCON0 	adcon0;
    ADCON1 	adcon1;
    sfr_register  adres;
    INT_pin	int_pin;
};


class P16C712 :  public P16x71x
{
public:
    P16C712(const char *_name = nullptr, const char *desc = nullptr);
    ~P16C712();

    PROCESSOR_TYPE isa() override { return _P16C712_; }
    unsigned int program_memory_size() const override { return 1024; }
    unsigned int register_memory_size() const override { return 0x100; }

    void create_sfr_map() override;
    void create() override;
    static Processor *construct(const char *name);

    bool hasSSP() override { return false; }

    TRISCCP trisccp;
    DATACCP dataccp;
};


class P16C716 :  public P16C712
{
public:
    P16C716(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P16C716_; }

    unsigned int program_memory_size() const override { return 0x800; }

    static Processor *construct(const char *name);
};


class P16F716 :  public P16C712
{
public:
    P16F716(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F716();

    PROCESSOR_TYPE isa() override { return _P16F716_; }

    unsigned int program_memory_size() const override { return 0x800; }

    static Processor *construct(const char *name);
    void create_sfr_map() override;
    void create() override;

    ECCPAS        eccpas;
    PWM1CON       pwm1con;
};


class P16C72 : public P16C62
{
public:
    P16C72(const char *_name = nullptr, const char *desc = nullptr);
    ~P16C72();

    PROCESSOR_TYPE isa() override
    {
        return _P16C72_;
    }
    void create_symbols() override;
    void create_sfr_map() override;
    PIR *get_pir1() override { return pir1_2_reg; }
    PIR *get_pir2() override { return pir2_2_reg; }
    PIR_SET *get_pir_set() override { return &pir_set_2_def; }

    void create() override;
    static Processor *construct(const char *name);

    // XXX
    // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
    // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
    // PIR2v2, but C++ only supports covariance in member function return
    // values.
    PIR1v2 *pir1_2_reg;
    PIR2v2 *pir2_2_reg;
    PIR_SET_2 pir_set_2_def;
    ADCON0 adcon0;
    ADCON1 adcon1;
    sfr_register  adres;
};


class P16C73 : public P16C63
{
public:
    P16C73(const char *_name = nullptr, const char *desc = nullptr);
    ~P16C73();

    PROCESSOR_TYPE isa() override { return _P16C73_; }
    void create_symbols() override;
    void create_sfr_map() override;
    PIR *get_pir1() override { return pir1_2_reg; }
    PIR *get_pir2() override { return pir2_2_reg; }
    PIR_SET *get_pir_set() override { return &pir_set_2_def; }

    void create() override;
    static Processor *construct(const char *name);

    // XXX
    // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
    // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
    // PIR2v2, but C++ only supports covariance in member function return
    // values.
    PIR1v2 *pir1_2_reg;
    PIR2v2 *pir2_2_reg;
    PIR_SET_2 pir_set_2_def;
    ADCON0 adcon0;
    ADCON1 adcon1;
    sfr_register  adres;
};


class P16F73 : public P16C73
{
public:
    P16F73(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F73();

    PROCESSOR_TYPE isa() override { return _P16F73_; }
    unsigned int register_memory_size() const override { return 0x200; }
    void create_symbols() override;
    void create_sfr_map() override;

    void create() override;
    static Processor *construct(const char *name);

protected:
    PM_RD pm_rd;
};


//---------------------------------------------------------

class P16C74 : public P16C65   // Not a typo, a 'c74 is more like a 'c65 then a 'c64!
{
public:
    P16C74(const char *_name = nullptr, const char *desc = nullptr);
    ~P16C74();

    PROCESSOR_TYPE isa() override { return _P16C74_; }
    void create_symbols() override;
    void create_sfr_map() override;
    PIR *get_pir1() override { return pir1_2_reg; }
    PIR *get_pir2() override { return pir2_2_reg; }
    PIR_SET *get_pir_set() override { return &pir_set_2_def; }

    unsigned int program_memory_size() const override { return 0x1000; }

    void create() override;
    static Processor *construct(const char *name);

    // XXX
    // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
    // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
    // PIR2v2, but C++ only supports covariance in member function return
    // values.
    PIR1v2 *pir1_2_reg;
    PIR2v2 *pir2_2_reg;
    PIR_SET_2 pir_set_2_def;
    ADCON0 adcon0;
    ADCON1 adcon1;
    sfr_register  adres;
};

class P16F74 : public P16C74
{
public:
    P16F74(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F74();

    PROCESSOR_TYPE isa() override { return _P16F74_; }
    unsigned int register_memory_size() const override { return 0x200; }
    void create_symbols() override;
    void create_sfr_map() override;
    void create() override;
    static Processor *construct(const char *name);

protected:
    PM_RD pm_rd;
};


#endif

