/*
   Copyright (C) 2013,2014,2017,2020 Roy R. Rankin

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
// p1xf1xxx
//
//  This file supports:
//    PIC12[L]F1822
//    PIC12[L]F1840
//    PIC16[L]F1303
//    PIC16[L]F1823
//    PIC16[L]F1825
//    PIC16[L]F1788
//
//Note: All these  processors have extended 14bit instructions

#include <stdio.h>
#include <string>

#include "stimuli.h"
#include "eeprom.h"
#include "intcon.h"
#include "packages.h"
#include "pic-ioports.h"
#include "pic-registers.h"
#include "tmr0.h"
#include "trace.h"

#include "p1xf1xxx.h"

class Processor;

//#define DEBUG
#if defined(DEBUG)
#include <config.h>
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif



// Does not match any of 3 versions in pir.h, pir.cc
// If required by any other processors should be moved there
//
class PIR1v1822 : public PIR1v2
{
public:
    // rest of bits defined in PIR1v2
    enum
    {
        TMR1GIF = 1 << 7
    };

    //------------------------------------------------------------------------

    PIR1v1822(Processor *pCpu, const char *pName, const char *pDesc, INTCON *_intcon, PIE *_pie)
        : PIR1v2(pCpu, pName, pDesc, _intcon, _pie)
    {
        valid_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | TXIF | RCIF | ADIF | TMR1GIF;
        writable_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | ADIF | TMR1GIF;
    }

    void set_tmr1gif() override
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | TMR1GIF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }
};


class PIR2v1822 : public PIR
{
public:
    enum
    {
        CCP2IF  = 1 << 0,	// for 16f178[89]
        C3IF    = 1 << 1,	// for 16f178[89]
        C4IF    = 1 << 2,	// for 16f178[89]
        BCLIF   = 1 << 3,
        EEIF    = 1 << 4,
        C1IF    = 1 << 5,
        C2IF    = 1 << 6, // not 12f1822
        OSFIF   = 1 << 7
    };

    //------------------------------------------------------------------------

    PIR2v1822(Processor *pCpu, const char *pName, const char *pDesc, INTCON *_intcon, PIE *_pie)
        : PIR(pCpu, pName, pDesc, _intcon, _pie, 0)
    {
        valid_bits = BCLIF | EEIF | C1IF | OSFIF;
        writable_bits = BCLIF | EEIF | C1IF | OSFIF;
    }

    void set_ccp2if()
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | CCP2IF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }

    void set_c3if() override
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | C3IF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }

    void set_c4if() override
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | C4IF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }

    void set_bclif() override
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | BCLIF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }
    void set_eeif() override
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | EEIF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }
    void set_c1if() override
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | C1IF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }
    void set_c2if() override
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | C2IF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }
    void set_osfif()
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | OSFIF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }
};


class PIR3v178x : public PIR
{
public:
    enum
    {
        CCP3IF = 1 << 4,	// PIR3
        PSMC4TIF = 1 << 7,
        PSMC3TIF = 1 << 6,
        PSMC2TIF = 1 << 5,
        PSMC1TIF = 1 << 4,
        PSMC4SIF = 1 << 3,
        PSMC3SIF = 1 << 2,
        PSMC2SIF = 1 << 1,
        PSMC1SIF = 1 << 0,
    };

    PIR3v178x(Processor *pCpu, const char *pName, const char *pDesc, INTCON *_intcon, PIE *_pie)
        : PIR(pCpu, pName, pDesc, _intcon, _pie, 0)
    {
        writable_bits = valid_bits = CCP3IF;
    }

    void set_ccp3if()
    {
        trace.raw(write_trace.get() | value.get());
        value.put(value.get() | CCP3IF);

        if (value.get() & pie->value.get())
        {
            setPeripheralInterrupt();
        }
    }
};






//========================================================================
//                  12F1822 & 12F1840
//========================================================================

P12F1822::P12F1822(const char *_name, const char *desc)
    : _14bit_e_processor(_name, desc),
      comparator(this),
      pie1(this, "pie1", "Peripheral Interrupt Enable"),
      pie2(this, "pie2", "Peripheral Interrupt Enable"),
      t2con(this, "t2con", "TMR2 Control"),
      pr2(this, "pr2", "TMR2 Period Register"),
      tmr2(this, "tmr2", "TMR2 Register"),
      t1con_g(this, "t1con", "TMR1 Control Register"),
      tmr1l(this, "tmr1l", "TMR1 Low"),
      tmr1h(this, "tmr1h", "TMR1 High"),
      ccp1con(this, "ccp1con", "Capture Compare Control"),
      ccpr1l(this, "ccpr1l", "Capture Compare 1 Low"),
      ccpr1h(this, "ccpr1h", "Capture Compare 1 High"),
      fvrcon(this, "fvrcon", "Voltage reference control register", 0xbf),
      borcon(this, "borcon", "Brown-out reset control register"),
      ansela(this, "ansela", "Analog Select"),
      adcon0(this, "adcon0", "A2D Control 0"),
      adcon1(this, "adcon1", "A2D Control 1"),
      adresh(this, "adresh", "A2D Result High"),
      adresl(this, "adresl", "A2D Result Low"),
      osccon(0),
      osctune(this, "osctune", "Oscillator Tunning Register"),
      oscstat(this, "oscstat", "Oscillator Status Register"),
      wdtcon(this, "wdtcon", "Watch dog timer control", 0x3f),
      usart(this),
      ssp(this),
      apfcon(this, "apfcon", "Alternate Pin Function Control Register", 0xef),
      pwm1con(this, "pwm1con", "Enhanced PWM Control Register"),
      ccp1as(this, "ccp1as", "CCP1 Auto-Shutdown Control Register"),
      pstr1con(this, "pstr1con", "Pulse Sterring Control Register"),
      cpscon0(this, "cpscon0", " Capacitive Sensing Control Register 0"),
      cpscon1(this, "cpscon1", " Capacitive Sensing Control Register 1"),
      sr_module(this), dsm_module(this)
{
    m_iocaf = new IOCxF(this, "iocaf", "Interrupt-On-Change flag Register", 0x3f);
    m_iocap = new IOC(this, "iocap", "Interrupt-On-Change positive edge", 0x3f);
    m_iocan = new IOC(this, "iocan", "Interrupt-On-Change negative edge", 0x3f);
    m_porta = new PicPortIOCRegister(this, "porta", "", intcon, m_iocap, m_iocan, m_iocaf, 8, 0x3f);
    m_trisa = new PicTrisRegister(this, "trisa", "", m_porta, false, 0x37);
    m_lata  = new PicLatchRegister(this, "lata", "", m_porta, 0x37);
    m_daccon0 = new DACCON0(this, "daccon0", "DAC Voltage reference register 0", 0xec, 32);
    m_daccon1 = new DACCON1(this, "daccon1", "DAC Voltage reference register 1", 0x1f, m_daccon0);
    m_cpu_temp = new CPU_Temp("cpu_temperature", 30., "CPU die temperature");
    tmr0.link_cpu(this, m_porta, 4, option_reg);
    tmr0.start(0);
    tmr0.set_t1gcon(&t1con_g.t1gcon);
    cpscon1.m_cpscon0 = &cpscon0;
    cpscon0.m_tmr0 = &tmr0;
    cpscon0.m_t1con_g = &t1con_g;
    ((INTCON_14_PIR *)intcon)->write_mask = 0xfe;
    m_wpua = new WPU(this, "wpua", "Weak Pull-up Register", m_porta, 0x3f);
    pir1 = new PIR1v1822(this, "pir1", "Peripheral Interrupt Register", intcon, &pie1);
    pir2 = new PIR2v1822(this, "pir2", "Peripheral Interrupt Register", intcon, &pie2);
    comparator.cmxcon0[0] = new CMxCON0(this, "cm1con0", " Comparator C1 Control Register 0", 0, &comparator);
    comparator.cmxcon1[0] = new CMxCON1(this, "cm1con1", " Comparator C1 Control Register 1", 0, &comparator);
    comparator.cmout = new CMOUT(this, "cmout", "Comparator Output Register");
}


P12F1822::~P12F1822()
{
    adcon1.detach_fvr();
    adcon1.detach_DAC();
    comparator.detach_fvr();
    cpscon0.detach_fvr();
    cpscon0.detach_DAC();
    m_daccon0->detach_fvr();
    unassignMCLRPin();
    delete_file_registers(0x20, 0x7f);
    delete_file_registers(0xa0, 0xbf);
    delete_sfr_register(m_iocap);
    delete_sfr_register(m_iocan);
    delete_sfr_register(m_iocaf);
    delete_sfr_register(m_daccon0);
    delete_sfr_register(m_daccon1);
    delete_sfr_register(m_trisa);
    delete_sfr_register(m_porta);
    delete_sfr_register(m_lata);
    delete_sfr_register(m_wpua);
    remove_sfr_register(&tmr0);
    remove_sfr_register(&tmr1l);
    remove_sfr_register(&tmr1h);
    remove_sfr_register(&t1con_g);
    remove_sfr_register(&t1con_g.t1gcon);
    remove_sfr_register(&tmr2);
    remove_sfr_register(&pr2);
    remove_sfr_register(&t2con);
    remove_sfr_register(&cpscon0);
    remove_sfr_register(&cpscon1);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&ccpr1l);
    remove_sfr_register(&ccpr1h);
    remove_sfr_register(&ccp1con);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&ccp1as);
    remove_sfr_register(&pstr1con);
    remove_sfr_register(&pie1);
    remove_sfr_register(&pie2);
    remove_sfr_register(&adresl);
    remove_sfr_register(&adresh);
    remove_sfr_register(&adcon0);
    remove_sfr_register(&adcon1);
    remove_sfr_register(&borcon);
    remove_sfr_register(&fvrcon);
    remove_sfr_register(sr_module.srcon0);
    remove_sfr_register(sr_module.srcon1);
    remove_sfr_register(&apfcon);
    remove_sfr_register(&ansela);
    remove_sfr_register(get_eeprom()->get_reg_eeadr());
    remove_sfr_register(get_eeprom()->get_reg_eeadrh());
    remove_sfr_register(get_eeprom()->get_reg_eedata());
    remove_sfr_register(get_eeprom()->get_reg_eedatah());
    remove_sfr_register(get_eeprom()->get_reg_eecon1());
    remove_sfr_register(get_eeprom()->get_reg_eecon2());
    remove_sfr_register(&usart.spbrg);
    remove_sfr_register(&usart.spbrgh);
    remove_sfr_register(&usart.rcsta);
    remove_sfr_register(&usart.txsta);
    remove_sfr_register(&usart.baudcon);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&ccpr1l);
    remove_sfr_register(&ccpr1h);
    remove_sfr_register(&ccp1con);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&ccp1as);
    remove_sfr_register(&pstr1con);
    remove_sfr_register(&osctune);
    remove_sfr_register(option_reg);
    remove_sfr_register(osccon);
    remove_sfr_register(&oscstat);
    remove_sfr_register(comparator.cmxcon0[0]);
    remove_sfr_register(comparator.cmxcon1[0]);
    remove_sfr_register(comparator.cmout);
    delete_sfr_register(usart.rcreg);
    delete_sfr_register(usart.txreg);
    delete_sfr_register(pir1);
    delete_sfr_register(pir2);
    remove_sfr_register(&dsm_module.mdcon);
    remove_sfr_register(&dsm_module.mdsrc);
    remove_sfr_register(&dsm_module.mdcarl);
    remove_sfr_register(&dsm_module.mdcarh);
    delete e;
    delete m_cpu_temp;
    delete osccon;
}


Processor * P12F1822::construct(const char *name)
{
    P12F1822 *p = new P12F1822(name);
    p->create(0x7f, 256, 0x2700);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


void P12F1822::create_symbols()
{
    pic_processor::create_symbols();
    addSymbol(Wreg);
    addSymbol(m_cpu_temp);
}


void P12F1822::create_sfr_map()
{
    pir_set_2_def.set_pir1(pir1);
    pir_set_2_def.set_pir2(pir2);
    //add_sfr_register(indf,    0x00);
    add_file_registers(0xa0, 0xbf, 0x00);
    add_sfr_register(m_porta, 0x0c);
    add_sfr_registerR(pir1,    0x11, RegisterValue(0, 0), "pir1");
    add_sfr_registerR(pir2,    0x12, RegisterValue(0, 0), "pir2");
    add_sfr_register(&tmr0,   0x15);
    add_sfr_register(&tmr1l,  0x16, RegisterValue(0, 0), "tmr1l");
    add_sfr_register(&tmr1h,  0x17, RegisterValue(0, 0), "tmr1h");
    add_sfr_register(&t1con_g,  0x18, RegisterValue(0, 0));
    add_sfr_register(&t1con_g.t1gcon, 0x19, RegisterValue(0, 0));
    add_sfr_register(&tmr2,   0x1a, RegisterValue(0, 0));
    add_sfr_register(&pr2,    0x1b, RegisterValue(0xff, 0));
    add_sfr_register(&t2con,  0x1c, RegisterValue(0, 0));
    add_sfr_register(&cpscon0,  0x1e, RegisterValue(0, 0), "cpscon0");
    add_sfr_register(&cpscon1,  0x1f, RegisterValue(0, 0));
    add_sfr_register(m_trisa, 0x8c, RegisterValue(0x3f, 0));
    pcon.valid_bits = 0xcf;
    add_sfr_register(option_reg, 0x95, RegisterValue(0xff, 0));
    add_sfr_register(&osctune,    0x98, RegisterValue(0, 0));
    add_sfr_register(osccon,     0x99, RegisterValue(0x38, 0));
    add_sfr_register(&oscstat,    0x9a, RegisterValue(0, 0));
    intcon_reg.set_pir_set(get_pir_set());
    tmr1l.tmrh = &tmr1h;
    tmr1l.t1con = &t1con_g;
    tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v1::TMR1IF));
    tmr1h.tmrl  = &tmr1l;
    t1con_g.tmrl  = &tmr1l;
    t1con_g.t1gcon.set_tmrl(&tmr1l);
    t1con_g.t1gcon.setInterruptSource(new InterruptSource(pir1, PIR1v1822::TMR1GIF));
    tmr1l.setIOpin(&(*m_porta)[5]);
    t1con_g.t1gcon.setGatepin(&(*m_porta)[3]);
    add_sfr_register(&pie1,   0x91, RegisterValue(0, 0));
    add_sfr_register(&pie2,   0x92, RegisterValue(0, 0));
    add_sfr_register(&adresl, 0x9b);
    add_sfr_register(&adresh, 0x9c);
    add_sfr_register(&adcon0, 0x9d, RegisterValue(0x00, 0));
    add_sfr_register(&adcon1, 0x9e, RegisterValue(0x00, 0));
    usart.initialize(pir1,
                     &(*m_porta)[0], // TX pin
                     & (*m_porta)[1], // RX pin
                     new _TXREG(this, "txreg", "USART Transmit Register", &usart),
                     new _RCREG(this, "rcreg", "USART Receiver Register", &usart));
    usart.set_eusart(true);
    add_sfr_register(m_lata,    0x10c);
    add_sfr_register(comparator.cmxcon0[0], 0x111, RegisterValue(0x04, 0));
    add_sfr_register(comparator.cmxcon1[0], 0x112, RegisterValue(0x00, 0));
    add_sfr_register(comparator.cmout,      0x115, RegisterValue(0x00, 0));
    add_sfr_register(&borcon,   0x116, RegisterValue(0x80, 0));
    add_sfr_register(&fvrcon,   0x117, RegisterValue(0x00, 0));
    add_sfr_register(m_daccon0, 0x118, RegisterValue(0x00, 0));
    add_sfr_register(m_daccon1, 0x119, RegisterValue(0x00, 0));
    sr_module.srcon0 = new SRCON0(this, "srcon0", "SR Latch Control 0 Register", &sr_module);
    sr_module.srcon1 = new SRCON1(this, "srcon1", "SR Latch Control 1 Register", &sr_module);
    add_sfr_register(sr_module.srcon0, 0x11a, RegisterValue(0x00, 0));
    add_sfr_register(sr_module.srcon1, 0x11b, RegisterValue(0x00, 0));
    add_sfr_register(&apfcon,  0x11d, RegisterValue(0x00, 0));
    add_sfr_register(&ansela,   0x18c, RegisterValue(0x17, 0));
    add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x191);
    add_sfr_register(get_eeprom()->get_reg_eeadrh(),   0x192);
    get_eeprom()->get_reg_eedata()->new_name("eedatl");
    get_eeprom()->get_reg_eedatah()->new_name("eedath");
    add_sfr_register(get_eeprom()->get_reg_eedata(),  0x193);
    add_sfr_register(get_eeprom()->get_reg_eedatah(),  0x194);
    add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x195, RegisterValue(0x00, 0));
    add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x196);
    add_sfr_register(usart.rcreg,    0x199, RegisterValue(0, 0), "rcreg");
    add_sfr_register(usart.txreg,    0x19a, RegisterValue(0, 0), "txreg");
    add_sfr_register(&usart.spbrg,   0x19b, RegisterValue(0, 0), "spbrgl");
    add_sfr_register(&usart.spbrgh,  0x19c, RegisterValue(0, 0), "spbrgh");
    add_sfr_register(&usart.rcsta,   0x19d, RegisterValue(0, 0), "rcsta");
    add_sfr_register(&usart.txsta,   0x19e, RegisterValue(2, 0), "txsta");
    add_sfr_register(&usart.baudcon, 0x19f, RegisterValue(0x40, 0), "baudcon");
    add_sfr_register(m_wpua,       0x20c, RegisterValue(0x3f, 0), "wpua");
    add_sfr_register(&ssp.sspbuf,  0x211, RegisterValue(0, 0), "ssp1buf");
    add_sfr_register(&ssp.sspadd,  0x212, RegisterValue(0, 0), "ssp1add");
    add_sfr_register(ssp.sspmsk, 0x213, RegisterValue(0xff, 0), "ssp1msk");
    add_sfr_register(&ssp.sspstat, 0x214, RegisterValue(0, 0), "ssp1stat");
    add_sfr_register(&ssp.sspcon,  0x215, RegisterValue(0, 0), "ssp1con");
    add_sfr_register(&ssp.sspcon2, 0x216, RegisterValue(0, 0), "ssp1con2");
    add_sfr_register(&ssp.ssp1con3, 0x217, RegisterValue(0, 0), "ssp1con3");
    add_sfr_register(&ccpr1l,      0x291, RegisterValue(0, 0));
    add_sfr_register(&ccpr1h,      0x292, RegisterValue(0, 0));
    add_sfr_register(&ccp1con,     0x293, RegisterValue(0, 0));
    add_sfr_register(&pwm1con,     0x294, RegisterValue(0, 0));
    add_sfr_register(&ccp1as,      0x295, RegisterValue(0, 0));
    add_sfr_register(&pstr1con,    0x296, RegisterValue(1, 0));
    add_sfr_register(m_iocap, 0x391, RegisterValue(0, 0), "iocap");
    add_sfr_register(m_iocan, 0x392, RegisterValue(0, 0), "iocan");
    add_sfr_register(m_iocaf, 0x393, RegisterValue(0, 0), "iocaf");
    m_iocaf->set_intcon(intcon);
    add_sfr_register(&dsm_module.mdcon, 0x39c, RegisterValue(0x20, 0));
    add_sfr_register(&dsm_module.mdsrc, 0x39d, RegisterValue(0x00, 0));
    add_sfr_register(&dsm_module.mdcarl, 0x39e, RegisterValue(0x00, 0));
    add_sfr_register(&dsm_module.mdcarh, 0x39f, RegisterValue(0x00, 0));
    tmr2.ssp_module[0] = &ssp;
    ssp.initialize(
        get_pir_set(),    // PIR
        & (*m_porta)[1],  // SCK
        & (*m_porta)[3],  // SS
        & (*m_porta)[0],  // SDO
        & (*m_porta)[2],   // SDI
        m_trisa,          // i2c tris port
        SSP_TYPE_MSSP1
    );
    apfcon.set_pins(0, &ccp1con, CCPCON::CCP_PIN, &(*m_porta)[2], &(*m_porta)[5]); //CCP1/P1A
    apfcon.set_pins(1, &ccp1con, CCPCON::PxB_PIN, &(*m_porta)[0], &(*m_porta)[4]); //P1B
    apfcon.set_pins(2, &usart, USART_MODULE::TX_PIN, &(*m_porta)[0], &(*m_porta)[4]); //USART TX Pin
    apfcon.set_pins(3, &t1con_g.t1gcon, 0, &(*m_porta)[4], &(*m_porta)[3]); //tmr1 gate
    apfcon.set_pins(5, &ssp, SSP1_MODULE::SS_PIN, &(*m_porta)[3], &(*m_porta)[0]); //SSP SS
    apfcon.set_pins(6, &ssp, SSP1_MODULE::SDO_PIN, &(*m_porta)[0], &(*m_porta)[4]); //SSP SDO
    apfcon.set_pins(7, &usart, USART_MODULE::RX_PIN, &(*m_porta)[1], &(*m_porta)[5]); //USART RX Pin

    if (pir1)
    {
        pir1->set_intcon(intcon);
        pir1->set_pie(&pie1);
    }

    pie1.setPir(pir1);
    pie2.setPir(pir2);
    t2con.tmr2 = &tmr2;
    tmr2.pir_set   = get_pir_set();
    tmr2.pr2    = &pr2;
    tmr2.t2con  = &t2con;
    tmr2.add_ccp(&ccp1con);
    //  tmr2.add_ccp ( &ccp2con );
    pr2.tmr2    = &tmr2;
    ccp1as.setIOpin(0, 0, &(*m_porta)[2]);
    ccp1as.link_registers(&pwm1con, &ccp1con);
    ccp1con.setIOpin(&(*m_porta)[2], &(*m_porta)[0]);
    ccp1con.pstrcon = &pstr1con;
    ccp1con.pwm1con = &pwm1con;
    ccp1con.setCrosslinks(&ccpr1l, pir1, PIR1v1822::CCP1IF, &tmr2, &ccp1as);
    ccpr1l.ccprh  = &ccpr1h;
    ccpr1l.tmrl   = &tmr1l;
    ccpr1h.ccprl  = &ccpr1l;
    ansela.config(0x17, 0);
    ansela.setValidBits(0x17);
    ansela.setAdcon1(&adcon1);
    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    adcon0.setIntcon(intcon);
    adcon0.setA2DBits(10);
    adcon0.setPir(pir1);
    adcon0.setChannel_Mask(0x1f);
    adcon0.setChannel_shift(2);
    adcon0.setGo(1);
    adcon1.set_FVR_chan(0x1f);
    adcon1.setAdcon0(&adcon0);
    adcon1.attach_ad_fvr(fvrcon.get_node_adcvref(), 0x1f);
    adcon1.attach_Vt_fvr(fvrcon.get_node_Vtref(), 0x1d);
    adcon1.attach_DAC(m_daccon0->get_node_dacout(), 0x1e);
    adcon1.setNumberOfChannels(32); // not all channels are used
    adcon1.setIOPin(0, &(*m_porta)[0]);
    adcon1.setIOPin(1, &(*m_porta)[1]);
    adcon1.setIOPin(2, &(*m_porta)[2]);
    adcon1.setIOPin(3, &(*m_porta)[4]);
    adcon1.setValidBits(0xf3);
    adcon1.setVrefHiConfiguration(0, 1);
    comparator.cmxcon1[0]->set_OUTpin(&(*m_porta)[2]);
    comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[1], &(*m_porta)[4]);
    comparator.cmxcon1[0]->set_INpinPos(&(*m_porta)[0]);
    comparator.cmxcon0[0]->setBitMask(0xf7);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, (1 << 5)));
    comparator.cmxcon1[0]->setBitMask(0xf1);
    comparator.assign_pir_set(get_pir_set());
    comparator.assign_t1gcon(&t1con_g.t1gcon);
    comparator.assign_sr_module(&sr_module);
    comparator.Pmask[0] = CMxCON1::CM_PIN;
    comparator.Pmask[2] = CMxCON1::CM_DAC1;
    comparator.Pmask[4] = CMxCON1::CM_FVR;
    comparator.Nmask[0] = CMxCON1::CM_PIN;
    comparator.Nmask[1] = CMxCON1::CM_PIN;
    comparator.attach_cda_fvr(fvrcon.get_node_cvref());
    m_daccon0->set_adcon1(&adcon1);
    m_daccon0->set_cpscon0(&cpscon0);
    m_daccon0->attach_cda_fvr(fvrcon.get_node_cvref(), 0x1e);
    m_daccon0->setDACOUT(&(*m_porta)[0]);
    cpscon0.attach_cda_fvr(fvrcon.get_node_cvref());
    cpscon0.attach_DAC(m_daccon0->get_node_dacout(), 99);
    cpscon0.set_pin(0, &(*m_porta)[0]);
    cpscon0.set_pin(1, &(*m_porta)[1]);
    cpscon0.set_pin(2, &(*m_porta)[2]);
    cpscon0.set_pin(3, &(*m_porta)[4]);
    sr_module.setPins(&(*m_porta)[1], &(*m_porta)[2], &(*m_porta)[5]);
    osccon->set_osctune(&osctune);
    osccon->set_oscstat(&oscstat);
    osctune.set_osccon((OSCCON *)osccon);
    osccon->write_mask = 0xfb;
    dsm_module.usart_mod = &usart;
    int_pin.setIOpin(&(*m_porta)[2],0);
}


//-------------------------------------------------------------------
void P12F1822::set_out_of_range_pm(unsigned int address, unsigned int value)
{
    if ((address >= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
        get_eeprom()->change_rom(address - 0x2100, value);
    }
}


void P12F1822::create_iopin_map()
{
    package = new Package(8);

    // Now Create the package and place the I/O pins
    package->assign_pin(7, m_porta->addPin(new IO_bi_directional_pu("porta0"), 0));
    package->assign_pin(6, m_porta->addPin(new IO_bi_directional_pu("porta1"), 1));
    package->assign_pin(5, m_porta->addPin(new IO_bi_directional_pu("porta2"), 2));
    package->assign_pin(4, m_porta->addPin(new IO_bi_directional_pu("porta3"), 3));
    package->assign_pin(3, m_porta->addPin(new IO_bi_directional_pu("porta4"), 4));
    package->assign_pin(2, m_porta->addPin(new IO_bi_directional_pu("porta5"), 5));
    package->assign_pin(1, 0);	// Vdd
    package->assign_pin(8, 0);	// Vss
}


void  P12F1822::create(int ram_top, int eeprom_size, int dev_id)
{
    create_iopin_map();
    e = new EEPROM_EXTND(this, pir2);
    set_eeprom(e);
    osccon = new OSCCON_2(this, "osccon", "Oscillator Control Register");
    pic_processor::create();
    e->initialize(eeprom_size, 16, 16, 0x8000);
    e->set_intcon(intcon);
    e->get_reg_eecon1()->set_valid_bits(0xff);
    add_file_registers(0x20, ram_top, 0x00);
    _14bit_e_processor::create_sfr_map();
    create_sfr_map();
    dsm_module.setOUTpin(&(*m_porta)[0]);
    dsm_module.setMINpin(&(*m_porta)[1]);
    dsm_module.setCIN1pin(&(*m_porta)[2]);
    dsm_module.setCIN2pin(&(*m_porta)[4]);

    // Set DeviceID
    if (m_configMemory && m_configMemory->getConfigWord(6))
    {
        m_configMemory->getConfigWord(6)->set(dev_id);
    }
}


//-------------------------------------------------------------------
void P12F1822::enter_sleep()
{
    tmr1l.sleep();
    osccon->sleep();
    _14bit_e_processor::enter_sleep();
}


//-------------------------------------------------------------------
void P12F1822::exit_sleep()
{
    if (m_ActivityState == ePASleeping)
    {
        tmr1l.wake();
        osccon->wake();
        _14bit_e_processor::exit_sleep();
    }
}


//-------------------------------------------------------------------
void P12F1822::option_new_bits_6_7(unsigned int bits)
{
    Dprintf(("P12F1822::option_new_bits_6_7 bits=%x\n", bits));
    m_porta->setIntEdge((bits & OPTION_REG::BIT6) == OPTION_REG::BIT6);
    m_wpua->set_wpu_pu((bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
}


void P12F1822::oscillator_select(unsigned int cfg_word1, bool clkout)
{
    unsigned int mask = 0x1f;
    unsigned int fosc = cfg_word1 & (FOSC0 | FOSC1 | FOSC2);
    osccon->set_config_irc(fosc == 4);
    osccon->set_config_xosc(fosc < 3);
    osccon->set_config_ieso(cfg_word1 & IESO);
    set_int_osc(false);

    switch (fosc)
    {
    case 0:	//LP oscillator: low power crystal
    case 1:	//XT oscillator: Crystal/resonator
    case 2:	//HS oscillator: High-speed crystal/resonator
        (m_porta->getPin(4))->newGUIname("OSC2");
        (m_porta->getPin(5))->newGUIname("OSC1");
        mask = 0x0f;
        break;

    case 3:	//EXTRC oscillator External RC circuit connected to CLKIN pin
        (m_porta->getPin(5))->newGUIname("CLKIN");
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        break;

    case 4:	//INTOSC oscillator: I/O function on CLKIN pin
        set_int_osc(true);
        mask = 0x3f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x2f;
        }

        (m_porta->getPin(5))->newGUIname((m_porta->getPin(5))->name().c_str());
        break;

    case 5:	//ECL: External Clock, Low-Power mode (0-0.5 MHz): on CLKIN pin
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;

    case 6:	//ECM: External Clock, Medium-Power mode (0.5-4 MHz): on CLKIN pin
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;

    case 7:	//ECH: External Clock, High-Power mode (4-32 MHz): on CLKIN pin
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;
    };

    ansela.setValidBits(0x17 & mask);

    m_porta->setEnableMask(mask);
}


void P12F1822::program_memory_wp(unsigned int mode)
{
    switch (mode)
    {
    case 3:	// no write protect
        get_eeprom()->set_prog_wp(0x0);
        break;

    case 2: // write protect 0000-01ff
        get_eeprom()->set_prog_wp(0x0200);
        break;

    case 1: // write protect 0000-03ff
        get_eeprom()->set_prog_wp(0x0400);
        break;

    case 0: // write protect 0000-07ff
        get_eeprom()->set_prog_wp(0x0800);
        break;

    default:
        printf("%s unexpected mode %u\n", __FUNCTION__, mode);
        break;
    }
}
//========================================================================


P12LF1822::P12LF1822(const char *_name, const char *desc)
    : P12F1822(_name, desc)
{
}


P12LF1822::~P12LF1822()
{
}


Processor * P12LF1822::construct(const char *name)
{
    P12LF1822 *p = new P12LF1822(name);
    p->create(0x7f, 256, 0x2800);
    p->create_invalid_registers();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}


void  P12LF1822::create(int ram_top, int eeprom_size, int dev_id)
{
    P12F1822::create(ram_top, eeprom_size, dev_id);
}


//========================================================================
Processor * P12F1840::construct(const char *name)
{
    P12F1840 *p = new P12F1840(name);
    p->create(0x7f, 256, 0x1b80);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


P12F1840::P12F1840(const char *_name, const char *desc)
    : P12F1822(_name, desc),
      vregcon(nullptr)
{
}


P12F1840::~P12F1840()
{
    delete_file_registers(0xc0, 0xef, 0x00);
    delete_file_registers(0x120, 0x16f, 0x00);
    delete_sfr_register(vregcon);
}


void  P12F1840::create(int ram_top, int eeprom_size, int /* dev_id */ )
{
    P12F1822::create(ram_top, eeprom_size, 0x1b80);
    add_file_registers(0xc0, 0xef, 0x00);
    add_file_registers(0x120, 0x16f, 0x00);
    vregcon = new sfr_register(this, "vregcon",
                               "Voltage Regulator Control Register");
    add_sfr_register(vregcon, 0x197, RegisterValue(0x01, 0));
}


//========================================================================
Processor * P12LF1840::construct(const char *name)
{
    P12LF1840 *p = new P12LF1840(name);
    p->create(0x7f, 256, 0x1b80);
    p->create_invalid_registers();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}


P12LF1840::P12LF1840(const char *_name, const char *desc) :
    P12F1840(_name, desc)
{
}


P12LF1840::~P12LF1840()
{
}


void  P12LF1840::create(int ram_top, int eeprom_size, int /* dev_id */ )
{
    P12F1840::create(ram_top, eeprom_size, 0x1bc0);
}


//========================================================================


P16F1503::P16F1503(const char *_name, const char *desc)
    : _14bit_e_processor(_name, desc),
      comparator(this),
      pie1(this, "pie1", "Peripheral Interrupt Enable"),
      pie2(this, "pie2", "Peripheral Interrupt Enable"),
      pie3(this, "pie3", "Peripheral Interrupt Enable"),
      t2con(this, "t2con", "TMR2 Control"),
      pr2(this, "pr2", "TMR2 Period Register"),
      tmr2(this, "tmr2", "TMR2 Register"),
      t1con_g(this, "t1con", "TMR1 Control Register"),
      tmr1l(this, "tmr1l", "TMR1 Low"),
      tmr1h(this, "tmr1h", "TMR1 High"),
      fvrcon(this, "fvrcon", "Voltage reference control register", 0xbf),
      borcon(this, "borcon", "Brown-out reset control register"),
      ansela(this, "ansela", "Analog Select port a"),
      anselc(this, "anselc", "Analog Select port c"),
      adcon0(this, "adcon0", "A2D Control 0"),
      adcon1(this, "adcon1", "A2D Control 1"),
      adcon2(this, "adcon2", "A2D Control 2"),
      adresh(this, "adresh", "A2D Result High"),
      adresl(this, "adresl", "A2D Result Low"),
      osccon(0),
      oscstat(this, "oscstat", "Oscillator Status Register"),
      wdtcon(this, "wdtcon", "Watch dog timer control", 0x3f),
      ssp(this),
      apfcon1(this, "apfcon", "Alternate Pin Function Control Register", 0x3b),
      pwm1con(this, "pwm1con", "PWM 1 Control Register", 0),
      pwm1dcl(this, "pwm1dcl", "PWM 1 DUTY CYCLE LOW BITS"),
      pwm1dch(this, "pwm1dch", "PWM 1 DUTY CYCLE HIGH BITS"),
      pwm2con(this, "pwm2con", "PWM 2 Control Register", 1),
      pwm2dcl(this, "pwm2dcl", "PWM 2 DUTY CYCLE LOW BITS"),
      pwm2dch(this, "pwm2dch", "PWM 2 DUTY CYCLE HIGH BITS"),
      pwm3con(this, "pwm3con", "PWM 3 Control Register", 2),
      pwm3dcl(this, "pwm3dcl", "PWM 3 DUTY CYCLE LOW BITS"),
      pwm3dch(this, "pwm3dch", "PWM 3 DUTY CYCLE HIGH BITS"),
      pwm4con(this, "pwm4con", "PWM 4 Control Register", 3),
      pwm4dcl(this, "pwm4dcl", "PWM 4 DUTY CYCLE LOW BITS"),
      pwm4dch(this, "pwm4dch", "PWM 4 DUTY CYCLE HIGH BITS"),
      cwg(this), nco(this),
      clcdata(this, "clcdata", "CLC Data Output"),
      clc1(this, 0, &clcdata), clc2(this, 1, &clcdata),
      frc(600000., CLC::FRC_IN),
      lfintosc(32000., CLC::LFINTOSC),  // 32kHz is within tolerance or 31kHz
      hfintosc(16e6, CLC::HFINTOSC),
      vregcon(this, "vregcon", "Voltage Regulator Control Register")
{
    m_portc = new PicPortBRegister(this, "portc", "", intcon, 8, 0x3f);
    m_trisc = new PicTrisRegister(this, "trisc", "", m_portc, false, 0x3f);
    m_latc  = new PicLatchRegister(this, "latc", "", m_portc, 0x3f);
    m_iocaf = new IOCxF(this, "iocaf", "Interrupt-On-Change flag Register", 0x3f);
    m_iocap = new IOC(this, "iocap", "Interrupt-On-Change positive edge", 0x3f);
    m_iocan = new IOC(this, "iocan", "Interrupt-On-Change negative edge", 0x3f);
    m_porta = new PicPortIOCRegister(this, "porta", "", intcon, m_iocap, m_iocan, m_iocaf, 8, 0x3f);
    m_trisa = new PicTrisRegister(this, "trisa", "", m_porta, false, 0x37);
    m_lata  = new PicLatchRegister(this, "lata", "", m_porta, 0x37);
    m_wpua = new WPU(this, "wpua", "Weak Pull-up Register", m_porta, 0x3f);
    m_daccon0 = new DACCON0(this, "daccon0", "DAC1 8bit Voltage reference register 0", 0xb4, 32);
    m_daccon1 = new DACCON1(this, "daccon1", "DAC1 8bit Voltage reference register 1", 0xff, m_daccon0);
    m_cpu_temp = new CPU_Temp("cpu_temperature", 30., "CPU die temperature");
    tmr0.link_cpu(this, m_porta, 4, option_reg);
    tmr0.start(0);
    tmr0.set_t1gcon(&t1con_g.t1gcon);
    set_mclr_pin(4);
    ((INTCON_14_PIR *)intcon)->write_mask = 0xfe;
    pir1 = new PIR1v1822(this, "pir1", "Peripheral Interrupt Register", intcon, &pie1);
    pir2 = new PIR2v1822(this, "pir2", "Peripheral Interrupt Register", intcon, &pie2);
    pir3 = new PIR3v178x(this, "pir3", "Peripheral Interrupt Register", intcon, &pie3);
    pir1->valid_bits = pir1->writable_bits = 0xcb;
    pir2->valid_bits = pir2->writable_bits = 0x6c;
    pir3->valid_bits = pir3->writable_bits = 0x03;
    comparator.cmxcon0[0] = new CMxCON0(this, "cm1con0", " Comparator C1 Control Register 0", 0, &comparator);
    comparator.cmxcon1[0] = new CMxCON1(this, "cm1con1", " Comparator C1 Control Register 1", 0, &comparator);
    comparator.cmout = new CMOUT(this, "cmout", "Comparator Output Register");
    comparator.cmxcon0[1] = new CMxCON0(this, "cm2con0", " Comparator C2 Control Register 0", 1, &comparator);
    comparator.cmxcon1[1] = new CMxCON1(this, "cm2con1", " Comparator C2 Control Register 1", 1, &comparator);

    clc1.set_dxs_data(1, 8, lcxd1_1);
    clc1.set_dxs_data(2, 8, lcxd2_1);
    clc1.set_dxs_data(3, 8, lcxd3_1);
    clc1.set_dxs_data(4, 8, lcxd4_1);
    clc2.set_dxs_data(1, 8, lcxd1_2);
    clc2.set_dxs_data(2, 8, lcxd2_2);
    clc2.set_dxs_data(3, 8, lcxd3_2);
    clc2.set_dxs_data(4, 8, lcxd4_2);
}


P16F1503::~P16F1503()
{
    adcon1.detach_fvr();
    adcon1.detach_DAC();
    comparator.detach_fvr();
    m_daccon0->detach_fvr();
    unassignMCLRPin();
    delete_file_registers(0x20, 0x7f);
    delete_file_registers(0xa0, 0xbf);
    delete_sfr_register(m_iocap);
    delete_sfr_register(m_iocan);
    delete_sfr_register(m_iocaf);
    delete_sfr_register(m_daccon0);
    delete_sfr_register(m_daccon1);
    delete_sfr_register(m_trisa);
    delete_sfr_register(m_porta);
    delete_sfr_register(m_lata);
    delete_sfr_register(m_wpua);
    delete_sfr_register(m_portc);
    delete_sfr_register(m_trisc);
    delete_sfr_register(m_latc);
    remove_sfr_register(&clcdata);
    remove_sfr_register(&clc1.clcxcon);
    remove_sfr_register(&clc1.clcxpol);
    remove_sfr_register(&clc1.clcxsel0);
    remove_sfr_register(&clc1.clcxsel1);
    remove_sfr_register(&clc1.clcxgls0);
    remove_sfr_register(&clc1.clcxgls1);
    remove_sfr_register(&clc1.clcxgls2);
    remove_sfr_register(&clc1.clcxgls3);
    remove_sfr_register(&clc2.clcxcon);
    remove_sfr_register(&clc2.clcxpol);
    remove_sfr_register(&clc2.clcxsel0);
    remove_sfr_register(&clc2.clcxsel1);
    remove_sfr_register(&clc2.clcxgls0);
    remove_sfr_register(&clc2.clcxgls1);
    remove_sfr_register(&clc2.clcxgls2);
    remove_sfr_register(&clc2.clcxgls3);
    remove_sfr_register(&tmr0);
    remove_sfr_register(&tmr1l);
    remove_sfr_register(&tmr1h);
    remove_sfr_register(&t1con_g);
    remove_sfr_register(&t1con_g.t1gcon);
    remove_sfr_register(&tmr2);
    remove_sfr_register(&pr2);
    remove_sfr_register(&t2con);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&pwm1dcl);
    remove_sfr_register(&pwm1dch);
    remove_sfr_register(&pwm2con);
    remove_sfr_register(&pwm2dcl);
    remove_sfr_register(&pwm2dch);
    remove_sfr_register(&pwm3con);
    remove_sfr_register(&pwm3dcl);
    remove_sfr_register(&pwm3dch);
    remove_sfr_register(&pwm4con);
    remove_sfr_register(&pwm4dcl);
    remove_sfr_register(&pwm4dch);
    remove_sfr_register(&pie1);
    remove_sfr_register(&pie2);
    remove_sfr_register(&pie3);
    remove_sfr_register(&adresl);
    remove_sfr_register(&adresh);
    remove_sfr_register(&adcon0);
    remove_sfr_register(&adcon1);
    remove_sfr_register(&adcon2);
    remove_sfr_register(&borcon);
    remove_sfr_register(&fvrcon);
    remove_sfr_register(&apfcon1);
    remove_sfr_register(&ansela);
    remove_sfr_register(&anselc);
    remove_sfr_register(&vregcon);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&nco.nco1accl);
    remove_sfr_register(&nco.nco1acch);
    remove_sfr_register(&nco.nco1accu);
    remove_sfr_register(&nco.nco1incl);
    remove_sfr_register(&nco.nco1inch);
    remove_sfr_register(&nco.nco1con);
    remove_sfr_register(&nco.nco1clk);
    remove_sfr_register(&cwg.cwg1con0);
    remove_sfr_register(&cwg.cwg1con1);
    remove_sfr_register(&cwg.cwg1con2);
    remove_sfr_register(&cwg.cwg1dbr);
    remove_sfr_register(&cwg.cwg1dbf);
    remove_sfr_register(option_reg);
    remove_sfr_register(osccon);
    remove_sfr_register(&oscstat);
    remove_sfr_register(comparator.cmxcon0[0]);
    remove_sfr_register(comparator.cmxcon1[0]);
    remove_sfr_register(comparator.cmout);
    remove_sfr_register(comparator.cmxcon0[1]);
    remove_sfr_register(comparator.cmxcon1[1]);
    delete_sfr_register(pir1);
    delete_sfr_register(pir2);
    delete_sfr_register(pir3);
    delete e;
    delete m_cpu_temp;
}


void P16F1503::create_iopin_map()
{
    package = new Package(14);

    //createMCLRPin(1);
    // Now Create the package and place the I/O pins
    package->assign_pin(1, 0);	//Vdd
    package->assign_pin(2, m_porta->addPin(new IO_bi_directional_pu("porta5"), 5));
    package->assign_pin(3, m_porta->addPin(new IO_bi_directional_pu("porta4"), 4));
    package->assign_pin(4, m_porta->addPin(new IO_bi_directional_pu("porta3"), 3));
    package->assign_pin(5, m_portc->addPin(new IO_bi_directional_pu("portc5"), 5));
    package->assign_pin(6, m_portc->addPin(new IO_bi_directional_pu("portc4"), 4));
    package->assign_pin(7, m_portc->addPin(new IO_bi_directional_pu("portc3"), 3));
    package->assign_pin(8, m_portc->addPin(new IO_bi_directional_pu("portc2"), 2));
    package->assign_pin(9, m_portc->addPin(new IO_bi_directional_pu("portc1"), 1));
    package->assign_pin(10, m_portc->addPin(new IO_bi_directional_pu("portc0"), 0));
    package->assign_pin(11, m_porta->addPin(new IO_bi_directional_pu("porta2"), 2));
    package->assign_pin(12, m_porta->addPin(new IO_bi_directional_pu("porta1"), 1));
    package->assign_pin(13, m_porta->addPin(new IO_bi_directional_pu("porta0"), 0));
    package->assign_pin(14, 0);	// Vss
}


void P16F1503::create_symbols()
{
    pic_processor::create_symbols();
    addSymbol(Wreg);
    addSymbol(m_cpu_temp);
}


void P16F1503::create_sfr_map()
{
    pir_set_2_def.set_pir1(pir1);
    pir_set_2_def.set_pir2(pir2);
    pir_set_2_def.set_pir3(pir3);
    add_file_registers(0x20, 0x7f, 0x00);
    add_file_registers(0xa0, 0xbf, 0x00);
    add_sfr_register(m_porta, 0x0c);
    add_sfr_register(m_portc, 0x0e);
    add_sfr_registerR(pir1,    0x11, RegisterValue(0, 0), "pir1");
    add_sfr_registerR(pir2,    0x12, RegisterValue(0, 0), "pir2");
    add_sfr_registerR(pir3,    0x13, RegisterValue(0, 0), "pir3");
    add_sfr_register(&tmr0,   0x15);
    add_sfr_register(&tmr1l,  0x16, RegisterValue(0, 0), "tmr1l");
    add_sfr_register(&tmr1h,  0x17, RegisterValue(0, 0), "tmr1h");
    add_sfr_register(&t1con_g,  0x18, RegisterValue(0, 0));
    add_sfr_register(&t1con_g.t1gcon, 0x19, RegisterValue(0, 0));
    add_sfr_registerR(&tmr2,   0x1a, RegisterValue(0, 0));
    add_sfr_registerR(&pr2,    0x1b, RegisterValue(0, 0));
    add_sfr_registerR(&t2con,  0x1c, RegisterValue(0, 0));
    add_sfr_register(m_trisa, 0x8c, RegisterValue(0x3f, 0));
    add_sfr_register(m_trisc, 0x8e, RegisterValue(0x3f, 0));
    pcon.valid_bits = 0xcf;
    add_sfr_register(option_reg, 0x95, RegisterValue(0xff, 0));
    add_sfr_registerR(osccon,     0x99, RegisterValue(0x38, 0));
    add_sfr_register(&oscstat,    0x9a, RegisterValue(0, 0));
    intcon_reg.set_pir_set(get_pir_set());
    tmr1l.tmrh = &tmr1h;
    tmr1l.t1con = &t1con_g;
    tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v1::TMR1IF));
    tmr1h.tmrl  = &tmr1l;
    t1con_g.tmrl  = &tmr1l;
    t1con_g.t1gcon.set_tmrl(&tmr1l);
    t1con_g.t1gcon.setInterruptSource(new InterruptSource(pir1, PIR1v1822::TMR1GIF));
    tmr1l.setIOpin(&(*m_porta)[5]);
    t1con_g.t1gcon.setGatepin(&(*m_porta)[3]);
    add_sfr_registerR(&pie1,   0x91, RegisterValue(0, 0));
    add_sfr_registerR(&pie2,   0x92, RegisterValue(0, 0));
    add_sfr_registerR(&pie3,   0x93, RegisterValue(0, 0));
    add_sfr_register(&adresl, 0x9b);
    add_sfr_register(&adresh, 0x9c);
    add_sfr_registerR(&adcon0, 0x9d, RegisterValue(0x00, 0));
    add_sfr_registerR(&adcon1, 0x9e, RegisterValue(0x00, 0));
    add_sfr_registerR(&adcon2, 0x9f, RegisterValue(0x00, 0));
    add_sfr_register(m_lata,    0x10c);
    add_sfr_register(m_latc, 0x10e);
    add_sfr_registerR(comparator.cmxcon0[0], 0x111, RegisterValue(0x04, 0));
    add_sfr_registerR(comparator.cmxcon1[0], 0x112, RegisterValue(0x00, 0));
    add_sfr_registerR(comparator.cmxcon0[1], 0x113, RegisterValue(0x04, 0));
    add_sfr_registerR(comparator.cmxcon1[1], 0x114, RegisterValue(0x00, 0));
    add_sfr_registerR(comparator.cmout,      0x115, RegisterValue(0x00, 0));
    add_sfr_register(&borcon,   0x116, RegisterValue(0x80, 0));
    add_sfr_register(&fvrcon,   0x117, RegisterValue(0x00, 0));
    add_sfr_registerR(m_daccon0, 0x118, RegisterValue(0x00, 0));
    add_sfr_registerR(m_daccon1, 0x119, RegisterValue(0x00, 0));
    add_sfr_registerR(&apfcon1,  0x11d, RegisterValue(0x00, 0));
    add_sfr_registerR(&ansela,   0x18c, RegisterValue(0x17, 0));
    add_sfr_registerR(&anselc,   0x18e, RegisterValue(0x0f, 0));
    get_eeprom()->get_reg_eedata()->new_name("pmdatl");
    get_eeprom()->get_reg_eedatah()->new_name("pmdath");
    add_sfr_registerR(get_eeprom()->get_reg_eeadr(), 0x191, RegisterValue(0, 0), "pmadrl");
    add_sfr_registerR(get_eeprom()->get_reg_eeadrh(), 0x192, RegisterValue(0, 0), "pmadrh");
    add_sfr_register(get_eeprom()->get_reg_eedata(),  0x193);
    add_sfr_register(get_eeprom()->get_reg_eedatah(),  0x194);
    get_eeprom()->get_reg_eecon1()->set_always_on(1 << 7);
    add_sfr_registerR(get_eeprom()->get_reg_eecon1(),  0x195, RegisterValue(0x80, 0), "pmcon1");
    add_sfr_registerR(get_eeprom()->get_reg_eecon2(),  0x196, RegisterValue(0, 0), "pmcon2");
    add_sfr_registerR(&vregcon, 0x197, RegisterValue(1, 0));
    add_sfr_register(m_wpua,     0x20c, RegisterValue(0xff, 0), "wpua");
    add_sfr_registerR(&ssp.sspbuf,  0x211, RegisterValue(0, 0), "ssp1buf");
    add_sfr_registerR(&ssp.sspadd,  0x212, RegisterValue(0, 0), "ssp1add");
    add_sfr_registerR(ssp.sspmsk, 0x213, RegisterValue(0xff, 0), "ssp1msk");
    add_sfr_registerR(&ssp.sspstat, 0x214, RegisterValue(0, 0), "ssp1stat");
    add_sfr_registerR(&ssp.sspcon,  0x215, RegisterValue(0, 0), "ssp1con");
    add_sfr_registerR(&ssp.sspcon2, 0x216, RegisterValue(0, 0), "ssp1con2");
    add_sfr_registerR(&ssp.ssp1con3, 0x217, RegisterValue(0, 0), "ssp1con3");
    //  add_sfr_register(&pstr1con,    0x296, RegisterValue(1,0));
    add_sfr_registerR(m_iocap, 0x391, RegisterValue(0, 0), "iocap");
    add_sfr_registerR(m_iocan, 0x392, RegisterValue(0, 0), "iocan");
    add_sfr_registerR(m_iocaf, 0x393, RegisterValue(0, 0), "iocaf");
    m_iocaf->set_intcon(intcon);
    add_sfr_registerR(&nco.nco1accl, 0x498, RegisterValue(0, 0));
    add_sfr_registerR(&nco.nco1acch, 0x499, RegisterValue(0, 0));
    add_sfr_registerR(&nco.nco1accu, 0x49a, RegisterValue(0, 0));
    add_sfr_registerR(&nco.nco1incl, 0x49b, RegisterValue(1, 0));
    add_sfr_registerR(&nco.nco1inch, 0x49c, RegisterValue(0, 0));
    add_sfr_registerR(&nco.nco1con,  0x49e, RegisterValue(0, 0));
    add_sfr_registerR(&nco.nco1clk,  0x49f, RegisterValue(0, 0));
    nco.setIOpins(&(*m_porta)[5], &(*m_portc)[1]);
    nco.m_NCOif = new InterruptSource(pir2, 4);
    nco.set_clc(&clc1, 0);
    nco.set_clc(&clc2, 1);
    nco.set_cwg(&cwg);
    nco.set_clc_data_server(clc1.get_CLC_data_server());
    add_sfr_registerR(&pwm1dcl,  0x611, RegisterValue(0, 0));
    add_sfr_register(&pwm1dch,  0x612, RegisterValue(0, 0));
    add_sfr_registerR(&pwm1con,  0x613, RegisterValue(0, 0));
    add_sfr_registerR(&pwm2dcl,  0x614, RegisterValue(0, 0));
    add_sfr_register(&pwm2dch,  0x615, RegisterValue(0, 0));
    add_sfr_registerR(&pwm2con,  0x616, RegisterValue(0, 0));
    add_sfr_registerR(&pwm3dcl,  0x617, RegisterValue(0, 0));
    add_sfr_register(&pwm3dch,  0x618, RegisterValue(0, 0));
    add_sfr_registerR(&pwm3con,  0x619, RegisterValue(0, 0));
    add_sfr_registerR(&pwm4dcl,  0x61a, RegisterValue(0, 0));
    add_sfr_register(&pwm4dch,  0x61b, RegisterValue(0, 0));
    add_sfr_registerR(&pwm4con,  0x61c, RegisterValue(0, 0));
    add_sfr_registerR(&cwg.cwg1dbr, 0x691);
    add_sfr_register(&cwg.cwg1dbf, 0x692);
    add_sfr_registerR(&cwg.cwg1con0, 0x693, RegisterValue(0, 0));
    add_sfr_registerR(&cwg.cwg1con1, 0x694);
    add_sfr_registerR(&cwg.cwg1con2, 0x695);
    add_sfr_registerR(&clcdata, 0xf0f, RegisterValue(0, 0));
    add_sfr_registerR(&clc1.clcxcon, 0xf10, RegisterValue(0, 0), "clc1con");
    add_sfr_register(&clc1.clcxpol, 0xf11, RegisterValue(0, 0), "clc1pol");
    add_sfr_register(&clc1.clcxsel0, 0xf12, RegisterValue(0, 0), "clc1sel0");
    add_sfr_register(&clc1.clcxsel1, 0xf13, RegisterValue(0, 0), "clc1sel1");
    add_sfr_register(&clc1.clcxgls0, 0xf14, RegisterValue(0, 0), "clc1gls0");
    add_sfr_register(&clc1.clcxgls1, 0xf15, RegisterValue(0, 0), "clc1gls1");
    add_sfr_register(&clc1.clcxgls2, 0xf16, RegisterValue(0, 0), "clc1gls2");
    add_sfr_register(&clc1.clcxgls3, 0xf17, RegisterValue(0, 0), "clc1gls3");
    add_sfr_registerR(&clc2.clcxcon, 0xf18, RegisterValue(0, 0), "clc2con");
    add_sfr_register(&clc2.clcxpol, 0xf19, RegisterValue(0, 0), "clc2pol");
    add_sfr_register(&clc2.clcxsel0, 0xf1a, RegisterValue(0, 0), "clc2sel0");
    add_sfr_register(&clc2.clcxsel1, 0xf1b, RegisterValue(0, 0), "clc2sel1");
    add_sfr_register(&clc2.clcxgls0, 0xf1c, RegisterValue(0, 0), "clc2gls0");
    add_sfr_register(&clc2.clcxgls1, 0xf1d, RegisterValue(0, 0), "clc2gls1");
    add_sfr_register(&clc2.clcxgls2, 0xf1e, RegisterValue(0, 0), "clc2gls2");
    add_sfr_register(&clc2.clcxgls3, 0xf1f, RegisterValue(0, 0), "clc2gls3");
    clc1.frc = &frc;
    clc2.frc = &frc;
    clc1.lfintosc = &lfintosc;
    clc2.lfintosc = &lfintosc;
    clc1.hfintosc = &hfintosc;
    clc2.hfintosc = &hfintosc;
    clc1.p_nco = &nco;
    clc1.set_clc(&clc1, &clc2);
    clc2.set_clc(&clc1, &clc2);
    frc.set_clc(&clc1, &clc2);
    lfintosc.set_clc(&clc1, &clc2);
    hfintosc.set_clc(&clc1, &clc2);
    tmr0.set_clc(&clc1, 0);
    tmr0.set_clc(&clc2, 1);
    t1con_g.tmrl->m_clc[0] = tmr2.m_clc[0] = &clc1;
    t1con_g.tmrl->m_clc[1] = tmr2.m_clc[1] = &clc2;
    comparator.m_clc[0] = &clc1;
    comparator.m_clc[1] = &clc2;
    clc1.set_clcPins(&(*m_porta)[2], &(*m_porta)[3], &(*m_porta)[5]);
    clc2.set_clcPins(&(*m_portc)[2], &(*m_portc)[3], &(*m_portc)[4]);
    clc1.setInterruptSource(new InterruptSource(pir3, 1));
    clc2.setInterruptSource(new InterruptSource(pir3, 2));
    tmr2.ssp_module[0] = &ssp;
    ssp.initialize(
        get_pir_set(),    // PIR
        & (*m_portc)[0],  // SCK
        & (*m_portc)[3],  // SS
        & (*m_portc)[2],  // SDO
        & (*m_portc)[1],   // SDI
        m_trisc,          // i2c tris port
        SSP_TYPE_MSSP1
    );
    apfcon1.set_ValidBits(0x3b);
    apfcon1.set_pins(0, &nco, NCO::NCOout_PIN, &(*m_portc)[1], &(*m_porta)[4]); //NCO
    apfcon1.set_pins(1, &clc1, CLC::CLCout_PIN, &(*m_porta)[2], &(*m_portc)[5]); //CLC
    apfcon1.set_pins(3, &t1con_g.t1gcon, 0, &(*m_porta)[4], &(*m_porta)[3]); //tmr1 gate
    apfcon1.set_pins(4, &ssp, SSP1_MODULE::SS_PIN, &(*m_portc)[3], &(*m_porta)[3]); //SSP SS
    apfcon1.set_pins(5, &ssp, SSP1_MODULE::SDO_PIN, &(*m_portc)[2], &(*m_porta)[4]); //SSP SDO

    if (pir1)
    {
        pir1->set_intcon(intcon);
        pir1->set_pie(&pie1);
    }

    pie1.setPir(pir1);
    pie2.setPir(pir2);
    pie3.setPir(pir3);
    t2con.tmr2 = &tmr2;
    tmr2.pir_set   = get_pir_set();
    tmr2.pr2    = &pr2;
    tmr2.t2con  = &t2con;
    tmr2.add_ccp(&pwm1con);
    tmr2.add_ccp(&pwm2con);
    tmr2.add_ccp(&pwm3con);
    tmr2.add_ccp(&pwm4con);
    pr2.tmr2    = &tmr2;
    pwm1con.set_pwmdc(&pwm1dcl, &pwm1dch);
    pwm1con.setIOpin(&(*m_portc)[5], CCPCON::CCP_PIN);
    pwm1con.set_tmr2(&tmr2);
    pwm1con.set_cwg(&cwg);
    pwm1con.set_clc(&clc1, 0);
    pwm1con.set_clc(&clc2, 1);
    pwm2con.set_pwmdc(&pwm2dcl, &pwm2dch);
    pwm2con.setIOpin(&(*m_portc)[3], CCPCON::CCP_PIN);
    pwm2con.set_tmr2(&tmr2);
    pwm2con.set_cwg(&cwg);
    pwm2con.set_clc(&clc1, 0);
    pwm2con.set_clc(&clc2, 1);
    pwm3con.set_pwmdc(&pwm3dcl, &pwm3dch);
    pwm3con.setIOpin(&(*m_porta)[2], CCPCON::CCP_PIN);
    pwm3con.set_tmr2(&tmr2);
    pwm3con.set_cwg(&cwg);
    pwm3con.set_clc(&clc1, 0);
    pwm3con.set_clc(&clc2, 1);
    pwm4con.set_pwmdc(&pwm4dcl, &pwm4dch);
    pwm4con.setIOpin(&(*m_portc)[1], CCPCON::CCP_PIN);
    pwm4con.set_tmr2(&tmr2);
    pwm4con.set_cwg(&cwg);
    pwm4con.set_clc(&clc1, 0);
    pwm4con.set_clc(&clc2, 1);
    cwg.set_IOpins(&(*m_portc)[5], &(*m_portc)[4], &(*m_porta)[2]);
    ansela.config(0x17, 0);
    ansela.setValidBits(0x17);
    ansela.setAdcon1(&adcon1);
    anselc.config(0x0f, 4);
    anselc.setValidBits(0x0f);
    anselc.setAdcon1(&adcon1);
    ansela.setAnsel(&anselc);
    anselc.setAnsel(&ansela);
    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    //  adcon0.setAdcon2(&adcon2);
    adcon0.setIntcon(intcon);
    adcon0.setA2DBits(10);
    adcon0.setPir(pir1);
    adcon0.setChannel_Mask(0x1f);
    adcon0.setChannel_shift(2);
    adcon0.setGo(1);
    adcon2.setAdcon0(&adcon0);
    adcon1.set_FVR_chan(0x1f);
    adcon1.attach_ad_fvr(fvrcon.get_node_adcvref(), 0x1f);
    adcon1.attach_Vt_fvr(fvrcon.get_node_Vtref(), 0x1d);
    adcon1.attach_DAC(m_daccon0->get_node_dacout(), 0x1e);
    tmr0.set_adcon2(&adcon2);
    adcon1.setAdcon0(&adcon0);
    adcon1.setNumberOfChannels(32); // not all channels are used
    adcon1.setIOPin(0, &(*m_porta)[0]);
    adcon1.setIOPin(1, &(*m_porta)[1]);
    adcon1.setIOPin(2, &(*m_porta)[2]);
    adcon1.setIOPin(3, &(*m_porta)[4]);
    adcon1.setIOPin(4, &(*m_portc)[0]);
    adcon1.setIOPin(5, &(*m_portc)[1]);
    adcon1.setIOPin(6, &(*m_portc)[2]);
    adcon1.setIOPin(7, &(*m_portc)[3]);
    adcon1.setValidBits(0xf7);
    adcon1.setVrefHiConfiguration(0, 0);
    comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[0], &(*m_portc)[1],  &(*m_portc)[2],  &(*m_portc)[3]);
    comparator.cmxcon1[1]->set_INpinNeg(&(*m_porta)[0], &(*m_portc)[1],  &(*m_portc)[2],  &(*m_portc)[3]);
    comparator.cmxcon1[0]->set_INpinPos(&(*m_porta)[0]);
    comparator.cmxcon1[1]->set_INpinPos(&(*m_portc)[0]);
    comparator.cmxcon1[0]->set_OUTpin(&(*m_porta)[2]);
    comparator.cmxcon1[1]->set_OUTpin(&(*m_portc)[4]);
    comparator.cmxcon0[0]->setBitMask(0xbf);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, (1 << 5)));
    comparator.cmxcon0[1]->setBitMask(0xbf);
    comparator.cmxcon0[1]->setIntSrc(new InterruptSource(pir2, (1 << 6)));
    comparator.cmxcon1[0]->setBitMask(0xff);
    comparator.cmxcon1[1]->setBitMask(0xff);
    comparator.assign_pir_set(get_pir_set());
    comparator.assign_t1gcon(&t1con_g.t1gcon);
    m_daccon0->set_adcon1(&adcon1);
    comparator.attach_cda_fvr(fvrcon.get_node_cvref());
    m_daccon0->attach_cda_fvr(fvrcon.get_node_cvref(), 0x1e);
    m_daccon0->setDACOUT(&(*m_porta)[0], &(*m_porta)[2]);
    osccon->set_oscstat(&oscstat);
    //RRRosctune.set_osccon((OSCCON *)osccon);
    osccon->write_mask = 0xfb;
    int_pin.setIOpin(&(*m_porta)[2]);
}


//-------------------------------------------------------------------
void P16F1503::set_out_of_range_pm(unsigned int address, unsigned int value)
{
    if ((address >= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
        get_eeprom()->change_rom(address - 0x2100, value);
    }
}


void  P16F1503::create(int /* ram_top */, int dev_id)
{
    create_iopin_map();
    osccon = new OSCCON_2(this, "osccon", "Oscillator Control Register");
    e = new EEPROM_EXTND(this, pir2);
    set_eeprom(e);
    e->initialize(0, 16, 16, 0x8000, true);
    e->set_intcon(intcon);
    e->get_reg_eecon1()->set_valid_bits(0x7f);
    pic_processor::create();
    P16F1503::create_sfr_map();
    _14bit_e_processor::create_sfr_map();

    // Set DeviceID
    if (m_configMemory && m_configMemory->getConfigWord(6))
    {
        m_configMemory->getConfigWord(6)->set(dev_id);
    }
}


//-------------------------------------------------------------------
void P16F1503::enter_sleep()
{
    if (wdt_flag == 2)          // WDT is suspended during sleep
    {
        wdt->initialize(false);

    }
    else if (get_pir_set()->interrupt_status())
    {
        pc->increment();
        return;
    }

    tmr1l.sleep();
    osccon->sleep();
    tmr0.sleep();
    nco.sleep(true);
    pic_processor::enter_sleep();
}


//-------------------------------------------------------------------
void P16F1503::exit_sleep()
{
    if (m_ActivityState == ePASleeping)
    {
        tmr1l.wake();
        osccon->wake();
        nco.sleep(false);
        _14bit_e_processor::exit_sleep();
    }
}


//-------------------------------------------------------------------
void P16F1503::option_new_bits_6_7(unsigned int bits)
{
    Dprintf(("P16F1503::option_new_bits_6_7 bits=%x\n", bits));
    m_porta->setIntEdge((bits & OPTION_REG::BIT6) == OPTION_REG::BIT6);
    m_wpua->set_wpu_pu((bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
}


void P16F1503::oscillator_select(unsigned int cfg_word1, bool clkout)
{
    unsigned int mask = 0x1f;
    unsigned int fosc = cfg_word1 & (FOSC0 | FOSC1 | FOSC2);
    osccon->set_config_irc(fosc == 4);
    osccon->set_config_xosc(fosc < 3);
    osccon->set_config_ieso(cfg_word1 & IESO);
    set_int_osc(false);

    switch (fosc)
    {
    case 0:	//LP oscillator: low power crystal
    case 1:	//XT oscillator: Crystal/resonator
    case 2:	//HS oscillator: High-speed crystal/resonator
        (m_porta->getPin(4))->newGUIname("OSC2");
        (m_porta->getPin(5))->newGUIname("OSC1");
        mask = 0x0f;
        break;

    case 3:	//EXTRC oscillator External RC circuit connected to CLKIN pin
        (m_porta->getPin(5))->newGUIname("CLKIN");
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        break;

    case 4:	//INTOSC oscillator: I/O function on CLKIN pin
        set_int_osc(true);
        mask = 0x3f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x2f;
        }

        (m_porta->getPin(5))->newGUIname((m_porta->getPin(5))->name().c_str());
        break;

    case 5:	//ECL: External Clock, Low-Power mode (0-0.5 MHz): on CLKIN pin
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;

    case 6:	//ECM: External Clock, Medium-Power mode (0.5-4 MHz): on CLKIN pin
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;

    case 7:	//ECH: External Clock, High-Power mode (4-32 MHz): on CLKIN pin
        mask = 0x1f;

        if (clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }

        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;
    };

    ansela.setValidBits(0x17 & mask);

    m_porta->setEnableMask(mask);
}


void P16F1503::program_memory_wp(unsigned int mode)
{
    switch (mode)
    {
    case 3:	// no write protect
        get_eeprom()->set_prog_wp(0x0);
        break;

    case 2: // write protect 0000-01ff
        get_eeprom()->set_prog_wp(0x0200);
        break;

    case 1: // write protect 0000-03ff
        get_eeprom()->set_prog_wp(0x0400);
        break;

    case 0: // write protect 0000-07ff
        get_eeprom()->set_prog_wp(0x0800);
        break;

    default:
        printf("%s unexpected mode %u\n", __FUNCTION__, mode);
        break;
    }
}


Processor * P16F1503::construct(const char *name)
{
    P16F1503 *p = new P16F1503(name);
    p->create(2048, 0x2ce0);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


//========================================================================


P16LF1503::P16LF1503(const char *_name, const char *desc)
    : P16F1503(_name, desc)
{
}


Processor * P16LF1503::construct(const char *name)
{
    P16LF1503 *p = new P16LF1503(name);
    p->create(2048, 0x2da0);
    p->create_invalid_registers();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}


//========================================================================


P16F170x::P16F170x(const char *_name, const char *desc)
    : _14bit_e_processor(_name,desc),
      comparator(this),
      pie1(this,"pie1", "Peripheral Interrupt Enable"),
      pie2(this,"pie2", "Peripheral Interrupt Enable"),
      pie3(this,"pie3", "Peripheral Interrupt Enable"),
      t2con(this, "t2con", "TMR2 Control"),
      pr2(this, "pr2", "TMR2 Period Register"),
      tmr2(this, "tmr2", "TMR2 Register"),
      t4con(this, "t4con", "TMR4 Control"),
      pr4(this, "pr4", "TMR4 Period Register"),
      tmr4(this, "tmr4", "TMR4 Register"),
      t6con(this, "t6con", "TMR6 Control"),
      pr6(this, "pr6", "TMR6 Period Register"),
      tmr6(this, "tmr6", "TMR6 Register"),
      t1con_g(this, "t1con", "TMR1 Control Register"),
      tmr1l(this, "tmr1l", "TMR1 Low"),
      tmr1h(this, "tmr1h", "TMR1 High"),
      fvrcon(this, "fvrcon", "Voltage reference control register", 0xbf),
      borcon(this, "borcon", "Brown-out reset control register"),
      ansela(this, "ansela", "Analog Select port a"),
      anselc(this, "anselc", "Analog Select port c"),
      adcon0(this,"adcon0", "A2D Control 0"),
      adcon1(this,"adcon1", "A2D Control 1"),
      adcon2(this,"adcon2", "A2D Control 2"),
      adresh(this,"adresh", "A2D Result High"),
      adresl(this,"adresl", "A2D Result Low"),
      osccon(0),
      osctune(this, "osctune", "Oscillator Tunning Register"),
      oscstat(this, "oscstat", "Oscillator Status Register"),
      wdtcon(this, "wdtcon", "Watch dog timer control", 0x3f),
      usart(this),
      ssp(this),
      ccp1con(this, "ccp1con", "Capture Compare Control"),
      ccpr1l(this, "ccpr1l", "Capture Compare 1 Low"),
      ccpr1h(this, "ccpr1h", "Capture Compare 1 High"),
      ccp2con(this, "ccp2con", "Capture Compare Control"),
      ccpr2l(this, "ccpr2l", "Capture Compare 2 Low"),
      ccpr2h(this, "ccpr2h", "Capture Compare 2 High"),
      ccptmrs(this, "ccptmrs", "PWM Timer Selection Control Register"),
      pwm3con(this, "pwm3con", "PWM 3 Control Register", 2),
      pwm3dcl(this, "pwm3dcl", "PWM 3 DUTY CYCLE LOW BITS"),
      pwm3dch(this, "pwm3dch", "PWM 3 DUTY CYCLE HIGH BITS"),
      pwm4con(this, "pwm4con", "PWM 4 Control Register", 3),
      pwm4dcl(this, "pwm4dcl", "PWM 4 DUTY CYCLE LOW BITS"),
      pwm4dch(this, "pwm4dch", "PWM 4 Duty Cycle High bits"),
      cog(this, "COG"),
      clcdata(this, "clcdata", "CLC Data Output"),
      clc1(this, 0, &clcdata),
      clc2(this, 1, &clcdata),
      clc3(this, 2, &clcdata),
      frc(600000., CLC::FRC_IN),
      lfintosc(32000., CLC::LFINTOSC),  // 32kHz is within tolerance or 31kHz
      hfintosc(16e6, CLC::HFINTOSC),
      pps(),
      ppslock(&pps, this, "ppslock", "PPS Lock Register"),
      zcd1con(this, "zcd1con", "Zero Crossing Detector"),
      slrcona(this, "slrcona", "Slew Rate Control port A"),
      slrconc(this, "slrconc", "Slew Rate Control port C"),
      opa1con(this, "opa1con", "Operational Amplifier 1 Control Registers"),
      opa2con(this, "opa2con", "Operational Amplifier 2 Control Registers")
{
    m_iocaf = new IOCxF(this, "iocaf", "Interrupt-On-Change flag Register", 0x3f);
    m_iocap = new IOC(this, "iocap", "Interrupt-On-Change positive edge", 0x3f);
    m_iocan = new IOC(this, "iocan", "Interrupt-On-Change negative edge", 0x3f);
    m_porta= new PicPortIOCRegister(this,"porta","", intcon, m_iocap, m_iocan, m_iocaf, 8,0x3f);
    m_trisa = new PicTrisRegister(this,"trisa","", m_porta, false, 0x37);
    m_lata  = new PicLatchRegister(this,"lata","",m_porta, 0x37);
    m_wpua = new WPU(this, "wpua", "Weak Pull-up Register", m_porta, 0x3f);
    m_odcona = new ODCON(this, "odcona", "Open Drain Control Register", m_porta, 0x37);
    m_inlvla = new INLVL(this, "inlvla", "Input Level Control Register A", m_porta, 0x3f);
    m_daccon0 = new DACCON0(this, "dac1con0", "DAC1 8bit Voltage reference register 0", 0xbd, 32);
    m_daccon1 = new DACCON1(this, "dac1con1", "DAC1 8bit Voltage reference register 1", 0xff, m_daccon0);
    m_cpu_temp = new CPU_Temp("cpu_temperature", 30., "CPU die temperature");


    tmr0.link_cpu(this, nullptr,option_reg);
    //tmr0.link_cpu(this, m_porta, 4, option_reg);
    tmr0.start(0);
    tmr0.set_t1gcon(&t1con_g.t1gcon);
    set_mclr_pin(4);

    ((INTCON_14_PIR *)intcon)->write_mask = 0xfe;

    pir1 = new PIR1v1822(this,"pir1","Peripheral Interrupt Register",intcon, &pie1);
    pir2 = new PIR2v1822(this,"pir2","Peripheral Interrupt Register",intcon, &pie2);
    pir3 = new PIR3v178x(this,"pir3","Peripheral Interrupt Register",intcon, &pie3);

    pir1->valid_bits = 0xff;
    pir1->writable_bits = 0xcf;
    pir2->valid_bits = pir2->writable_bits = 0xef;
    pir3->valid_bits = pir3->writable_bits = 0x37;

    clc1.set_dxs_data(1, 32, lcxd);
    clc1.set_dxs_data(2, 32, lcxd);
    clc1.set_dxs_data(3, 32, lcxd);
    clc1.set_dxs_data(4, 32, lcxd);
    clc2.set_dxs_data(1, 32, lcxd);
    clc2.set_dxs_data(2, 32, lcxd);
    clc2.set_dxs_data(3, 32, lcxd);
    clc2.set_dxs_data(4, 32, lcxd);
    clc3.set_dxs_data(1, 32, lcxd);
    clc3.set_dxs_data(2, 32, lcxd);
    clc3.set_dxs_data(3, 32, lcxd);
    clc3.set_dxs_data(4, 32, lcxd);

    comparator.cmxcon0[0] = new CMxCON0_PPS(this, "cm1con0", " Comparator C1 Control Register 0", 0, &comparator);
    comparator.cmxcon1[0] = new CMxCON1(this, "cm1con1", " Comparator C1 Control Register 1", 0, &comparator);
    comparator.cmout = new CMOUT(this, "cmout", "Comparator Output Register");
    comparator.cmxcon0[1] = new CMxCON0_PPS(this, "cm2con0", " Comparator C2 Control Register 0", 1, &comparator);
    comparator.cmxcon1[1] = new CMxCON1(this, "cm2con1", " Comparator C2 Control Register 1", 1, &comparator);
    comparator.attach_cda_fvr(fvrcon.get_node_cvref());
    ram_size = 1024;
}
P16F170x::~P16F170x()
{
    unassignMCLRPin();
    delete_file_registers(0x20, 0x7f);
    unsigned int ram = ram_size - 96; // first 96 bytes already added
    unsigned int add;
    for(add = 0x80; ram >= 80; add += 0x80)
    {
        ram -= 80;
        delete_file_registers(add + 0x20, add + 0x6f);
    }
    if (ram > 0)
        delete_file_registers(add + 0x20, add + 0x20 + ram -1);

    adcon1.detach_fvr();
    adcon1.detach_DAC();
    comparator.detach_fvr();
    m_daccon0->detach_fvr();
    delete_sfr_register(m_intpps);
    delete_sfr_register(m_ccp1pps);
    delete_sfr_register(m_ccp2pps);
    delete_sfr_register(m_coginpps);
    delete_sfr_register(m_t0ckipps);
    delete_sfr_register(m_t1ckipps);
    delete_sfr_register(m_sspclkpps);
    delete_sfr_register(m_sspdatpps);
    delete_sfr_register(m_sspsspps);
    delete_sfr_register(m_rxpps);
    delete_sfr_register(m_ckpps);
    delete_sfr_register(m_clcin0pps);
    delete_sfr_register(m_clcin1pps);
    delete_sfr_register(m_clcin2pps);
    delete_sfr_register(m_clcin3pps);
    delete_sfr_register(m_t1gpps);
    delete_sfr_register(m_ra0pps);
    delete_sfr_register(m_ra1pps);
    delete_sfr_register(m_ra2pps);
    delete_sfr_register(m_ra4pps);
    delete_sfr_register(m_ra5pps);
    delete_sfr_register(m_rc0pps);
    delete_sfr_register(m_rc1pps);
    delete_sfr_register(m_rc2pps);
    delete_sfr_register(m_rc3pps);
    delete_sfr_register(m_rc4pps);
    delete_sfr_register(m_rc5pps);
    delete_sfr_register(m_iocap);
    delete_sfr_register(m_iocan);
    delete_sfr_register(m_iocaf);
    delete_sfr_register(m_ioccp);
    delete_sfr_register(m_ioccn);
    delete_sfr_register(m_ioccf);
    delete_sfr_register(m_odcona);
    delete_sfr_register(m_odconc);
    delete_sfr_register(m_inlvla);
    delete_sfr_register(m_inlvlc);
    delete_sfr_register(m_daccon0);
    delete_sfr_register(m_daccon1);

    delete_sfr_register(m_trisa);
    delete_sfr_register(m_porta);
    delete_sfr_register(m_lata);
    delete_sfr_register(m_wpua);
    delete_sfr_register(m_portc);
    delete_sfr_register(m_trisc);
    delete_sfr_register(m_latc);
    delete_sfr_register(m_wpuc);

    remove_sfr_register(&osctune);
    remove_sfr_register(&zcd1con);
    remove_sfr_register(&ppslock);
    remove_sfr_register(&clcdata);
    remove_sfr_register(&clc1.clcxcon);
    remove_sfr_register(&clc1.clcxpol);
    remove_sfr_register(&clc1.clcxsel0);
    remove_sfr_register(&clc1.clcxsel1);
    remove_sfr_register(&clc1.clcxsel2);
    remove_sfr_register(&clc1.clcxsel3);
    remove_sfr_register(&clc1.clcxgls0);
    remove_sfr_register(&clc1.clcxgls1);
    remove_sfr_register(&clc1.clcxgls2);
    remove_sfr_register(&clc1.clcxgls3);
    remove_sfr_register(&clc2.clcxcon);
    remove_sfr_register(&clc2.clcxpol);
    remove_sfr_register(&clc2.clcxsel0);
    remove_sfr_register(&clc2.clcxsel1);
    remove_sfr_register(&clc2.clcxsel2);
    remove_sfr_register(&clc2.clcxsel3);
    remove_sfr_register(&clc2.clcxgls0);
    remove_sfr_register(&clc2.clcxgls1);
    remove_sfr_register(&clc2.clcxgls2);
    remove_sfr_register(&clc2.clcxgls3);
    remove_sfr_register(&clc3.clcxcon);
    remove_sfr_register(&clc3.clcxpol);
    remove_sfr_register(&clc3.clcxsel0);
    remove_sfr_register(&clc3.clcxsel1);
    remove_sfr_register(&clc3.clcxsel2);
    remove_sfr_register(&clc3.clcxsel3);
    remove_sfr_register(&clc3.clcxgls0);
    remove_sfr_register(&clc3.clcxgls1);
    remove_sfr_register(&clc3.clcxgls2);
    remove_sfr_register(&clc3.clcxgls3);
    remove_sfr_register(&tmr0);

    remove_sfr_register(&tmr1l);
    remove_sfr_register(&tmr1h);
    remove_sfr_register(&t1con_g);
    remove_sfr_register(&t1con_g.t1gcon);

    remove_sfr_register(&tmr2);
    remove_sfr_register(&pr2);
    remove_sfr_register(&t2con);
    remove_sfr_register(&tmr4);
    remove_sfr_register(&pr4);
    remove_sfr_register(&t4con);
    remove_sfr_register(&tmr6);
    remove_sfr_register(&pr6);
    remove_sfr_register(&t6con);
    remove_sfr_register(&opa1con);
    remove_sfr_register(&opa2con);
    remove_sfr_register(&usart.spbrg);
    remove_sfr_register(&usart.spbrgh);
    remove_sfr_register(&usart.rcsta);
    remove_sfr_register(&usart.txsta);
    remove_sfr_register(&usart.baudcon);
    delete_sfr_register(usart.rcreg);
    delete_sfr_register(usart.txreg);

    remove_sfr_register(&slrcona);
    remove_sfr_register(&slrconc);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&ccpr1l);
    remove_sfr_register(&ccpr1h);
    remove_sfr_register(&ccp1con);
    remove_sfr_register(&ccpr2l);
    remove_sfr_register(&ccpr2h);
    remove_sfr_register(&ccp2con);
    remove_sfr_register(&ccptmrs);
    remove_sfr_register(&pwm3con);
    remove_sfr_register(&pwm3dcl);
    remove_sfr_register(&pwm3dch);
    remove_sfr_register(&pwm4con);
    remove_sfr_register(&pwm4dcl);
    remove_sfr_register(&pwm4dch);
    remove_sfr_register(&pie1);
    remove_sfr_register(&pie2);
    remove_sfr_register(&pie3);
    remove_sfr_register(&adresl);
    remove_sfr_register(&adresh);
    remove_sfr_register(&adcon0);
    remove_sfr_register(&adcon1);
    remove_sfr_register(&adcon2);
    remove_sfr_register(&borcon);
    remove_sfr_register(&fvrcon);
    remove_sfr_register(&ansela);
    remove_sfr_register(&anselc);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&cog.cogxphr);
    remove_sfr_register(&cog.cogxphf);
    remove_sfr_register(&cog.cogxblkr);
    remove_sfr_register(&cog.cogxblkf);
    remove_sfr_register(&cog.cogxdbr);
    remove_sfr_register(&cog.cogxdbf);
    remove_sfr_register(&cog.cogxcon0);
    remove_sfr_register(&cog.cogxcon1);
    remove_sfr_register(&cog.cogxris);
    remove_sfr_register(&cog.cogxrsim);
    remove_sfr_register(&cog.cogxfis);
    remove_sfr_register(&cog.cogxfsim);
    remove_sfr_register(&cog.cogxasd0);
    remove_sfr_register(&cog.cogxasd1);
    remove_sfr_register(&cog.cogxstr);

    remove_sfr_register(option_reg);
    remove_sfr_register(osccon);
    remove_sfr_register(&oscstat);

    remove_sfr_register(comparator.cmxcon0[0]);
    remove_sfr_register(comparator.cmxcon1[0]);
    remove_sfr_register(comparator.cmout);
    remove_sfr_register(comparator.cmxcon0[1]);
    remove_sfr_register(comparator.cmxcon1[1]);
    delete_sfr_register(pir1);
    delete_sfr_register(pir2);
    delete_sfr_register(pir3);
    delete e;
    delete m_cpu_temp;
}
void P16F170x::program_memory_wp(unsigned int mode)
{
    switch(mode)
    {
    case 3:	// no write protect
        get_eeprom()->set_prog_wp(0x0);
        break;

    case 2: // write protect 0000-01ff
        get_eeprom()->set_prog_wp(0x0200);
        break;

    case 1: // write protect 0000-03ff
        get_eeprom()->set_prog_wp(0x0400);
        break;

    case 0: // write protect 0000-07ff
        get_eeprom()->set_prog_wp(0x0800);
        break;

    default:
        printf("%s unexpected mode %u\n", __FUNCTION__, mode);
        break;
    }

}
void P16F170x::oscillator_select(unsigned int cfg_word1, bool clkout)
{
    unsigned int mask = 0x1f;

    unsigned int fosc = cfg_word1 & (FOSC0|FOSC1|FOSC2);

    osccon->set_config_irc(fosc == 4);
    osccon->set_config_xosc(fosc < 3);
    osccon->set_config_ieso(cfg_word1 & IESO);
    set_int_osc(false);
    switch(fosc)
    {
    case 0:	//LP oscillator: low power crystal
    case 1:	//XT oscillator: Crystal/resonator
    case 2:	//HS oscillator: High-speed crystal/resonator
        (m_porta->getPin(4))->newGUIname("OSC2");
        (m_porta->getPin(5))->newGUIname("OSC1");
        mask = 0x0f;

        break;

    case 3:	//EXTRC oscillator External RC circuit connected to CLKIN pin
        (m_porta->getPin(5))->newGUIname("CLKIN");
        mask = 0x1f;
        if(clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }
        break;

    case 4:	//INTOSC oscillator: I/O function on CLKIN pin
        set_int_osc(true);
        mask = 0x3f;
        if(clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x2f;
        }
        (m_porta->getPin(5))->newGUIname((m_porta->getPin(5))->name().c_str());
        break;

    case 5:	//ECL: External Clock, Low-Power mode (0-0.5 MHz): on CLKIN pin
        mask = 0x1f;
        if(clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }
        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;

    case 6:	//ECM: External Clock, Medium-Power mode (0.5-4 MHz): on CLKIN pin
        mask = 0x1f;
        if(clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }
        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;

    case 7:	//ECH: External Clock, High-Power mode (4-32 MHz): on CLKIN pin
        mask = 0x1f;
        if(clkout)
        {
            (m_porta->getPin(4))->newGUIname("CLKOUT");
            mask = 0x0f;
        }
        (m_porta->getPin(5))->newGUIname("CLKIN");
        break;
    };
    ansela.setValidBits(0x17 & mask);
    m_porta->setEnableMask(mask);
}

void P16F170x::create_sfr_map()
{
    pir_set_2_def.set_pir1(pir1);
    pir_set_2_def.set_pir2(pir2);
    pir_set_2_def.set_pir3(pir3);

    add_file_registers(0x20, 0x7f, 0x00);
    unsigned int ram = ram_size - 96; // first 96 bytes already added
    unsigned int add;
    for(add = 0x80; ram >= 80; add += 0x80)
    {
        ram -= 80;
        add_file_registers(add + 0x20, add + 0x6f, 0x00);
    }
    if (ram > 0)
        add_file_registers(add + 0x20, add + 0x20 + ram -1, 0x00);

    m_intpps = new xxxPPS(&pps, this, "intpps", "INT PPS input selection", 0x1f, &int_pin, 0);
    m_ccp1pps = new xxxPPS(&pps, this, "ccp1pps", "CCP1 PPS input selection", 0x1f,
                           &ccp1con, CCPCON::CCP_PIN);
    m_ccp2pps = new xxxPPS(&pps, this, "ccp2pps", "CCP2 PPS input selection", 0x1f,
                           &ccp2con, CCPCON::CCP_PIN);
    m_coginpps = new xxxPPS(&pps, this, "coginpps", "COG IN  PPS input selection", 0x1f,
                            &cog, 4);
    m_t0ckipps = new xxxPPS(&pps, this, "t0ckipps", "T0CKI PPS input selection", 0x1f,
                            &tmr0, 0);
    m_t1ckipps = new xxxPPS(&pps, this, "t1ckipps", "T1CKI PPS input selection", 0x1f,
                            &tmr1l, 0);
    m_t1gpps = new xxxPPS(&pps, this, "t1gpps", "T1G PPS input selection", 0x1f,
                          &t1con_g.t1gcon, 0);
    m_sspclkpps = new xxxPPS(&pps, this, "sspclkpps", "SSP SCK PPS input selection", 0x1f,
                             &ssp, SSP1_MODULE::SCK_IN_PIN);
    m_sspdatpps = new xxxPPS(&pps, this, "sspdatpps", "SSP DAT PPS input selection", 0x1f,
                             &ssp, SSP1_MODULE::SDI_PIN);
    m_sspsspps = new xxxPPS(&pps, this, "sspsspps", "SSP SS PPS input selection", 0x1f,
                            &ssp, SSP1_MODULE::SS_PIN);
    m_rxpps = new xxxPPS(&pps, this, "rxpps", "USART RX PPS input selection", 0x1f,
                         &usart, USART_MODULE::RX_PIN);
    m_ckpps = new xxxPPS(&pps, this, "ckpps", "USART CK PPS input selection", 0x1f,
                         &usart, USART_MODULE::CK_PIN);
    m_clcin0pps = new xxxPPS(&pps, this, "clcin0pps", "CLC IN0 PPS input selection", 0x1f,
                             &clcdata, CLC_BASE::CLCin0_PIN);
    m_clcin1pps = new xxxPPS(&pps, this, "clcin1pps", "CLC IN1 PPS input selection", 0x1f,
                             &clcdata, CLC_BASE::CLCin1_PIN);
    m_clcin2pps = new xxxPPS(&pps, this, "clcin2pps", "CLC IN2 PPS input selection", 0x1f,
                             &clcdata, CLC_BASE::CLCin2_PIN);
    m_clcin3pps = new xxxPPS(&pps, this, "clcin3pps", "CLC IN3 PPS input selection", 0x1f,
                             &clcdata, CLC_BASE::CLCin3_PIN);
    m_ra0pps = new RxyPPS(&pps, &(*m_porta)[0], this, "ra0pps", "RA0 PPS output selection");
    m_ra1pps = new RxyPPS(&pps, &(*m_porta)[1], this, "ra1pps", "RA1 PPS output selection");
    m_ra2pps = new RxyPPS(&pps, &(*m_porta)[2], this, "ra2pps", "RA2 PPS output selection");
    m_ra4pps = new RxyPPS(&pps, &(*m_porta)[4], this, "ra4pps", "RA4 PPS output selection");
    m_ra5pps = new RxyPPS(&pps, &(*m_porta)[5], this, "ra5pps", "RA5 PPS output selection");
    m_rc0pps = new RxyPPS(&pps, &(*m_portc)[0], this, "rc0pps", "RC0 PPS output selection");
    m_rc1pps = new RxyPPS(&pps, &(*m_portc)[1], this, "rc1pps", "RC1 PPS output selection");
    m_rc2pps = new RxyPPS(&pps, &(*m_portc)[2], this, "rc2pps", "RC2 PPS output selection");
    m_rc3pps = new RxyPPS(&pps, &(*m_portc)[3], this, "rc3pps", "RC3 PPS output selection");
    m_rc4pps = new RxyPPS(&pps, &(*m_portc)[4], this, "rc4pps", "RC4 PPS output selection");
    m_rc5pps = new RxyPPS(&pps, &(*m_portc)[5], this, "rc5pps", "RC5 PPS output selection");
    add_sfr_register(m_porta, 0x0c);
    add_sfr_register(m_portc, 0x0e);
    add_sfr_registerR(pir1,    0x11, RegisterValue(0,0),"pir1");
    add_sfr_registerR(pir2,    0x12, RegisterValue(0,0),"pir2");
    add_sfr_registerR(pir3,    0x13, RegisterValue(0,0),"pir3");
    add_sfr_register(&tmr0,   0x15);

    add_sfr_register(&tmr1l,  0x16, RegisterValue(0,0),"tmr1l");
    add_sfr_register(&tmr1h,  0x17, RegisterValue(0,0),"tmr1h");
    add_sfr_register(&t1con_g,  0x18, RegisterValue(0,0));
    add_sfr_register(&t1con_g.t1gcon, 0x19, RegisterValue(0,0));

    add_sfr_registerR(&tmr2,   0x1a, RegisterValue(0,0));
    add_sfr_registerR(&pr2,    0x1b, RegisterValue(0,0));
    add_sfr_registerR(&t2con,  0x1c, RegisterValue(0,0));

    add_sfr_register(m_trisa, 0x8c, RegisterValue(0x3f,0));
    add_sfr_register(m_trisc, 0x8e, RegisterValue(0x3f,0));

    pcon.valid_bits = 0xcf;
    add_sfr_register(option_reg, 0x95, RegisterValue(0xff,0));
    add_sfr_registerR(osccon,     0x99, RegisterValue(0x38,0));
    add_sfr_register(&oscstat,    0x9a, RegisterValue(0,0));

    intcon_reg.set_pir_set(get_pir_set());


    tmr1l.tmrh = &tmr1h;
    tmr1l.t1con = &t1con_g;
    tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v1::TMR1IF));

    tmr1h.tmrl  = &tmr1l;
    t1con_g.tmrl  = &tmr1l;
    t1con_g.t1gcon.set_tmrl(&tmr1l);
    t1con_g.t1gcon.setInterruptSource(new InterruptSource(pir1, PIR1v1822::TMR1GIF));

    tmr1l.setIOpin(&(*m_porta)[5]);
    t1con_g.t1gcon.setGatepin(&(*m_porta)[3]);

    add_sfr_registerR(&pie1,   0x91, RegisterValue(0,0));
    add_sfr_registerR(&pie2,   0x92, RegisterValue(0,0));
    add_sfr_registerR(&pie3,   0x93, RegisterValue(0,0));
    add_sfr_registerR(&osctune,0x98, RegisterValue(0,0));
    add_sfr_register(&adresl,  0x9b);
    add_sfr_register(&adresh,  0x9c);
    add_sfr_registerR(&adcon0, 0x9d, RegisterValue(0x00,0));
    add_sfr_registerR(&adcon1, 0x9e, RegisterValue(0x00,0));
    add_sfr_registerR(&adcon2, 0x9f, RegisterValue(0x00,0));

    add_sfr_register(m_lata,    0x10c);
    add_sfr_register(m_latc, 0x10e);
    add_sfr_registerR(comparator.cmxcon0[0], 0x111, RegisterValue(0x04,0));
    add_sfr_registerR(comparator.cmxcon1[0], 0x112, RegisterValue(0x00,0));
    add_sfr_registerR(comparator.cmxcon0[1], 0x113, RegisterValue(0x04,0));
    add_sfr_registerR(comparator.cmxcon1[1], 0x114, RegisterValue(0x00,0));
    add_sfr_registerR(comparator.cmout,      0x115, RegisterValue(0x00,0));
    add_sfr_register(&borcon,   0x116, RegisterValue(0x80,0));
    add_sfr_register(&fvrcon,   0x117, RegisterValue(0x00,0));
    add_sfr_registerR(m_daccon0, 0x118, RegisterValue(0x00,0));
    add_sfr_registerR(m_daccon1, 0x119, RegisterValue(0x00,0));
    add_sfr_registerR(&zcd1con, 0x11c, RegisterValue(0x00,0));
    zcd1con.setIOpin(&(*m_porta)[2]);
    zcd1con.setInterruptSource(new InterruptSource(pir3, 1<<4));
    clc1.set_zcd(&zcd1con);
    clc2.set_zcd(&zcd1con);
    clc3.set_zcd(&zcd1con);
    add_sfr_registerR(&ansela,   0x18c, RegisterValue(0x17,0));
    add_sfr_registerR(&anselc,   0x18e, RegisterValue(0xff,0));
    get_eeprom()->get_reg_eedata()->new_name("pmdatl");
    get_eeprom()->get_reg_eedatah()->new_name("pmdath");
    add_sfr_registerR(get_eeprom()->get_reg_eeadr(), 0x191, RegisterValue(0,0), "pmadrl");
    add_sfr_registerR(get_eeprom()->get_reg_eeadrh(), 0x192, RegisterValue(0,0), "pmadrh");
    add_sfr_register(get_eeprom()->get_reg_eedata(),  0x193);
    add_sfr_register(get_eeprom()->get_reg_eedatah(),  0x194);
    get_eeprom()->get_reg_eecon1()->set_always_on(1<<7);
    add_sfr_registerR(get_eeprom()->get_reg_eecon1(),  0x195, RegisterValue(0x80,0), "pmcon1");
    add_sfr_registerR(get_eeprom()->get_reg_eecon2(),  0x196, RegisterValue(0,0), "pmcon2");
    usart.initialize(pir1,
                     &(*m_porta)[0], // TX pin
                     & (*m_porta)[1], // RX pin
                     new _TXREG(this, "tx1reg", "USART Transmit Register", &usart),
                     new _RCREG(this, "rc1reg", "USART Receiver Register", &usart));
    usart.set_eusart(true);
    add_sfr_register(usart.rcreg,    0x199, RegisterValue(0, 0), "rc1reg");
    add_sfr_register(usart.txreg,    0x19a, RegisterValue(0, 0), "tx1reg");
    add_sfr_register(&usart.spbrg,   0x19b, RegisterValue(0, 0), "sp1brgl");
    add_sfr_register(&usart.spbrgh,  0x19c, RegisterValue(0, 0), "sp1brgh");
    add_sfr_register(&usart.rcsta,   0x19d, RegisterValue(0, 0), "rc1sta");
    add_sfr_register(&usart.txsta,   0x19e, RegisterValue(2, 0), "tx1sta");
    add_sfr_register(&usart.baudcon, 0x19f, RegisterValue(0x40, 0), "baud1con");


    add_sfr_register(m_wpua,     0x20c, RegisterValue(0xff,0),"wpua");

    add_sfr_registerR(&ssp.sspbuf,  0x211, RegisterValue(0,0),"ssp1buf");
    add_sfr_registerR(&ssp.sspadd,  0x212, RegisterValue(0,0),"ssp1add");
    add_sfr_registerR(ssp.sspmsk, 0x213, RegisterValue(0xff,0),"ssp1msk");
    add_sfr_registerR(&ssp.sspstat, 0x214, RegisterValue(0,0),"ssp1stat");
    add_sfr_registerR(&ssp.sspcon,  0x215, RegisterValue(0,0),"ssp1con");
    add_sfr_registerR(&ssp.sspcon2, 0x216, RegisterValue(0,0),"ssp1con2");
    add_sfr_registerR(&ssp.ssp1con3, 0x217, RegisterValue(0,0),"ssp1con3");
    add_sfr_register(m_odcona, 0x28c, RegisterValue(0,0),"odcona");
    add_sfr_register(m_odconc, 0x28e, RegisterValue(0,0),"odconc");

    add_sfr_register(&ccpr1l, 0x291, RegisterValue(0,0), "ccpr1l");
    add_sfr_register(&ccpr1h, 0x292, RegisterValue(0,0), "ccpr1h");
    add_sfr_registerR(&ccp1con, 0x293, RegisterValue(0,0), "ccp1con");

    add_sfr_register(&ccpr2l, 0x298, RegisterValue(0,0), "ccpr2l");
    add_sfr_register(&ccpr2h, 0x299, RegisterValue(0,0), "ccpr2h");
    add_sfr_registerR(&ccp2con, 0x29a, RegisterValue(0,0), "ccp2con");
    add_sfr_registerR(&ccptmrs, 0x29e, RegisterValue(0,0));

    ccp1con.setIOpin(&(*m_portc)[5]);
    ccp1con.setCrosslinks(&ccpr1l, pir1, PIR1v1822::CCP1IF, &tmr2);
    ccpr1l.ccprh  = &ccpr1h;
    ccpr1l.tmrl   = &tmr1l;
    ccpr1h.ccprl  = &ccpr1l;

    ccp2con.setIOpin(&(*m_portc)[3]);
    ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v1822::CCP2IF, &tmr2);
    ccpr2l.ccprh  = &ccpr2h;
    ccpr2l.tmrl   = &tmr1l;
    ccpr2h.ccprl  = &ccpr2l;
    add_sfr_registerR(&slrcona, 0x30c, RegisterValue(0,0));
    add_sfr_registerR(&slrconc, 0x30e, RegisterValue(0,0));
    add_sfr_register(m_inlvla, 0x38c, RegisterValue(0,0),"inlvla");
    add_sfr_register(m_inlvlc, 0x38e, RegisterValue(0,0),"inlvlc");
    add_sfr_registerR(m_iocap, 0x391, RegisterValue(0,0),"iocap");
    add_sfr_registerR(m_iocan, 0x392, RegisterValue(0,0),"iocan");
    add_sfr_registerR(m_iocaf, 0x393, RegisterValue(0,0),"iocaf");
    m_iocaf->set_intcon(intcon);

    add_sfr_registerR(m_ioccp, 0x397, RegisterValue(0,0),"ioccp");
    add_sfr_registerR(m_ioccn, 0x398, RegisterValue(0,0),"ioccn");
    add_sfr_registerR(m_ioccf, 0x399, RegisterValue(0,0),"ioccf");
    m_ioccf->set_intcon(intcon);


    add_sfr_register(&tmr4,  0x415, RegisterValue(0,0));
    add_sfr_register(&pr4,  0x416, RegisterValue(0xff,0));
    add_sfr_registerR(&t4con,  0x417, RegisterValue(0,0));

    add_sfr_register(&tmr6,  0x41c, RegisterValue(0,0));
    add_sfr_register(&pr6,  0x41d, RegisterValue(0xff,0));
    add_sfr_registerR(&t6con,  0x41e, RegisterValue(0,0));

    add_sfr_registerR(&opa1con,  0x511, RegisterValue(0,0));
    add_sfr_registerR(&opa2con,  0x515, RegisterValue(0,0));
    opa1con.set_pins(&(*m_portc)[0], &(*m_portc)[1], &(*m_portc)[2]);
    opa2con.set_pins(&(*m_portc)[5], &(*m_portc)[4], &(*m_portc)[3]);

    add_sfr_registerR(&pwm3dcl,  0x617, RegisterValue(0,0));
    add_sfr_register(&pwm3dch,  0x618, RegisterValue(0,0));
    add_sfr_registerR(&pwm3con,  0x619, RegisterValue(0,0));
    add_sfr_registerR(&pwm4dcl,  0x61a, RegisterValue(0,0));
    add_sfr_register(&pwm4dch,  0x61b, RegisterValue(0,0));
    add_sfr_registerR(&pwm4con,  0x61c, RegisterValue(0,0));

    ccptmrs.set_tmr246(&tmr2, &tmr4, &tmr6);
    ccptmrs.set_ccp(&ccp1con, &ccp2con, &pwm3con, &pwm4con);

    add_sfr_register(&cog.cogxphr, 0x691, RegisterValue(0,0));
    add_sfr_register(&cog.cogxphf, 0x692, RegisterValue(0,0));
    add_sfr_register(&cog.cogxblkr, 0x693, RegisterValue(0,0));
    add_sfr_register(&cog.cogxblkf, 0x694, RegisterValue(0,0));
    add_sfr_register(&cog.cogxdbr, 0x695, RegisterValue(0,0));
    add_sfr_register(&cog.cogxdbf, 0x696, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxcon0, 0x697, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxcon1, 0x698, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxris, 0x699, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxrsim, 0x69a, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxfis, 0x69b, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxfsim, 0x69c, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxasd0, 0x69d, RegisterValue(0x14,0));
    add_sfr_registerR(&cog.cogxasd1, 0x69e, RegisterValue(0,0));
    add_sfr_registerR(&cog.cogxstr, 0x69f, RegisterValue(0x01,0));

    add_sfr_register(&ppslock, 0xe0f, RegisterValue(0,0));

    add_sfr_register(m_intpps, 0xe10, RegisterValue(0x02,0));
    add_sfr_register(m_t0ckipps, 0xe11, RegisterValue(0x02,0));
    add_sfr_register(m_t1ckipps, 0xe12, RegisterValue(0x05,0));
    add_sfr_register(m_t1gpps, 0xe13, RegisterValue(0x04,0));
    add_sfr_register(m_ccp1pps, 0xe14, RegisterValue(0x15,0));
    add_sfr_register(m_ccp2pps, 0xe15, RegisterValue(0x13,0));
    add_sfr_register(m_coginpps, 0xe17, RegisterValue(0x02,0));
    add_sfr_register(m_clcin0pps, 0xe28, RegisterValue(0x13,0));
    add_sfr_register(m_clcin1pps, 0xe29, RegisterValue(0x14,0));
    add_sfr_register(m_clcin2pps, 0xe2a, RegisterValue(0x11,0));
    add_sfr_register(m_clcin3pps, 0xe2b, RegisterValue(0x05,0));
    add_sfr_register(m_ra0pps, 0xe90, RegisterValue(0,0));
    add_sfr_register(m_ra1pps, 0xe91, RegisterValue(0,0));
    add_sfr_register(m_ra2pps, 0xe92, RegisterValue(0,0));
    add_sfr_register(m_ra4pps, 0xe94, RegisterValue(0,0));
    add_sfr_register(m_ra5pps, 0xe95, RegisterValue(0,0));

    add_sfr_register(m_rc0pps, 0xea0, RegisterValue(0,0));
    add_sfr_register(m_rc1pps, 0xea1, RegisterValue(0,0));
    add_sfr_register(m_rc2pps, 0xea2, RegisterValue(0,0));
    add_sfr_register(m_rc3pps, 0xea3, RegisterValue(0,0));
    add_sfr_register(m_rc4pps, 0xea4, RegisterValue(0,0));
    add_sfr_register(m_rc5pps, 0xea5, RegisterValue(0,0));

    add_sfr_registerR(&clcdata, 0xf0f, RegisterValue(0,0));
    add_sfr_registerR(&clc1.clcxcon, 0xf10, RegisterValue(0,0), "clc1con");
    add_sfr_register(&clc1.clcxpol, 0xf11, RegisterValue(0,0), "clc1pol");
    add_sfr_register(&clc1.clcxsel0, 0xf12, RegisterValue(0,0), "clc1sel0");
    add_sfr_register(&clc1.clcxsel1, 0xf13, RegisterValue(0,0), "clc1sel1");
    add_sfr_register(&clc1.clcxsel2, 0xf14, RegisterValue(0,0), "clc1sel2");
    add_sfr_register(&clc1.clcxsel3, 0xf15, RegisterValue(0,0), "clc1sel3");
    add_sfr_register(&clc1.clcxgls0, 0xf16, RegisterValue(0,0), "clc1gls0");
    add_sfr_register(&clc1.clcxgls1, 0xf17, RegisterValue(0,0), "clc1gls1");
    add_sfr_register(&clc1.clcxgls2, 0xf18, RegisterValue(0,0), "clc1gls2");
    add_sfr_register(&clc1.clcxgls3, 0xf19, RegisterValue(0,0), "clc1gls3");
    add_sfr_registerR(&clc2.clcxcon, 0xf1a, RegisterValue(0,0), "clc2con");
    add_sfr_register(&clc2.clcxpol, 0xf1b, RegisterValue(0,0), "clc2pol");
    add_sfr_register(&clc2.clcxsel0, 0xf1c, RegisterValue(0,0), "clc2sel0");
    add_sfr_register(&clc2.clcxsel1, 0xf1d, RegisterValue(0,0), "clc2sel1");
    add_sfr_register(&clc2.clcxsel2, 0xf1e, RegisterValue(0,0), "clc2sel2");
    add_sfr_register(&clc2.clcxsel3, 0xf1f, RegisterValue(0,0), "clc2sel3");
    add_sfr_register(&clc2.clcxgls0, 0xf20, RegisterValue(0,0), "clc2gls0");
    add_sfr_register(&clc2.clcxgls1, 0xf21, RegisterValue(0,0), "clc2gls1");
    add_sfr_register(&clc2.clcxgls2, 0xf22, RegisterValue(0,0), "clc2gls2");
    add_sfr_register(&clc2.clcxgls3, 0xf23, RegisterValue(0,0), "clc2gls3");
    add_sfr_registerR(&clc3.clcxcon, 0xf24, RegisterValue(0,0), "clc3con");
    add_sfr_register(&clc3.clcxpol, 0xf25, RegisterValue(0,0), "clc3pol");
    add_sfr_register(&clc3.clcxsel0, 0xf26, RegisterValue(0,0), "clc3sel0");
    add_sfr_register(&clc3.clcxsel1, 0xf27, RegisterValue(0,0), "clc3sel1");
    add_sfr_register(&clc3.clcxsel2, 0xf28, RegisterValue(0,0), "clc3sel2");
    add_sfr_register(&clc3.clcxsel3, 0xf29, RegisterValue(0,0), "clc3sel3");
    add_sfr_register(&clc3.clcxgls0, 0xf2a, RegisterValue(0,0), "clc3gls0");
    add_sfr_register(&clc3.clcxgls1, 0xf2b, RegisterValue(0,0), "clc3gls1");
    add_sfr_register(&clc3.clcxgls2, 0xf2c, RegisterValue(0,0), "clc3gls2");
    add_sfr_register(&clc3.clcxgls3, 0xf2d, RegisterValue(0,0), "clc3gls3");
    // RP - moved DXS setup to constructor to match 16F1503 and 10F32x

    clc1.frc = &frc;
    clc2.frc = &frc;
    clc3.frc = &frc;
    clc1.lfintosc = &lfintosc;
    clc2.lfintosc = &lfintosc;
    clc3.lfintosc = &lfintosc;
    clc1.hfintosc = &hfintosc;
    clc2.hfintosc = &hfintosc;
    clc3.hfintosc = &hfintosc;
    clcdata.set_clc(&clc1, &clc2, &clc3);
    clc1.set_clc(&clc1, &clc2, &clc3);
    clc2.set_clc(&clc1, &clc2, &clc3);
    clc3.set_clc(&clc1, &clc2, &clc3);
    frc.set_clc(&clc1, &clc2, &clc3);
    lfintosc.set_clc(&clc1, &clc2, &clc3);
    hfintosc.set_clc(&clc1, &clc2, &clc3);
    tmr0.set_clc(&clc1, 0);
    tmr0.set_clc(&clc2, 1);
    tmr0.set_clc(&clc3, 2);
    t1con_g.tmrl->m_clc[0] = tmr2.m_clc[0] = &clc1;
    t1con_g.tmrl->m_clc[1] = tmr2.m_clc[1] = &clc2;
    t1con_g.tmrl->m_clc[2] = tmr2.m_clc[2] = &clc3;
    comparator.m_clc[0] = &clc1;
    comparator.m_clc[1] = &clc2;
    comparator.m_clc[2] = &clc3;

    clc1.set_clcPins(nullptr, &(*m_portc)[3], &(*m_portc)[4], &(*m_portc)[1], &(*m_portc)[0]);
    clc2.set_clcPins(nullptr, &(*m_portc)[3], &(*m_portc)[4], &(*m_portc)[1], &(*m_portc)[0]);
    clc3.set_clcPins(nullptr, &(*m_portc)[3], &(*m_portc)[4], &(*m_portc)[1], &(*m_portc)[0]);
    clc1.setInterruptSource(new InterruptSource(pir3, 1));
    clc2.setInterruptSource(new InterruptSource(pir3, 2));
    clc3.setInterruptSource(new InterruptSource(pir3, 4));


    pps.set_output_source(0x04, &clc1, CLC_BASE::CLCout_PIN);
    pps.set_output_source(0x05, &clc2, CLC_BASE::CLCout_PIN);
    pps.set_output_source(0x06, &clc3, CLC_BASE::CLCout_PIN);
    pps.set_output_source(0x08, &cog, 0);
    pps.set_output_source(0x09, &cog, 1);
    pps.set_output_source(0x0a, &cog, 2);
    pps.set_output_source(0x0b, &cog, 3);
    pps.set_output_source(0x0c, &ccp1con, 0);
    pps.set_output_source(0x0d, &ccp2con, 0);
    pps.set_output_source(0x0e, &pwm3con, 0);
    pps.set_output_source(0x0f, &pwm4con, 0);
    pps.set_output_source(0x10, &ssp, SSP1_MODULE::SCK_PIN);
//SDA    pps.set_output_source(0x11, &ssp, SSP1_MODULE::SDA_PIN);
    pps.set_output_source(0x12, &ssp, SSP1_MODULE::SDO_PIN);
    pps.set_output_source(0x14, &usart, USART_MODULE::TX_PIN);
    pps.set_output_source(0x15, &usart, USART_MODULE::RX_PIN);
    pps.set_output_source(0x16, comparator.cmxcon0[0], 0);
    pps.set_output_source(0x17, comparator.cmxcon0[1], 0);

    tmr2.ssp_module[0] = &ssp;

    ssp.initialize(
        get_pir_set(),    // PIR
        &(*m_portc)[0],   // SCK
        &(*m_portc)[3],   // SS
        &(*m_portc)[2],   // SDO
        &(*m_portc)[1],    // SDI
        m_trisc,          // i2c tris port
        SSP_TYPE_MSSP1
    );
    if (pir1)
    {
        pir1->set_intcon(intcon);
        pir1->set_pie(&pie1);
    }
    pie1.setPir(pir1);
    pie2.setPir(pir2);
    pie3.setPir(pir3);
    t2con.tmr2 = &tmr2;
    tmr2.pir_set   = get_pir_set();
    tmr2.pr2    = &pr2;
    tmr2.t2con  = &t2con;
//    tmr2.add_ccp ( &pwm3con );
//    tmr2.add_ccp ( &pwm4con );

    t4con.tmr2 = &tmr4;
    tmr4.setInterruptSource(new InterruptSource(pir2, 1<<1));
    tmr4.pr2    = &pr4;
    tmr4.t2con  = &t4con;


    t6con.tmr2 = &tmr6;
    tmr6.setInterruptSource(new InterruptSource(pir2, 1<<2));
    tmr6.pr2    = &pr6;
    tmr6.t2con  = &t6con;

    pr2.tmr2    = &tmr2;
    pr4.tmr2    = &tmr4;
    pr6.tmr2    = &tmr6;

    comparator.p_cog = &cog;
    ccp1con.set_cog(&cog);
    ccp2con.set_cog(&cog);
    clc1.set_cog(&cog);
    pwm3con.set_pwmdc(&pwm3dcl, &pwm3dch);
    pwm3con.set_cog(&cog);
    pwm3con.set_clc(&clc1, 0);
    pwm3con.set_clc(&clc2, 1);
    pwm3con.set_clc(&clc3, 2);
    pwm4con.set_pwmdc(&pwm4dcl, &pwm4dch);
    pwm4con.set_clc(&clc1, 0);
    pwm4con.set_clc(&clc2, 1);
    pwm4con.set_clc(&clc3, 2);

    ansela.config(0x17, 0);
    ansela.setValidBits(0x17);
    ansela.setAdcon1(&adcon1);

    anselc.config(0x0f, 4);
    anselc.setValidBits(0x0f);
    anselc.setAdcon1(&adcon1);
    ansela.setAnsel(&anselc);
    anselc.setAnsel(&ansela);

    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    adcon0.setIntcon(intcon);
    adcon0.setA2DBits(10);
    adcon0.setPir(pir1);
    adcon0.setChannel_Mask(0x1f);
    adcon0.setChannel_shift(2);
    adcon0.setGo(1);
    adcon1.set_FVR_chan(0x1f);
    adcon1.attach_ad_fvr(fvrcon.get_node_adcvref(), 0x1f);
    adcon1.attach_DAC(m_daccon0->get_node_dacout(), 0x1e);
    adcon1.attach_Vt_fvr(fvrcon.get_node_Vtref(), 0x1d);
    adcon2.setAdcon0(&adcon0);

    tmr0.set_adcon2(&adcon2);

    adcon1.setAdcon0(&adcon0);
    adcon1.setNumberOfChannels(32); // not all channels are used
    adcon1.setIOPin(0, &(*m_porta)[0]);
    adcon1.setIOPin(1, &(*m_porta)[1]);
    adcon1.setIOPin(2, &(*m_porta)[2]);
    adcon1.setIOPin(3, &(*m_porta)[4]);
    adcon1.setIOPin(4, &(*m_portc)[0]);
    adcon1.setIOPin(5, &(*m_portc)[1]);
    adcon1.setIOPin(6, &(*m_portc)[2]);
    adcon1.setIOPin(7, &(*m_portc)[3]);
    adcon1.setValidBits(0xf7);
    adcon1.setVrefHiConfiguration(0, 0);

    comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[1], &(*m_portc)[1],  &(*m_portc)[2],  &(*m_portc)[3]);
    comparator.cmxcon1[1]->set_INpinNeg(&(*m_porta)[1], &(*m_portc)[1],  &(*m_portc)[2],  &(*m_portc)[3]);
    comparator.cmxcon1[0]->set_INpinPos(&(*m_porta)[0]);
    comparator.cmxcon1[1]->set_INpinPos(&(*m_portc)[0]);
    comparator.set_DAC_volt(0., CMxCON1::CM_AGND);
    comparator.Pmask[0] = CMxCON1::CM_PIN;
    comparator.Pmask[5] = CMxCON1::CM_DAC1;
    comparator.Pmask[6] = CMxCON1::CM_FVR;
    comparator.Pmask[7] = CMxCON1::CM_AGND;
    comparator.Nmask[0] = CMxCON1::CM_PIN;
    comparator.Nmask[1] = CMxCON1::CM_PIN;
    comparator.Nmask[2] = CMxCON1::CM_PIN;
    comparator.Nmask[3] = CMxCON1::CM_PIN;
    comparator.Nmask[6] = CMxCON1::CM_FVR;
    comparator.Nmask[7] = CMxCON1::CM_AGND;
#ifdef DEBUG
    for(int i=0; i<8; i++)
	printf("\ti=%d Pmask=%d Nmask=%d\n", i, comparator.Pmask[i], comparator.Nmask[i]);
#endif

    comparator.cmxcon0[0]->setBitMask(0x9f);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, (1<<5)));
    comparator.cmxcon0[1]->setBitMask(0x9f);
    comparator.cmxcon0[1]->setIntSrc(new InterruptSource(pir2, (1<<6)));
    comparator.cmxcon1[0]->setBitMask(0xff);
    comparator.cmxcon1[1]->setBitMask(0xff);

    comparator.assign_pir_set(get_pir_set());
    comparator.assign_t1gcon(&t1con_g.t1gcon);

    m_daccon0->set_adcon1(&adcon1);
    m_daccon0->attach_cda_fvr(fvrcon.get_node_cvref(), 0x1e);
    m_daccon0->setDACOUT(&(*m_porta)[0], &(*m_porta)[2]);

    osccon->set_osctune(&osctune);
    osccon->set_oscstat(&oscstat);
    osctune.set_osccon((OSCCON *)osccon);
    osccon->write_mask = 0xfb;
}


void P16F170x::option_new_bits_6_7(unsigned int bits)
{
    Dprintf(("P16F1705::option_new_bits_6_7 bits=%x\n", bits));
    m_porta->setIntEdge ( (bits & OPTION_REG::BIT6) == OPTION_REG::BIT6);
    m_portc->setIntEdge ( (bits & OPTION_REG::BIT6) == OPTION_REG::BIT6);
    m_wpua->set_wpu_pu ( (bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
    m_wpuc->set_wpu_pu ( (bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
}


P16F1705::P16F1705(const char *_name, const char *desc)
    : P16F170x(_name,desc)
{
    m_portc= new PicPortBRegister(this,"portc","", intcon, 8,0x3f);
    m_trisc = new PicTrisRegister(this,"trisc","", m_portc, false, 0x3f);
    m_latc  = new PicLatchRegister(this,"latc","",m_portc, 0x3f);
    m_ioccf = new IOCxF(this, "ioccf", "Interrupt-On-Change flag Register", 0x3f);
    m_ioccp = new IOC(this, "ioccp", "Interrupt-On-Change positive edge", 0x3f);
    m_ioccn = new IOC(this, "ioccn", "Interrupt-On-Change negative edge", 0x3f);
    m_wpuc = new WPU(this, "wpuc", "Weak Pull-up Register", m_portc, 0x3f);
    m_odconc = new ODCON(this, "odconc", "Open Drain Control Register C", m_portc, 0x3f);
    m_inlvlc = new INLVL(this, "inlvlc", "Input Level Control Register C", m_portc, 0x3f);
}

Processor * P16F1705::construct(const char *name)
{
    P16F1705 *p = new P16F1705(name);

    p->create(8192, 0x3055);

    p->create_invalid_registers ();
    p->create_symbols();
    return p;
}


void  P16F1705::create(int /* ram_top */ , int dev_id)
{
    create_iopin_map();

    osccon = new OSCCON_2(this, "osccon", "Oscillator Control Register");

    e = new EEPROM_EXTND(this, pir2);
    set_eeprom(e);
    e->initialize(0, 16, 16, 0x8000, true);
    e->set_intcon(intcon);
    e->get_reg_eecon1()->set_valid_bits(0x7f);


    pic_processor::create();
    create_sfr_map();

    // Set DeviceID
    if (m_configMemory && m_configMemory->getConfigWord(6))
        m_configMemory->getConfigWord(6)->set(dev_id);
}


void P16F1705::create_sfr_map()
{
    P16F170x::create_sfr_map();
    _14bit_e_processor::create_sfr_map();
    pps.set_ports(m_porta, nullptr, m_portc);
    add_sfr_register(m_wpuc,     0x20e, RegisterValue(0xff,0),"wpuc");
    add_sfr_register(m_sspclkpps, 0xe20, RegisterValue(0x10,0));
    add_sfr_register(m_sspdatpps, 0xe21, RegisterValue(0x11,0));
    add_sfr_register(m_sspsspps, 0xe22, RegisterValue(0x13,0));
    add_sfr_register(m_rxpps, 0xe24, RegisterValue(0x15,0));
    add_sfr_register(m_ckpps, 0xe25, RegisterValue(0x14,0));
}

void P16F1705::create_iopin_map()
{
    package = new Package(14);
    if(!package)
        return;

    // Now Create the package and place the I/O pins
    package->assign_pin(1, 0);	//Vdd
    package->assign_pin(2, m_porta->addPin(new IO_open_collector("porta5"),5));
    package->assign_pin(3, m_porta->addPin(new IO_open_collector("porta4"),4));
    package->assign_pin(4, m_porta->addPin(new IO_open_collector("porta3"),3));
    package->assign_pin(5, m_portc->addPin(new IO_open_collector("portc5"),5));
    package->assign_pin(6, m_portc->addPin(new IO_open_collector("portc4"),4));
    package->assign_pin(7, m_portc->addPin(new IO_open_collector("portc3"),3));

    package->assign_pin(8, m_portc->addPin(new IO_open_collector("portc2"),2));
    package->assign_pin(9, m_portc->addPin(new IO_open_collector("portc1"),1));
    package->assign_pin(10, m_portc->addPin(new IO_open_collector("portc0"),0));

    package->assign_pin(11, m_porta->addPin(new IO_open_collector("porta2"),2));
    package->assign_pin(12, m_porta->addPin(new IO_open_collector("porta1"),1));
    package->assign_pin(13, m_porta->addPin(new IO_open_collector("porta0"),0));
    package->assign_pin(14, 0);	// Vss
}


P16LF1705::P16LF1705(const char *_name, const char *desc)
    : P16F1705(_name,desc)
{
}

Processor * P16LF1705::construct(const char *name)
{
    P16LF1705 *p = new P16LF1705(name);

    p->create(8192, 0x3057);

    p->create_invalid_registers ();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}


P16F1709::P16F1709(const char *_name, const char *desc)
    : P16F170x(_name,desc),
      anselb(this, "anselb", "Analog Select port b"),
      slrconb(this, "slrconb", "Slew Rate Control port B"),
      m_rb4pps(nullptr), m_rb5pps(nullptr), m_rb6pps(nullptr),
      m_rb7pps(nullptr), m_rc6pps(nullptr), m_rc7pps(nullptr)
{
    m_portb= new PicPortBRegister(this,"portb","", intcon, 8,0xf0);
    m_trisb = new PicTrisRegister(this,"trisb","", m_portb, false, 0xf0);
    m_latb  = new PicLatchRegister(this,"latb","",m_portb, 0xf0);
    m_iocbf = new IOCxF(this, "iocbf", "Interrupt-On-Change flag Register", 0xf0);
    m_iocbp = new IOC(this, "iocbp", "Interrupt-On-Change positive edge", 0xf0);
    m_iocbn = new IOC(this, "iocbn", "Interrupt-On-Change negative edge", 0xf0);
    m_wpub = new WPU(this, "wpub", "Weak Pull-up Register", m_portb, 0xf0);
    m_odconb = new ODCON(this, "odconb", "Open Drain Control Register B", m_portb, 0xf0);
    m_inlvlb = new INLVL(this, "inlvlb", "Input Level Control Register B", m_portb, 0xf0);
    m_portc= new PicPortBRegister(this,"portc","", intcon, 8,0xff);
    m_trisc = new PicTrisRegister(this,"trisc","", m_portc, false, 0xff);
    m_latc  = new PicLatchRegister(this,"latc","",m_portc, 0xff);
    m_ioccf = new IOCxF(this, "ioccf", "Interrupt-On-Change flag Register", 0xff);
    m_ioccp = new IOC(this, "ioccp", "Interrupt-On-Change positive edge", 0xff);
    m_ioccn = new IOC(this, "ioccn", "Interrupt-On-Change negative edge", 0xff);
    m_wpuc = new WPU(this, "wpuc", "Weak Pull-up Register", m_portc, 0xff);
    m_odconc = new ODCON(this, "odconc", "Open Drain Control Register C", m_portc, 0xff);
    m_inlvlc = new INLVL(this, "inlvlc", "Input Level Control Register C", m_portc, 0xff);
}

P16F1709::~P16F1709()
{
    remove_sfr_register(&anselb);
    remove_sfr_register(&slrconb);
    delete_sfr_register(m_wpub);
    delete_sfr_register(m_portb);
    delete_sfr_register(m_trisb);
    delete_sfr_register(m_latb);
    delete_sfr_register(m_odconb);
    delete_sfr_register(m_inlvlb);
    delete_sfr_register(m_iocbp);
    delete_sfr_register(m_iocbn);
    delete_sfr_register(m_iocbf);
    delete_sfr_register(m_rb4pps);
    delete_sfr_register(m_rb5pps);
    delete_sfr_register(m_rb6pps);
    delete_sfr_register(m_rb7pps);
    delete_sfr_register(m_rc6pps);
    delete_sfr_register(m_rc7pps);
}
Processor * P16F1709::construct(const char *name)
{

    P16F1709 *p = new P16F1709(name);

    p->create(8192, 0x3054);

    p->create_invalid_registers ();
    p->create_symbols();
    return p;

}


void  P16F1709::create(int /* ram_top */ , int dev_id)
{
    create_iopin_map();

    osccon = new OSCCON_2(this, "osccon", "Oscillator Control Register");

    e = new EEPROM_EXTND(this, pir2);
    set_eeprom(e);
    e->initialize(0, 16, 16, 0x8000, true);
    e->set_intcon(intcon);
    e->get_reg_eecon1()->set_valid_bits(0x7f);

    pic_processor::create();
    create_sfr_map();

    // Set DeviceID
    if (m_configMemory && m_configMemory->getConfigWord(6))
        m_configMemory->getConfigWord(6)->set(dev_id);
}


void  P16F1709::create_sfr_map()
{
    P16F170x::create_sfr_map();
    _14bit_e_processor::create_sfr_map();
    add_sfr_register(m_portb, 0x0d);
    pps.set_ports(m_porta, m_portb, m_portc);
    m_rb4pps = new RxyPPS(&pps, &(*m_portb)[4], this, "rb4pps", "RB4 PPS output selection");
    m_rb5pps = new RxyPPS(&pps, &(*m_portb)[5], this, "rb5pps", "RB5 PPS output selection");
    m_rb6pps = new RxyPPS(&pps, &(*m_portb)[6], this, "rb6pps", "RB6 PPS output selection");
    m_rb7pps = new RxyPPS(&pps, &(*m_portb)[7], this, "rb7pps", "RB7 PPS output selection");
    m_rc6pps = new RxyPPS(&pps, &(*m_portc)[6], this, "rc6pps", "RC6 PPS output selection");
    m_rc7pps = new RxyPPS(&pps, &(*m_portc)[7], this, "rc7pps", "RC7 PPS output selection");
    adcon1.setIOPin(8, &(*m_portc)[6]);
    adcon1.setIOPin(9, &(*m_portc)[7]);
    adcon1.setIOPin(10, &(*m_portb)[4]);
    adcon1.setIOPin(11, &(*m_portb)[5]);
    anselc.setValidBits(0xcf);
    anselc.config(0xcf, 4);
    ansela.setAnsel(&anselb);
    ansela.setAnsel(&anselc);
    anselb.setAnsel(&ansela);
    anselb.setAnsel(&anselc);
    anselc.setAnsel(&ansela);
    anselc.setAnsel(&anselb);
    anselb.setValidBits(0x30);
    anselb.config(0x30, 10);
    anselb.setAdcon1(&adcon1);
    add_sfr_register(m_trisb,  0x8d, RegisterValue(0xf0,0));
    add_sfr_register(m_latb,   0x10d);
    add_sfr_registerR(&anselb, 0x18d, RegisterValue(0x30,0));
    add_sfr_register(m_wpub,   0x20d, RegisterValue(0xf0,0),"wpub");
    add_sfr_register(m_wpuc,   0x20e, RegisterValue(0xff,0),"wpuc");
    add_sfr_register(m_odconb, 0x28d, RegisterValue(0,0),"odconb");
    add_sfr_registerR(&slrconb, 0x30d, RegisterValue(0,0));
    add_sfr_register(m_inlvlb, 0x38d, RegisterValue(0,0));
    add_sfr_registerR(m_iocbp, 0x394, RegisterValue(0,0),"iocbp");
    add_sfr_registerR(m_iocbn, 0x395, RegisterValue(0,0),"iocbn");
    add_sfr_registerR(m_iocbf, 0x396, RegisterValue(0,0),"iocbf");
    m_iocbf->set_intcon(intcon);
    add_sfr_register(m_sspclkpps, 0xe20, RegisterValue(0x0e,0));
    add_sfr_register(m_sspdatpps, 0xe21, RegisterValue(0x0c,0));
    add_sfr_register(m_sspsspps, 0xe22, RegisterValue(0x16,0));
    add_sfr_register(m_rxpps, 0xe24, RegisterValue(0x0d,0));
    add_sfr_register(m_ckpps, 0xe25, RegisterValue(0x0f,0));
    add_sfr_register(m_rb4pps, 0xe9c, RegisterValue(0,0));
    add_sfr_register(m_rb5pps, 0xe9d, RegisterValue(0,0));
    add_sfr_register(m_rb6pps, 0xe9e, RegisterValue(0,0));
    add_sfr_register(m_rb7pps, 0xe9f, RegisterValue(0,0));
    add_sfr_register(m_rc6pps, 0xea6, RegisterValue(0,0));
    add_sfr_register(m_rc7pps, 0xea7, RegisterValue(0,0));
}
void P16F1709::create_iopin_map()
{

    package = new Package(20);
    if(!package)
        return;

    // Now Create the package and place the I/O pins
    package->assign_pin(1, 0);	//Vdd
    package->assign_pin(2, m_porta->addPin(new IO_open_collector("porta5"),5));
    package->assign_pin(3, m_porta->addPin(new IO_open_collector("porta4"),4));
    package->assign_pin(4, m_porta->addPin(new IO_open_collector("porta3"),3));
    package->assign_pin(5, m_portc->addPin(new IO_open_collector("portc5"),5));
    package->assign_pin(6, m_portc->addPin(new IO_open_collector("portc4"),4));
    package->assign_pin(7, m_portc->addPin(new IO_open_collector("portc3"),3));
    package->assign_pin(8, m_portc->addPin(new IO_open_collector("portc6"),6));
    package->assign_pin(9, m_portc->addPin(new IO_open_collector("portc7"),7));
    package->assign_pin(10, m_portb->addPin(new IO_open_collector("portb7"),7));

    package->assign_pin(11, m_portb->addPin(new IO_open_collector("portb6"),6));
    package->assign_pin(12, m_portb->addPin(new IO_open_collector("portb5"),5));
    package->assign_pin(13, m_portb->addPin(new IO_open_collector("portb4"),4));
    package->assign_pin(14, m_portc->addPin(new IO_open_collector("portc2"),2));
    package->assign_pin(15, m_portc->addPin(new IO_open_collector("portc1"),1));
    package->assign_pin(16, m_portc->addPin(new IO_open_collector("portc0"),0));

    package->assign_pin(17, m_porta->addPin(new IO_open_collector("porta2"),2));
    package->assign_pin(18, m_porta->addPin(new IO_open_collector("porta1"),1));
    package->assign_pin(19, m_porta->addPin(new IO_open_collector("porta0"),0));
    package->assign_pin(20, 0);	// Vss
}
P16LF1709::P16LF1709(const char *_name, const char *desc)
    : P16F1709(_name,desc)
{
}
Processor * P16LF1709::construct(const char *name)
{

    P16LF1709 *p = new P16LF1709(name);

    p->create(8192, 0x3056);

    p->create_invalid_registers ();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}
//========================================================================


P16F178x::P16F178x(const char *_name, const char *desc)
    : _14bit_e_processor(_name, desc),
      comparator(this),
      pie1(this, "pie1", "Peripheral Interrupt Enable"),
      pie2(this, "pie2", "Peripheral Interrupt Enable"),
      pie3(this, "pie3", "Peripheral Interrupt Enable"),
      pie4(this, "pie4", "Peripheral Interrupt Enable"),
      t2con(this, "t2con", "TMR2 Control"),
      pr2(this, "pr2", "TMR2 Period Register"),
      tmr2(this, "tmr2", "TMR2 Register"),
      t1con_g(this, "t1con", "TMR1 Control Register"),
      tmr1l(this, "tmr1l", "TMR1 Low"),
      tmr1h(this, "tmr1h", "TMR1 High"),
      ccp1con(this, "ccp1con", "Capture Compare Control"),
      ccpr1l(this, "ccpr1l", "Capture Compare 1 Low"),
      ccpr1h(this, "ccpr1h", "Capture Compare 1 High"),
      fvrcon(this, "fvrcon", "Voltage reference control register", 0xbf),
      borcon(this, "borcon", "Brown-out reset control register"),
      ansela(this, "ansela", "Analog Select port a"),
      anselb(this, "anselb", "Analog Select port b"),
      anselc(this, "anselc", "Analog Select port c"),
      adcon0(this, "adcon0", "A2D Control 0"),
      adcon1(this, "adcon1", "A2D Control 1"),
      adcon2(this, "adcon2", "A2D Control 2"),
      adresh(this, "adresh", "A2D Result High"),
      adresl(this, "adresl", "A2D Result Low"),
      osccon(0),
      osctune(this, "osctune", "Oscillator Tunning Register"),
      oscstat(this, "oscstat", "Oscillator Status Register"),
      wdtcon(this, "wdtcon", "Watch dog timer control", 0x3f),
      usart(this),
      ssp(this),
      apfcon1(this, "apfcon1", "Alternate Pin Function Control Register 1", 0xff),
      apfcon2(this, "apfcon2", "Alternate Pin Function Control Register 2", 0x07),
      pwm1con(this, "pwm1con", "Enhanced PWM Control Register"),
      ccp1as(this, "ccp1as", "CCP1 Auto-Shutdown Control Register"),
      pstr1con(this, "pstr1con", "Pulse Sterring Control Register"),
      vregcon(this, "vregcon", "Voltage Regulator Control Register")
{
    m_iocbf = new IOCxF(this, "iocbf", "Interrupt-On-Change flag Register");
    m_iocbp = new IOC(this, "iocbp", "Interrupt-On-Change positive edge");
    m_iocbn = new IOC(this, "iocbn", "Interrupt-On-Change negative edge");
    m_portb = new PicPortIOCRegister(this, "portb", "", intcon, m_iocbp, m_iocbn, m_iocbf, 8, 0xff);
    m_trisb = new PicTrisRegister(this, "trisb", "", m_portb, false, 0xff);
    m_latb  = new PicLatchRegister(this, "latb", "", m_portb, 0xff);
    m_wpub = new WPU(this, "wpub", "Weak Pull-up Register", m_portb, 0xff);
    m_ioccf = new IOCxF(this, "ioccf", "Interrupt-On-Change flag Register");
    m_ioccp = new IOC(this, "ioccp", "Interrupt-On-Change positive edge");
    m_ioccn = new IOC(this, "ioccn", "Interrupt-On-Change negative edge");
    m_portc = new PicPortIOCRegister(this, "portc", "", intcon, m_ioccp, m_ioccn, m_ioccf, 8, 0xff);
    m_trisc = new PicTrisRegister(this, "trisc", "", m_portc, false, 0xff);
    m_latc  = new PicLatchRegister(this, "latc", "", m_portc, 0xff);
    m_wpuc = new WPU(this, "wpuc", "Weak Pull-up Register", m_portc, 0xff);
    m_iocaf = new IOCxF(this, "iocaf", "Interrupt-On-Change flag Register");
    m_iocap = new IOC(this, "iocap", "Interrupt-On-Change positive edge");
    m_iocan = new IOC(this, "iocan", "Interrupt-On-Change negative edge");
    m_porta = new PicPortIOCRegister(this, "porta", "", intcon, m_iocap, m_iocan, m_iocaf, 8, 0xff);
    m_trisa = new PicTrisRegister(this, "trisa", "", m_porta, false, 0xff);
    m_lata  = new PicLatchRegister(this, "lata", "", m_porta, 0xff);
    m_iocef = new IOCxF(this, "iocef", "Interrupt-On-Change flag Register", 0x08);
    m_iocep = new IOC(this, "iocep", "Interrupt-On-Change positive edge", 0x08);
    m_iocen = new IOC(this, "iocen", "Interrupt-On-Change negative edge", 0x08);
    m_porte = new PicPortIOCRegister(this, "porte", "", intcon, m_iocep, m_iocen, m_iocef, 8, 0x08);
    m_trise = new PicTrisRegister(this, "trise", "", m_porte, false, 0x00);
    m_daccon0 = new DACCON0(this, "dac1con0", "DAC1 8bit Voltage reference register 0", 0xbd, 256);
    m_daccon1 = new DACCON1(this, "dac1con1", "DAC1 8bit Voltage reference register 1", 0xff, m_daccon0);
    m_dac2con0 = new DACCON0(this, "dac2con0", "DAC2 5bit Voltage reference register 0", 0xb4, 32);
    m_dac2con1 = new DACCON1(this, "dac2con1", "DAC2 5bit Voltage reference register 1", 0x1f, m_dac2con0);
    m_dac3con0 = new DACCON0(this, "dac3con0", "DAC3 5bit Voltage reference register 0", 0xb4, 32);
    m_dac3con1 = new DACCON1(this, "dac3con1", "DAC3 5bit Voltage reference register 1", 0x1f, m_dac3con0);
    m_dac4con0 = new DACCON0(this, "dac4con0", "DAC4 5bit Voltage reference register 0", 0xb4, 32);
    m_dac4con1 = new DACCON1(this, "dac4con1", "DAC4 5bit Voltage reference register 1", 0x1f, m_dac4con0);
    m_cpu_temp = new CPU_Temp("cpu_temperature", 30., "CPU die temperature");
    tmr0.link_cpu(this, m_porta, 4, option_reg);
    tmr0.start(0);
    tmr0.set_t1gcon(&t1con_g.t1gcon);
    set_mclr_pin(1);
    ((INTCON_14_PIR *)intcon)->write_mask = 0xfe;
    m_wpua = new WPU(this, "wpua", "Weak Pull-up Register", m_porta, 0xff);
    m_wpue = new WPU(this, "wpue", "Weak Pull-up Register", m_porte, 0x08);
    pir1 = new PIR1v1822(this, "pir1", "Peripheral Interrupt Register", intcon, &pie1);
    pir2 = new PIR2v1822(this, "pir2", "Peripheral Interrupt Register", intcon, &pie2);
    pir3 = new PIR3v178x(this, "pir3", "Peripheral Interrupt Register", intcon, &pie3);
    pir4 = new PIR3v178x(this, "pir4", "Peripheral Interrupt Register", intcon, &pie3);
    pir2->valid_bits |= PIR2v1822::C2IF | PIR2v1822::CCP2IF | PIR2v1822::C3IF | PIR2v1822::C4IF;
    pir2->writable_bits |= PIR2v1822::C2IF | PIR2v1822::CCP2IF | PIR2v1822::C3IF | PIR2v1822::C4IF;
    pir4->valid_bits = pir4->writable_bits = 0xff;
}


P16F178x::~P16F178x()
{
    unassignMCLRPin();
    delete_file_registers(0x20, 0x7f);
    unsigned int ram = ram_size - 96; // first 96 bytes already added
    unsigned int add;

    for (add = 0x80; ram >= 80; add += 0x80)
    {
        ram -= 80;
        delete_file_registers(add + 0x20, add + 0x6f);
    }

    if (ram > 0)
    {
        delete_file_registers(add + 0x20, add + 0x20 + ram - 1);
    }

    adcon1.detach_fvr();
    adcon1.detach_DAC();
    comparator.detach_fvr();
    m_daccon0->detach_fvr();
    m_dac2con0->detach_fvr();
    m_dac3con0->detach_fvr();
    m_dac4con0->detach_fvr();
    delete_sfr_register(m_iocap);
    delete_sfr_register(m_iocan);
    delete_sfr_register(m_iocaf);
    delete_sfr_register(m_iocbp);
    delete_sfr_register(m_iocbn);
    delete_sfr_register(m_iocbf);
    delete_sfr_register(m_ioccp);
    delete_sfr_register(m_ioccn);
    delete_sfr_register(m_ioccf);
    delete_sfr_register(m_iocep);
    delete_sfr_register(m_iocen);
    delete_sfr_register(m_iocef);
    delete_sfr_register(m_daccon0);
    delete_sfr_register(m_daccon1);
    delete_sfr_register(m_dac2con0);
    delete_sfr_register(m_dac2con1);
    delete_sfr_register(m_dac3con0);
    delete_sfr_register(m_dac3con1);
    delete_sfr_register(m_dac4con0);
    delete_sfr_register(m_dac4con1);
    delete_sfr_register(m_trisa);
    delete_sfr_register(m_porta);
    delete_sfr_register(m_lata);
    delete_sfr_register(m_wpua);
    delete_sfr_register(m_portb);
    delete_sfr_register(m_trisb);
    delete_sfr_register(m_latb);
    delete_sfr_register(m_portc);
    delete_sfr_register(m_trisc);
    delete_sfr_register(m_latc);
    delete_sfr_register(m_wpub);
    delete_sfr_register(m_wpuc);
    delete_sfr_register(m_trise);
    delete_sfr_register(m_porte);
    delete_sfr_register(m_wpue);
    remove_sfr_register(&tmr0);
    remove_sfr_register(&tmr1l);
    remove_sfr_register(&tmr1h);
    remove_sfr_register(&t1con_g);
    remove_sfr_register(&t1con_g.t1gcon);
    remove_sfr_register(&tmr2);
    remove_sfr_register(&pr2);
    remove_sfr_register(&t2con);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&ccpr1l);
    remove_sfr_register(&ccpr1h);
    remove_sfr_register(&ccp1con);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&ccp1as);
    remove_sfr_register(&pstr1con);
    remove_sfr_register(&pie1);
    remove_sfr_register(&pie2);
    remove_sfr_register(&pie3);
    remove_sfr_register(&pie4);
    remove_sfr_register(&adresl);
    remove_sfr_register(&adresh);
    remove_sfr_register(&adcon0);
    remove_sfr_register(&adcon1);
    remove_sfr_register(&adcon2);
    remove_sfr_register(&borcon);
    remove_sfr_register(&fvrcon);
    remove_sfr_register(&apfcon1);
    remove_sfr_register(&apfcon2);
    remove_sfr_register(&ansela);
    remove_sfr_register(&anselb);
    remove_sfr_register(&anselc);
    remove_sfr_register(get_eeprom()->get_reg_eeadr());
    remove_sfr_register(get_eeprom()->get_reg_eeadrh());
    remove_sfr_register(get_eeprom()->get_reg_eedata());
    remove_sfr_register(get_eeprom()->get_reg_eedatah());
    remove_sfr_register(get_eeprom()->get_reg_eecon1());
    remove_sfr_register(get_eeprom()->get_reg_eecon2());
    remove_sfr_register(&usart.spbrg);
    remove_sfr_register(&usart.spbrgh);
    remove_sfr_register(&usart.rcsta);
    remove_sfr_register(&usart.txsta);
    remove_sfr_register(&usart.baudcon);
    remove_sfr_register(&ssp.sspbuf);
    remove_sfr_register(&ssp.sspadd);
    remove_sfr_register(ssp.sspmsk);
    remove_sfr_register(&ssp.sspstat);
    remove_sfr_register(&ssp.sspcon);
    remove_sfr_register(&ssp.sspcon2);
    remove_sfr_register(&ssp.ssp1con3);
    remove_sfr_register(&ccpr1l);
    remove_sfr_register(&ccpr1h);
    remove_sfr_register(&ccp1con);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&ccp1as);
    remove_sfr_register(&pstr1con);
    remove_sfr_register(&osctune);
    remove_sfr_register(option_reg);
    remove_sfr_register(osccon);
    remove_sfr_register(&oscstat);
    remove_sfr_register(&vregcon);
    remove_sfr_register(comparator.cmxcon0[0]);
    remove_sfr_register(comparator.cmxcon1[0]);
    remove_sfr_register(comparator.cmout);
    remove_sfr_register(comparator.cmxcon0[1]);
    remove_sfr_register(comparator.cmxcon1[1]);
    remove_sfr_register(comparator.cmxcon0[2]);
    remove_sfr_register(comparator.cmxcon1[2]);
    delete_sfr_register(usart.rcreg);
    delete_sfr_register(usart.txreg);
    delete_sfr_register(pir1);
    delete_sfr_register(pir2);
    delete_sfr_register(pir3);
    delete_sfr_register(pir4);
    delete e;
    delete m_cpu_temp;
}


void P16F178x::create_symbols()
{
    pic_processor::create_symbols();
    addSymbol(Wreg);
    addSymbol(m_cpu_temp);
}


void P16F178x::create_sfr_map()
{
    pir_set_2_def.set_pir1(pir1);
    pir_set_2_def.set_pir2(pir2);
    pir_set_2_def.set_pir3(pir3);
    pir_set_2_def.set_pir4(pir4);
    add_file_registers(0x20, 0x7f, 0x00);
    unsigned int ram = ram_size - 96; // first 96 bytes already added
    unsigned int add;

    for (add = 0x80; ram >= 80; add += 0x80)
    {
        ram -= 80;
        add_file_registers(add + 0x20, add + 0x6f, 0x00);
    }

    if (ram > 0)
    {
        add_file_registers(add + 0x20, add + 0x20 + ram - 1, 0x00);
    }

    comparator.cmxcon0[0] = new CMxCON0(this, "cm1con0", " Comparator C1 Control Register 0", 0, &comparator);
    comparator.cmxcon1[0] = new CMxCON1(this, "cm1con1", " Comparator C1 Control Register 1", 0, &comparator);
    comparator.cmout = new CMOUT(this, "cmout", "Comparator Output Register");
    comparator.cmxcon0[1] = new CMxCON0(this, "cm2con0", " Comparator C2 Control Register 0", 1, &comparator);
    comparator.cmxcon1[1] = new CMxCON1(this, "cm2con1", " Comparator C2 Control Register 1", 1, &comparator);
    comparator.cmxcon0[2] = new CMxCON0(this, "cm3con0", " Comparator C3 Control Register 0", 2, &comparator);
    comparator.cmxcon1[2] = new CMxCON1(this, "cm3con1", " Comparator C3 Control Register 1", 2, &comparator);
    add_sfr_register(m_porta, 0x0c);
    add_sfr_register(m_portb, 0x0d);
    add_sfr_register(m_portc, 0x0e);
    add_sfr_register(m_porte, 0x10);
    add_sfr_registerR(pir1,    0x11, RegisterValue(0, 0), "pir1");
    add_sfr_registerR(pir2,    0x12, RegisterValue(0, 0), "pir2");
    add_sfr_registerR(pir3,    0x13, RegisterValue(0, 0), "pir3");
    add_sfr_registerR(pir4,    0x14, RegisterValue(0, 0), "pir4");
    add_sfr_register(&tmr0,   0x15);
    add_sfr_register(&tmr1l,  0x16, RegisterValue(0, 0), "tmr1l");
    add_sfr_register(&tmr1h,  0x17, RegisterValue(0, 0), "tmr1h");
    add_sfr_register(&t1con_g,  0x18, RegisterValue(0, 0));
    add_sfr_register(&t1con_g.t1gcon, 0x19, RegisterValue(0, 0));
    add_sfr_register(&tmr2,   0x1a, RegisterValue(0, 0));
    add_sfr_register(&pr2,    0x1b, RegisterValue(0xff, 0));
    add_sfr_registerR(&t2con,  0x1c, RegisterValue(0, 0));
    add_sfr_register(m_trisa, 0x8c, RegisterValue(0xff, 0));
    add_sfr_register(m_trisb, 0x8d, RegisterValue(0xff, 0));
    add_sfr_register(m_trisc, 0x8e, RegisterValue(0xff, 0));
    add_sfr_register(m_trise, 0x90, RegisterValue(0x08, 0));
    pcon.valid_bits = 0xcf;
    add_sfr_register(option_reg, 0x95, RegisterValue(0xff, 0));
    add_sfr_register(&osctune,    0x98, RegisterValue(0, 0));
    add_sfr_register(osccon,     0x99, RegisterValue(0x38, 0));
    add_sfr_register(&oscstat,    0x9a, RegisterValue(0, 0));
    intcon_reg.set_pir_set(get_pir_set());
    tmr1l.tmrh = &tmr1h;
    tmr1l.t1con = &t1con_g;
    tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v1::TMR1IF));
    tmr1h.tmrl  = &tmr1l;
    t1con_g.tmrl  = &tmr1l;
    t1con_g.t1gcon.set_tmrl(&tmr1l);
    t1con_g.t1gcon.setInterruptSource(new InterruptSource(pir1, PIR1v1822::TMR1GIF));
    tmr1l.setIOpin(&(*m_porta)[5]);
    t1con_g.t1gcon.setGatepin(&(*m_porta)[3]);
    add_sfr_registerR(&pie1,   0x91, RegisterValue(0, 0));
    add_sfr_registerR(&pie2,   0x92, RegisterValue(0, 0));
    add_sfr_registerR(&pie3,   0x93, RegisterValue(0, 0));
    add_sfr_registerR(&pie4,   0x94, RegisterValue(0, 0));
    add_sfr_register(&adresl, 0x9b);
    add_sfr_register(&adresh, 0x9c);
    add_sfr_registerR(&adcon0, 0x9d, RegisterValue(0x00, 0));
    add_sfr_registerR(&adcon1, 0x9e, RegisterValue(0x00, 0));
    add_sfr_registerR(&adcon2, 0x9f, RegisterValue(0x00, 0));
    usart.initialize(pir1,
                     &(*m_porta)[0], // TX pin
                     & (*m_porta)[1], // RX pin
                     new _TXREG(this, "txreg", "USART Transmit Register", &usart),
                     new _RCREG(this, "rcreg", "USART Receiver Register", &usart));
    usart.set_eusart(true);
    add_sfr_register(m_lata,    0x10c);
    add_sfr_register(m_latb, 0x10d);
    add_sfr_register(m_latc, 0x10e);
    add_sfr_registerR(comparator.cmxcon0[0], 0x111, RegisterValue(0x04, 0));
    add_sfr_registerR(comparator.cmxcon1[0], 0x112, RegisterValue(0x00, 0));
    add_sfr_registerR(comparator.cmxcon0[1], 0x113, RegisterValue(0x04, 0));
    add_sfr_registerR(comparator.cmxcon1[1], 0x114, RegisterValue(0x00, 0));
    add_sfr_registerR(comparator.cmout,      0x115, RegisterValue(0x00, 0));
    add_sfr_registerR(&borcon,   0x116, RegisterValue(0x80, 0));
    add_sfr_registerR(&fvrcon,   0x117, RegisterValue(0x00, 0));
    add_sfr_registerR(m_daccon0, 0x118, RegisterValue(0x00, 0));
    add_sfr_registerR(m_daccon1, 0x119, RegisterValue(0x00, 0));
    add_sfr_registerR(&apfcon2,  0x11c, RegisterValue(0x00, 0));
    add_sfr_registerR(&apfcon1,  0x11d, RegisterValue(0x00, 0));
    add_sfr_registerR(comparator.cmxcon0[2], 0x11e, RegisterValue(0x04, 0));
    add_sfr_registerR(comparator.cmxcon1[2], 0x11f, RegisterValue(0x00, 0));
    add_sfr_registerR(&ansela,   0x18c, RegisterValue(0x17, 0));
    add_sfr_registerR(&anselb,   0x18d, RegisterValue(0x7f, 0));
    add_sfr_registerR(&anselc,   0x18e, RegisterValue(0xff, 0));
    get_eeprom()->get_reg_eedata()->new_name("eedatl");
    get_eeprom()->get_reg_eedatah()->new_name("eedath");
    add_sfr_registerR(get_eeprom()->get_reg_eeadr(),   0x191);
    add_sfr_registerR(get_eeprom()->get_reg_eeadrh(),   0x192);
    add_sfr_register(get_eeprom()->get_reg_eedata(),  0x193);
    add_sfr_register(get_eeprom()->get_reg_eedatah(),  0x194);
    add_sfr_registerR(get_eeprom()->get_reg_eecon1(),  0x195, RegisterValue(0, 0));
    add_sfr_registerR(get_eeprom()->get_reg_eecon2(),  0x196);
    add_sfr_registerR(&vregcon, 0x197, RegisterValue(1, 0));
    add_sfr_registerR(usart.rcreg,    0x199, RegisterValue(0, 0), "rcreg");
    add_sfr_registerR(usart.txreg,    0x19a, RegisterValue(0, 0), "txreg");
    add_sfr_registerR(&usart.spbrg,   0x19b, RegisterValue(0, 0), "spbrgl");
    add_sfr_registerR(&usart.spbrgh,  0x19c, RegisterValue(0, 0), "spbrgh");
    add_sfr_registerR(&usart.rcsta,   0x19d, RegisterValue(0, 0), "rcsta");
    add_sfr_registerR(&usart.txsta,   0x19e, RegisterValue(2, 0), "txsta");
    add_sfr_registerR(&usart.baudcon, 0x19f, RegisterValue(0x40, 0), "baudcon");
    add_sfr_registerR(m_wpua,     0x20c, RegisterValue(0xff, 0), "wpua");
    add_sfr_registerR(m_wpub,     0x20d, RegisterValue(0xff, 0), "wpub");
    add_sfr_registerR(m_wpuc,     0x20e, RegisterValue(0xff, 0), "wpuc");
    add_sfr_registerR(m_wpue,     0x210, RegisterValue(0x04, 0), "wpue");
    add_sfr_register(&ssp.sspbuf,  0x211, RegisterValue(0, 0), "ssp1buf");
    add_sfr_registerR(&ssp.sspadd,  0x212, RegisterValue(0, 0), "ssp1add");
    add_sfr_registerR(ssp.sspmsk, 0x213, RegisterValue(0xff, 0), "ssp1msk");
    add_sfr_registerR(&ssp.sspstat, 0x214, RegisterValue(0, 0), "ssp1stat");
    add_sfr_registerR(&ssp.sspcon,  0x215, RegisterValue(0, 0), "ssp1con");
    add_sfr_registerR(&ssp.sspcon2, 0x216, RegisterValue(0, 0), "ssp1con2");
    add_sfr_registerR(&ssp.ssp1con3, 0x217, RegisterValue(0, 0), "ssp1con3");
    add_sfr_register(&ccpr1l,      0x291, RegisterValue(0, 0));
    add_sfr_register(&ccpr1h,      0x292, RegisterValue(0, 0));
    add_sfr_registerR(&ccp1con,     0x293, RegisterValue(0, 0));
    add_sfr_register(&pwm1con,     0x294, RegisterValue(0, 0));
    add_sfr_register(&ccp1as,      0x295, RegisterValue(0, 0));
    add_sfr_register(&pstr1con,    0x296, RegisterValue(1, 0));
    add_sfr_registerR(m_iocap, 0x391, RegisterValue(0, 0), "iocap");
    add_sfr_registerR(m_iocan, 0x392, RegisterValue(0, 0), "iocan");
    add_sfr_registerR(m_iocaf, 0x393, RegisterValue(0, 0), "iocaf");
    m_iocaf->set_intcon(intcon);
    add_sfr_registerR(m_iocbp, 0x394, RegisterValue(0, 0), "iocbp");
    add_sfr_registerR(m_iocbn, 0x395, RegisterValue(0, 0), "iocbn");
    add_sfr_registerR(m_iocbf, 0x396, RegisterValue(0, 0), "iocbf");
    m_iocbf->set_intcon(intcon);
    add_sfr_registerR(m_ioccp, 0x397, RegisterValue(0, 0), "ioccp");
    add_sfr_registerR(m_ioccn, 0x398, RegisterValue(0, 0), "ioccn");
    add_sfr_registerR(m_ioccf, 0x399, RegisterValue(0, 0), "ioccf");
    m_ioccf->set_intcon(intcon);
    add_sfr_registerR(m_iocep, 0x39d, RegisterValue(0, 0), "iocep");
    add_sfr_registerR(m_iocen, 0x39e, RegisterValue(0, 0), "iocen");
    add_sfr_registerR(m_iocef, 0x39f, RegisterValue(0, 0), "iocef");
    m_iocef->set_intcon(intcon);
    add_sfr_registerR(m_dac2con0, 0x591, RegisterValue(0x00, 0));
    add_sfr_registerR(m_dac2con1, 0x592, RegisterValue(0x00, 0));
    add_sfr_registerR(m_dac3con0, 0x593, RegisterValue(0x00, 0));
    add_sfr_registerR(m_dac3con1, 0x594, RegisterValue(0x00, 0));
    add_sfr_registerR(m_dac4con0, 0x595, RegisterValue(0x00, 0));
    add_sfr_registerR(m_dac4con1, 0x596, RegisterValue(0x00, 0));
    tmr2.ssp_module[0] = &ssp;
    ssp.initialize(
        get_pir_set(),    // PIR
        & (*m_porta)[1],  // SCK
        & (*m_porta)[3],  // SS
        & (*m_porta)[0],  // SDO
        & (*m_porta)[2],   // SDI
        m_trisa,          // i2c tris port
        SSP_TYPE_MSSP1
    );
    apfcon1.set_pins(0, &ccp1con, CCPCON::CCP_PIN, &(*m_porta)[2], &(*m_porta)[5]); //CCP1/P1A
    apfcon1.set_pins(1, &ccp1con, CCPCON::PxB_PIN, &(*m_porta)[0], &(*m_porta)[4]); //P1B
    apfcon1.set_pins(2, &usart, USART_MODULE::TX_PIN, &(*m_porta)[0], &(*m_porta)[4]); //USART TX Pin
    apfcon1.set_pins(3, &t1con_g.t1gcon, 0, &(*m_porta)[4], &(*m_porta)[3]); //tmr1 gate
    apfcon1.set_pins(5, &ssp, SSP1_MODULE::SS_PIN, &(*m_porta)[3], &(*m_porta)[0]); //SSP SS
    apfcon1.set_pins(6, &ssp, SSP1_MODULE::SDO_PIN, &(*m_porta)[0], &(*m_porta)[4]); //SSP SDO
    apfcon1.set_pins(7, &usart, USART_MODULE::RX_PIN, &(*m_porta)[1], &(*m_porta)[5]); //USART RX Pin

    if (pir1)
    {
        pir1->set_intcon(intcon);
        pir1->set_pie(&pie1);
    }

    pie1.setPir(pir1);
    pie2.setPir(pir2);
    pie3.setPir(pir3);
    pie4.setPir(pir4);
    t2con.tmr2 = &tmr2;
    tmr2.pir_set   = get_pir_set();
    tmr2.pr2    = &pr2;
    tmr2.t2con  = &t2con;
    tmr2.add_ccp(&ccp1con);
    //  tmr2.add_ccp ( &ccp2con );
    pr2.tmr2    = &tmr2;
    ccp1as.setIOpin(0, 0, &(*m_porta)[2]);
    ccp1as.link_registers(&pwm1con, &ccp1con);
    ccp1con.setIOpin(&(*m_porta)[2], &(*m_porta)[0]);
    ccp1con.pstrcon = &pstr1con;
    ccp1con.pwm1con = &pwm1con;
    ccp1con.setCrosslinks(&ccpr1l, pir1, PIR1v1822::CCP1IF, &tmr2, &ccp1as);
    ccpr1l.ccprh  = &ccpr1h;
    ccpr1l.tmrl   = &tmr1l;
    ccpr1h.ccprl  = &ccpr1l;
    ansela.config(0x17, 0);
    ansela.setValidBits(0x17);
    ansela.setAdcon1(&adcon1);
    anselb.config(0x3f, 8);
    anselb.setValidBits(0x7f);
    anselb.setAdcon1(&adcon1);
    anselb.setAnsel(&ansela);
    ansela.setAnsel(&anselb);
    anselc.setValidBits(0xff);
    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    adcon0.setAdcon2(&adcon2);
    adcon0.setIntcon(intcon);
    adcon0.setA2DBits(12);
    adcon0.setPir(pir1);
    adcon0.setChannel_Mask(0x1f);
    adcon0.setChannel_shift(2);
    adcon0.setGo(1);
    adcon1.set_FVR_chan(0x1f);
    adcon1.attach_ad_fvr(fvrcon.get_node_adcvref(), 0x1f);
    adcon1.attach_DAC(m_daccon0->get_node_dacout(), 0x1e, 1);
    adcon1.attach_Vt_fvr(fvrcon.get_node_Vtref(), 0x1d);
    adcon1.attach_DAC(m_dac2con0->get_node_dacout(), 0x1c, 2);
    adcon1.attach_DAC(m_dac3con0->get_node_dacout(), 0x19, 3);
    adcon1.attach_DAC(m_dac4con0->get_node_dacout(), 0x18, 4);
    adcon1.setAdcon0(&adcon0);
    adcon1.setNumberOfChannels(32); // not all channels are used
    adcon1.setIOPin(0, &(*m_porta)[0]);
    adcon1.setIOPin(1, &(*m_porta)[1]);
    adcon1.setIOPin(2, &(*m_porta)[2]);
    adcon1.setIOPin(3, &(*m_porta)[4]);
    adcon1.setValidBits(0xf7);
    adcon1.setVrefHiConfiguration(0, 3);
    adcon1.setVrefLoConfiguration(0, 2);
    comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[0], &(*m_porta)[1],  &(*m_portb)[3],  &(*m_portb)[1]);
    comparator.cmxcon1[1]->set_INpinNeg(&(*m_porta)[0], &(*m_porta)[1],  &(*m_portb)[3],  &(*m_portb)[1]);
    comparator.cmxcon1[2]->set_INpinNeg(&(*m_porta)[0], &(*m_porta)[1],  &(*m_portb)[3],  &(*m_portb)[1]);
    comparator.cmxcon1[0]->set_INpinPos(&(*m_porta)[2], &(*m_porta)[3]);
    comparator.cmxcon1[1]->set_INpinPos(&(*m_porta)[2], &(*m_portb)[0]);
    comparator.cmxcon1[2]->set_INpinPos(&(*m_porta)[2], &(*m_portb)[4]);
    comparator.cmxcon1[0]->set_OUTpin(&(*m_porta)[4]);
    comparator.cmxcon1[1]->set_OUTpin(&(*m_porta)[5]);
    comparator.cmxcon1[2]->set_OUTpin(&(*m_portb)[5]);
    comparator.cmxcon0[0]->setBitMask(0xbf);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, (1 << 5)));
    comparator.cmxcon0[1]->setBitMask(0xbf);
    comparator.cmxcon0[1]->setIntSrc(new InterruptSource(pir2, (1 << 6)));
    comparator.cmxcon0[2]->setBitMask(0xbf);
    comparator.cmxcon0[2]->setIntSrc(new InterruptSource(pir2, (1 << 1)));
    comparator.cmxcon1[0]->setBitMask(0xff);
    comparator.cmxcon1[1]->setBitMask(0xff);
    comparator.cmxcon1[2]->setBitMask(0xff);
    comparator.assign_pir_set(get_pir_set());
    comparator.assign_t1gcon(&t1con_g.t1gcon);
    comparator.Pmask[0] = CMxCON1::CM_PIN;
    comparator.Pmask[1] = CMxCON1::CM_PIN;
    comparator.Pmask[2] = CMxCON1::CM_DAC4;
    comparator.Pmask[3] = CMxCON1::CM_DAC3;
    comparator.Pmask[4] = CMxCON1::CM_DAC2;
    comparator.Pmask[5] = CMxCON1::CM_DAC1;
    comparator.Pmask[6] = CMxCON1::CM_FVR;
    comparator.Pmask[7] = CMxCON1::CM_AGND;
    comparator.Nmask[0] = CMxCON1::CM_PIN;
    comparator.Nmask[1] = CMxCON1::CM_PIN;
    comparator.Nmask[2] = CMxCON1::CM_PIN;
    comparator.Nmask[3] = CMxCON1::CM_PIN;
    comparator.Nmask[4] = CMxCON1::CM_PIN;
    comparator.Nmask[7] = CMxCON1::CM_AGND;
#ifdef DEBUG
    for(int i=0; i<8; i++)
	printf("\ti=%d Pmask=%d Nmask=%d\n", i, comparator.Pmask[i], comparator.Nmask[i]);
#endif
    comparator.attach_cda_fvr(fvrcon.get_node_cvref());
    m_daccon0->set_adcon1(&adcon1);
    m_daccon0->attach_cda_fvr(fvrcon.get_node_cvref(), 0x1e);
    m_daccon0->setDACOUT(&(*m_porta)[2], &(*m_portb)[7]);
    m_dac2con0->set_adcon1(&adcon1);
    m_dac2con0->attach_cda_fvr(fvrcon.get_node_cvref(), 0x1c);
    m_dac2con0->setDACOUT(&(*m_porta)[5], &(*m_portb)[7]);
    m_dac3con0->set_adcon1(&adcon1);
    m_dac3con0->attach_cda_fvr(fvrcon.get_node_cvref(), 0x19);
    m_dac3con0->setDACOUT(&(*m_portb)[2], &(*m_portb)[7]);
    m_dac4con0->set_adcon1(&adcon1);
    m_dac4con0->attach_cda_fvr(fvrcon.get_node_cvref(), 0x18);
    m_dac4con0->setDACOUT(&(*m_porta)[4], &(*m_portb)[7]);
    osccon->set_osctune(&osctune);
    osccon->set_oscstat(&oscstat);
    osctune.set_osccon((OSCCON *)osccon);
    osccon->write_mask = 0xfb;
    int_pin.setIOpin(&(*m_portb)[0]);
}


//-------------------------------------------------------------------
void P16F178x::set_out_of_range_pm(unsigned int address, unsigned int value)
{
    if ((address >= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
        get_eeprom()->change_rom(address - 0x2100, value);
    }
}


void  P16F178x::create(int /* ram_top */, int eeprom_size)
{
    e = new EEPROM_EXTND(this, pir2);
    set_eeprom(e);
    osccon = new OSCCON_2(this, "osccon", "Oscillator Control Register");
    pic_processor::create();
    e->initialize(eeprom_size, 16, 16, 0x8000);
    e->set_intcon(intcon);
    e->get_reg_eecon1()->set_valid_bits(0xff);
    P16F178x::create_sfr_map();
    _14bit_e_processor::create_sfr_map();
}


//-------------------------------------------------------------------
void P16F178x::enter_sleep()
{
    tmr1l.sleep();
    osccon->sleep();
    _14bit_e_processor::enter_sleep();
}


//-------------------------------------------------------------------
void P16F178x::exit_sleep()
{
    if (m_ActivityState == ePASleeping)
    {
        tmr1l.wake();
        osccon->wake();
        _14bit_e_processor::exit_sleep();
    }
}


//-------------------------------------------------------------------
void P16F178x::option_new_bits_6_7(unsigned int bits)
{
    Dprintf(("P16F178x::option_new_bits_6_7 bits=%x\n", bits));
    m_porta->setIntEdge((bits & OPTION_REG::BIT6) == OPTION_REG::BIT6);
    m_wpua->set_wpu_pu((bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
}


void P16F178x::oscillator_select(unsigned int cfg_word1, bool clkout)
{
    unsigned int mask = m_porta->getEnableMask();
    unsigned int fosc = cfg_word1 & (FOSC0 | FOSC1 | FOSC2);
    osccon->set_config_irc(fosc == 4);
    osccon->set_config_xosc(fosc < 3);
    osccon->set_config_ieso(cfg_word1 & IESO);
    set_int_osc(false);

    switch (fosc)
    {
    case 0:	//LP oscillator: low power crystal
    case 1:	//XT oscillator: Crystal/resonator
    case 2:	//HS oscillator: High-speed crystal/resonator
        (m_porta->getPin(6))->newGUIname("OSC2");
        (m_porta->getPin(7))->newGUIname("OSC1");
        mask &= 0x3f;
        break;

    case 3:	//EXTRC oscillator External RC circuit connected to CLKIN pin
        (m_porta->getPin(7))->newGUIname("CLKIN");
        mask &= 0x7f;

        if (clkout)
        {
            (m_porta->getPin(6))->newGUIname("CLKOUT");
            mask &= 0xbf;

        }
        else
        {
            (m_porta->getPin(6))->newGUIname((m_porta->getPin(6))->name().c_str());
            mask |= 0x40;
        }

        break;

    case 4:	//INTOSC oscillator: I/O function on CLKIN pin
        set_int_osc(true);

        if (clkout)
        {
            (m_porta->getPin(6))->newGUIname("CLKOUT");
            mask &= 0xbf;

        }
        else
        {
            (m_porta->getPin(6))->newGUIname((m_porta->getPin(6))->name().c_str());
            mask |= 0x40;
        }

        mask |= 0x80;
        (m_porta->getPin(7))->newGUIname((m_porta->getPin(7))->name().c_str());
        break;

    case 5:	//ECL: External Clock, Low-Power mode (0-0.5 MHz): on CLKIN pin
        if (clkout)
        {
            (m_porta->getPin(6))->newGUIname("CLKOUT");
            mask &= 0xbf;

        }
        else
        {
            (m_porta->getPin(6))->newGUIname((m_porta->getPin(6))->name().c_str());
            mask |= 0x40;
        }

        mask &= 0x7f;
        (m_porta->getPin(7))->newGUIname("CLKIN");
        break;

    case 6:	//ECM: External Clock, Medium-Power mode (0.5-4 MHz): on CLKIN pin
        if (clkout)
        {
            (m_porta->getPin(6))->newGUIname("CLKOUT");
            mask &= 0xbf;

        }
        else
        {
            mask |= 0x40;
            (m_porta->getPin(6))->newGUIname((m_porta->getPin(6))->name().c_str());
        }

        (m_porta->getPin(7))->newGUIname("CLKIN");
        mask &= 0x7f;
        break;

    case 7:	//ECH: External Clock, High-Power mode (4-32 MHz): on CLKIN pin
        if (clkout)
        {
            (m_porta->getPin(6))->newGUIname("CLKOUT");
            mask &= 0xbf;

        }
        else
        {
            mask |= 0x40;
            (m_porta->getPin(6))->newGUIname((m_porta->getPin(6))->name().c_str());
        }

        (m_porta->getPin(7))->newGUIname("CLKIN");
        mask &= 0x7f;
        break;
    };

    ansela.setValidBits(0x17 & mask);

    m_porta->setEnableMask(mask);
}


void P16F178x::program_memory_wp(unsigned int mode)
{
    switch (mode)
    {
    case 3:	// no write protect
        get_eeprom()->set_prog_wp(0x0);
        break;

    case 2: // write protect 0000-01ff
        get_eeprom()->set_prog_wp(0x0200);
        break;

    case 1: // write protect 0000-03ff
        get_eeprom()->set_prog_wp(0x0400);
        break;

    case 0: // write protect 0000-07ff
        get_eeprom()->set_prog_wp(0x0800);
        break;

    default:
        printf("%s unexpected mode %u\n", __FUNCTION__, mode);
        break;
    }
}
//========================================================================


P16F1788::P16F1788(const char *_name, const char *desc)
    : P16F178x(_name, desc)
{
    comparator.cmxcon0[3] = new CMxCON0(this, "cm4con0", " Comparator C4 Control Register 0", 3, &comparator);
    comparator.cmxcon1[3] = new CMxCON1(this, "cm4con1", " Comparator C4 Control Register 1", 3, &comparator);
}


P16F1788::~P16F1788()
{
    remove_sfr_register(comparator.cmxcon0[3]);
    remove_sfr_register(comparator.cmxcon1[3]);
}


void P16F1788::create_iopin_map()
{
    package = new Package(28);

    //createMCLRPin(1);
    // Now Create the package and place the I/O pins
    package->assign_pin(1, m_porte->addPin(new IO_bi_directional_pu("porte3"), 3));
    package->assign_pin(2, m_porta->addPin(new IO_bi_directional_pu("porta0"), 0));
    package->assign_pin(3, m_porta->addPin(new IO_bi_directional_pu("porta1"), 1));
    package->assign_pin(4, m_porta->addPin(new IO_bi_directional_pu("porta2"), 2));
    package->assign_pin(5, m_porta->addPin(new IO_bi_directional_pu("porta3"), 3));
    package->assign_pin(6, m_porta->addPin(new IO_bi_directional_pu("porta4"), 4));
    package->assign_pin(7, m_porta->addPin(new IO_bi_directional_pu("porta5"), 5));
    package->assign_pin(10, m_porta->addPin(new IO_bi_directional_pu("porta6"), 6));
    package->assign_pin(9, m_porta->addPin(new IO_bi_directional_pu("porta7"), 7));
    package->assign_pin(11, m_portc->addPin(new IO_bi_directional_pu("portc0"), 0));
    package->assign_pin(12, m_portc->addPin(new IO_bi_directional_pu("portc1"), 1));
    package->assign_pin(13, m_portc->addPin(new IO_bi_directional_pu("portc2"), 2));
    package->assign_pin(14, m_portc->addPin(new IO_bi_directional_pu("portc3"), 3));
    package->assign_pin(15, m_portc->addPin(new IO_bi_directional_pu("portc4"), 4));
    package->assign_pin(16, m_portc->addPin(new IO_bi_directional_pu("portc5"), 5));
    package->assign_pin(17, m_portc->addPin(new IO_bi_directional_pu("portc6"), 6));
    package->assign_pin(18, m_portc->addPin(new IO_bi_directional_pu("portc7"), 7));
    package->assign_pin(21, m_portb->addPin(new IO_bi_directional_pu("portb0"), 0));
    package->assign_pin(22, m_portb->addPin(new IO_bi_directional_pu("portb1"), 1));
    package->assign_pin(23, m_portb->addPin(new IO_bi_directional_pu("portb2"), 2));
    package->assign_pin(24, m_portb->addPin(new IO_bi_directional_pu("portb3"), 3));
    package->assign_pin(25, m_portb->addPin(new IO_bi_directional_pu("portb4"), 4));
    package->assign_pin(26, m_portb->addPin(new IO_bi_directional_pu("portb5"), 5));
    package->assign_pin(27, m_portb->addPin(new IO_bi_directional_pu("portb6"), 6));
    package->assign_pin(28, m_portb->addPin(new IO_bi_directional_pu("portb7"), 7));
    package->assign_pin(20, 0);	// Vdd
    package->assign_pin(19, 0);	// Vss
    package->assign_pin(8, 0);	// Vss
}


Processor * P16F1788::construct(const char *name)
{
    P16F1788 *p = new P16F1788(name);
    p->create(2048, 256, 0x302b);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


void  P16F1788::create(int ram_top, int eeprom_size, int dev_id)
{
    ram_size = ram_top;
    create_iopin_map();
    P16F178x::create(ram_top, eeprom_size);
    create_sfr_map();

    // Set DeviceID
    if (m_configMemory && m_configMemory->getConfigWord(6))
    {
        m_configMemory->getConfigWord(6)->set(dev_id);
    }
}


void P16F1788::create_sfr_map()
{
    add_sfr_register(comparator.cmxcon0[3], 0x11a, RegisterValue(0x04, 0));
    add_sfr_register(comparator.cmxcon1[3], 0x11b, RegisterValue(0x00, 0));
    adcon1.setIOPin(12, &(*m_portb)[0]);
    adcon1.setIOPin(10, &(*m_portb)[1]);
    adcon1.setIOPin(8, &(*m_portb)[2]);
    adcon1.setIOPin(9, &(*m_portb)[3]);
    adcon1.setIOPin(11, &(*m_portb)[4]);
    adcon1.setIOPin(13, &(*m_portb)[5]);
    ssp.set_sckPin(&(*m_portc)[0]);
    ssp.set_sdiPin(&(*m_portc)[1]);
    ssp.set_sdoPin(&(*m_portc)[2]);
    ssp.set_ssPin(&(*m_portc)[3]);
    ssp.set_tris(m_trisc);
    // Pin values for default APFCON
    usart.setIOpin(&(*m_portc)[4], USART_MODULE::TX_PIN);
    usart.setIOpin(&(*m_portc)[5], USART_MODULE::RX_PIN);
    ccp1con.setIOpin(&(*m_portc)[5], &(*m_portc)[4], &(*m_portc)[3], &(*m_portc)[2]);
    apfcon1.set_ValidBits(0xff);
    apfcon2.set_ValidBits(0x07);
    // pins 0,1 not used for p16f1788
    apfcon1.set_pins(2, &usart, USART_MODULE::TX_PIN, &(*m_portc)[4], &(*m_porta)[0]); //USART TX Pin
    // pin 3 defined in p12f1822
    apfcon1.set_pins(5, &ssp, SSP1_MODULE::SS_PIN, &(*m_portc)[3], &(*m_porta)[3]); //SSP SS
    apfcon1.set_pins(6, &ssp, SSP1_MODULE::SDO_PIN, &(*m_portc)[2], &(*m_porta)[4]); //SSP SDO
    apfcon1.set_pins(7, &usart, USART_MODULE::RX_PIN, &(*m_portc)[5], &(*m_porta)[1]); //USART RX Pin
    comparator.cmxcon1[3]->set_INpinNeg(&(*m_porta)[0], &(*m_porta)[1],  &(*m_portb)[5],  &(*m_portb)[1]);
    comparator.cmxcon1[3]->set_INpinPos(&(*m_porta)[2], &(*m_portb)[6]);
    comparator.cmxcon1[3]->set_OUTpin(&(*m_portc)[7]);
    comparator.cmxcon0[3]->setBitMask(0xbf);
    comparator.cmxcon0[3]->setIntSrc(new InterruptSource(pir2, (1 << 2)));
    comparator.cmxcon1[3]->setBitMask(0xff);
}


P16LF1788::P16LF1788(const char *_name, const char *desc)
    : P16F1788(_name, desc)
{
}


P16LF1788::~P16LF1788()
{
}


Processor * P16LF1788::construct(const char *name)
{
    P16LF1788 *p = new P16LF1788(name);
    p->create(2048, 256, 0x302d);
    p->create_invalid_registers();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}


void  P16LF1788::create(int ram_top, int eeprom_size, int dev_id)
{
    P16F1788::create(ram_top, eeprom_size, dev_id);
}


//========================================================================


P16F1823::P16F1823(const char *_name, const char *desc)
    : P12F1822(_name, desc),
      anselc(this, "anselc", "Analog Select port c")
{
    m_portc = new PicPortBRegister(this, "portc", "", intcon, 8, 0x3f);
    m_trisc = new PicTrisRegister(this, "trisc", "", m_portc, false, 0x3f);
    m_latc  = new PicLatchRegister(this, "latc", "", m_portc, 0x3f);
    m_wpuc = new WPU(this, "wpuc", "Weak Pull-up Register", m_portc, 0x3f);
    comparator.cmxcon0[1] = new CMxCON0(this, "cm2con0", " Comparator C2 Control Register 0", 1, &comparator);
    comparator.cmxcon1[1] = new CMxCON1(this, "cm2con1", " Comparator C2 Control Register 1", 1, &comparator);
    cpscon1.mValidBits = 0x0f;
    pir2->valid_bits |= PIR2v1822::C2IF;
    pir2->writable_bits |= PIR2v1822::C2IF;
}


P16F1823::~P16F1823()
{
    delete_sfr_register(m_portc);
    delete_sfr_register(m_trisc);
    delete_sfr_register(m_latc);
    remove_sfr_register(comparator.cmxcon0[1]);
    remove_sfr_register(comparator.cmxcon1[1]);
    delete_sfr_register(m_wpuc);
    remove_sfr_register(&anselc);
}


void P16F1823::create_iopin_map()
{
    package = new Package(14);

    // Now Create the package and place the I/O pins
    package->assign_pin(13, m_porta->addPin(new IO_bi_directional_pu("porta0"), 0));
    package->assign_pin(12, m_porta->addPin(new IO_bi_directional_pu("porta1"), 1));
    package->assign_pin(11, m_porta->addPin(new IO_bi_directional_pu("porta2"), 2));
    package->assign_pin(4, m_porta->addPin(new IO_bi_directional_pu("porta3"), 3));
    package->assign_pin(3, m_porta->addPin(new IO_bi_directional_pu("porta4"), 4));
    package->assign_pin(2, m_porta->addPin(new IO_bi_directional_pu("porta5"), 5));
    package->assign_pin(10, m_portc->addPin(new IO_bi_directional_pu("portc0"), 0));
    package->assign_pin(9, m_portc->addPin(new IO_bi_directional_pu("portc1"), 1));
    package->assign_pin(8, m_portc->addPin(new IO_bi_directional_pu("portc2"), 2));
    package->assign_pin(7, m_portc->addPin(new IO_bi_directional_pu("portc3"), 3));
    package->assign_pin(6, m_portc->addPin(new IO_bi_directional_pu("portc4"), 4));
    package->assign_pin(5, m_portc->addPin(new IO_bi_directional_pu("portc5"), 5));
    package->assign_pin(1, 0);	// Vdd
    package->assign_pin(14, 0);	// Vss
}


Processor * P16F1823::construct(const char *name)
{
    P16F1823 *p = new P16F1823(name);
    p->create(0x7f, 256, 0x2720);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


void  P16F1823::create(int ram_top, int eeprom_size, int dev_id)
{
    create_iopin_map();
    e = new EEPROM_EXTND(this, pir2);
    set_eeprom(e);
    osccon = new OSCCON_2(this, "osccon", "Oscillator Control Register");
    pic_processor::create();
    e->initialize(eeprom_size, 16, 16, 0x8000);
    e->set_intcon(intcon);
    e->get_reg_eecon1()->set_valid_bits(0xff);
    add_file_registers(0x20, ram_top, 0x00);
    _14bit_e_processor::create_sfr_map();
    P12F1822::create_sfr_map();
    create_sfr_map();
    dsm_module.setOUTpin(&(*m_portc)[4]);
    dsm_module.setMINpin(&(*m_portc)[3]);
    dsm_module.setCIN1pin(&(*m_portc)[2]);
    dsm_module.setCIN2pin(&(*m_portc)[5]);

    // Set DeviceID
    if (m_configMemory && m_configMemory->getConfigWord(6))
    {
        m_configMemory->getConfigWord(6)->set(dev_id);
    }
}


void P16F1823::create_sfr_map()
{
    add_sfr_register(m_portc, 0x0e);
    add_sfr_register(m_trisc, 0x8e, RegisterValue(0x3f, 0));
    add_sfr_register(m_latc, 0x10e);
    add_sfr_register(comparator.cmxcon0[1], 0x113, RegisterValue(0x04, 0));
    add_sfr_register(comparator.cmxcon1[1], 0x114, RegisterValue(0x00, 0));
    add_sfr_register(&anselc, 0x18e, RegisterValue(0x0f, 0));
    add_sfr_register(m_wpuc, 0x20e, RegisterValue(0x3f, 0), "wpuc");
    anselc.config(0x0f, 4);
    anselc.setValidBits(0x0f);
    anselc.setAdcon1(&adcon1);
    ansela.setAnsel(&anselc);
    anselc.setAnsel(&ansela);
    adcon1.setIOPin(4, &(*m_portc)[0]);
    adcon1.setIOPin(5, &(*m_portc)[1]);
    adcon1.setIOPin(6, &(*m_portc)[2]);
    adcon1.setIOPin(7, &(*m_portc)[3]);
    ssp.set_sckPin(&(*m_portc)[0]);
    ssp.set_sdiPin(&(*m_portc)[1]);
    ssp.set_sdoPin(&(*m_portc)[2]);
    ssp.set_ssPin(&(*m_portc)[3]);
    ssp.set_tris(m_trisc);
    // Pin values for default APFCON
    usart.setIOpin(&(*m_portc)[4], USART_MODULE::TX_PIN);
    usart.setIOpin(&(*m_portc)[5], USART_MODULE::RX_PIN);
    ccp1con.setIOpin(&(*m_portc)[5], &(*m_portc)[4], &(*m_portc)[3], &(*m_portc)[2]);
    apfcon.set_ValidBits(0xec);
    // pins 0,1 not used for p16f1823
    apfcon.set_pins(2, &usart, USART_MODULE::TX_PIN, &(*m_portc)[4], &(*m_porta)[0]); //USART TX Pin
    // pin 3 defined in p12f1822
    apfcon.set_pins(5, &ssp, SSP1_MODULE::SS_PIN, &(*m_portc)[3], &(*m_porta)[3]); //SSP SS
    apfcon.set_pins(6, &ssp, SSP1_MODULE::SDO_PIN, &(*m_portc)[2], &(*m_porta)[4]); //SSP SDO
    apfcon.set_pins(7, &usart, USART_MODULE::RX_PIN, &(*m_portc)[5], &(*m_porta)[1]); //USART RX Pin
    comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[1], &(*m_portc)[1],
                                        &(*m_portc)[2],  &(*m_portc)[3]);
    comparator.cmxcon1[1]->set_INpinNeg(&(*m_porta)[1], &(*m_portc)[1],
                                        &(*m_portc)[2],  &(*m_portc)[3]);
    comparator.cmxcon1[1]->set_INpinPos(&(*m_portc)[0]);
    comparator.cmxcon1[0]->set_OUTpin(&(*m_porta)[2]);
    comparator.cmxcon1[1]->set_OUTpin(&(*m_portc)[4]);
    comparator.cmxcon0[0]->setBitMask(0xf7);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, (1 << 5)));
    comparator.cmxcon0[1]->setBitMask(0xf7);
    comparator.cmxcon0[1]->setIntSrc(new InterruptSource(pir2, (1 << 6)));
    comparator.cmxcon1[0]->setBitMask(0xf3);
    comparator.cmxcon1[1]->setBitMask(0xf3);
    comparator.Nmask[2] = CMxCON1::CM_PIN;
    comparator.Nmask[3] = CMxCON1::CM_PIN;
    cpscon0.set_pin(4, &(*m_portc)[0]);
    cpscon0.set_pin(5, &(*m_portc)[1]);
    cpscon0.set_pin(6, &(*m_portc)[2]);
    cpscon0.set_pin(7, &(*m_portc)[3]);
    sr_module.srcon1->set_ValidBits(0xff);
    sr_module.setPins(&(*m_porta)[1], &(*m_porta)[2], &(*m_portc)[4]);
}


//========================================================================


P16LF1823::P16LF1823(const char *_name, const char *desc)
    : P16F1823(_name, desc)
{
}


P16LF1823::~P16LF1823()
{
}


void  P16LF1823::create(int ram_top, int eeprom_size, int dev_id)
{
    P16F1823::create(ram_top, eeprom_size, dev_id);
}


Processor * P16LF1823::construct(const char *name)
{
    P16LF1823 *p = new P16LF1823(name);
    p->create(0x7f, 256, 0x2820);
    p->create_invalid_registers();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}


//========================================================================
Processor * P16F1825::construct(const char *name)
{
    P16F1825 *p = new P16F1825(name);
    p->create(0x7f, 256, 0x2760);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


P16F1825::P16F1825(const char *_name, const char *desc) :
    P16F1823(_name, desc),
    pie3(this, "pie3", "Peripheral Interrupt Enable"),
    t4con(this, "t4con", "TMR4 Control"),
    pr4(this, "pr4", "TMR4 Period Register"),
    tmr4(this, "tmr4", "TMR4 Register"),
    t6con(this, "t6con", "TMR6 Control"),
    pr6(this, "pr6", "TMR6 Period Register"),
    tmr6(this, "tmr6", "TMR6 Register"),
    ccp2con(this, "ccp2con", "Capture Compare Control"),
    ccpr2l(this, "ccpr2l", "Capture Compare 2 Low"),
    ccpr2h(this, "ccpr2h", "Capture Compare 2 High"),
    pwm2con(this, "pwm2con", "Enhanced PWM Control Register"),
    ccp2as(this, "ccp2as", "CCP2 Auto-Shutdown Control Register"),
    pstr2con(this, "pstr2con", "Pulse Sterring Control Register"),
    ccp3con(this, "ccp3con", "Capture Compare Control"),
    ccpr3l(this, "ccpr3l", "Capture Compare 3 Low"),
    ccpr3h(this, "ccpr3h", "Capture Compare 3 High"),
    ccp4con(this, "ccp4con", "Capture Compare Control"),
    ccpr4l(this, "ccpr4l", "Capture Compare 4 Low"),
    ccpr4h(this, "ccpr4h", "Capture Compare 4 High"),
    ccptmrs(this, "ccptmrs", "PWM Timer Selection Control Register"),
    apfcon0(this, "apfcon0", "Alternate Pin Function Control Register 0", 0xec),
    apfcon1(this, "apfcon1", "Alternate Pin Function Control Register 1", 0x0f),
    inlvla(this, "inlvla", "PORTA Input Level Control Register"),
    inlvlc(this, "inlvlc", "PORTC Input Level Control Register")
{
    pir3 = new PIR(this, "pir3", "Peripheral Interrupt Register", intcon, &pie3, 0x3a);
}


P16F1825::~P16F1825()
{
    delete_file_registers(0xc0, 0xef);
    delete_file_registers(0x120, 0x16f);
    delete_file_registers(0x1a0, 0x1ef);
    delete_file_registers(0x220, 0x26f);
    delete_file_registers(0x2a0, 0x2ef);
    delete_file_registers(0x320, 0x32f);
    delete_file_registers(0x420, 0x46f);
    delete_file_registers(0x4a0, 0x4ef);
    delete_file_registers(0x520, 0x56f);
    delete_file_registers(0x5a0, 0x5ef);
    delete_sfr_register(pir3);
    remove_sfr_register(&pie3);
    remove_sfr_register(&ccpr2l);
    remove_sfr_register(&ccpr2h);
    remove_sfr_register(&ccp2con);
    remove_sfr_register(&pwm2con);
    remove_sfr_register(&ccp2as);
    remove_sfr_register(&pstr2con);
    remove_sfr_register(&ccptmrs);
    remove_sfr_register(&ccpr3l);
    remove_sfr_register(&ccpr3h);
    remove_sfr_register(&ccp3con);
    remove_sfr_register(&ccpr4l);
    remove_sfr_register(&ccpr4h);
    remove_sfr_register(&ccp4con);
    remove_sfr_register(&apfcon1);
    remove_sfr_register(&inlvla);
    remove_sfr_register(&inlvlc);
    remove_sfr_register(&tmr4);
    remove_sfr_register(&pr4);
    remove_sfr_register(&t4con);
    remove_sfr_register(&tmr6);
    remove_sfr_register(&pr6);
    remove_sfr_register(&t6con);
}


void  P16F1825::create(int ram_top, int eeprom_size, int dev_id)
{
    P16F1823::create(ram_top, eeprom_size, dev_id);
    pir_set_2_def.set_pir3(pir3);
    pie3.setPir(pir3);
    // SB The DS41440A datasheet shows an illogical memory layout for the
    // general purpose RAM that also doesn't add up to the advertised 1024
    // bytes. The layout presented in the DS40001440E datasheet and the
    // linker script seems much more reasonable.
    add_file_registers(0xc0, 0xef, 0x00);
    add_file_registers(0x120, 0x16f, 0x00);
    add_file_registers(0x1a0, 0x1ef, 0x00);
    add_file_registers(0x220, 0x26f, 0x00);
    add_file_registers(0x2a0, 0x2ef, 0x00);
    add_file_registers(0x320, 0x36f, 0x00);
    add_file_registers(0x3a0, 0x3ef, 0x00);
    add_file_registers(0x420, 0x46f, 0x00);
    add_file_registers(0x4a0, 0x4ef, 0x00);
    add_file_registers(0x520, 0x56f, 0x00);
    add_file_registers(0x5a0, 0x5ef, 0x00);
    add_file_registers(0x620, 0x64f, 0x00);
    add_sfr_register(pir3,         0x013, RegisterValue(0, 0));
    add_sfr_register(&pie3,        0x093, RegisterValue(0, 0));
    add_sfr_register(&apfcon1,     0x11e, RegisterValue(0, 0));
    add_sfr_register(&ccpr2l,      0x298, RegisterValue(0, 0));
    add_sfr_register(&ccpr2h,      0x299, RegisterValue(0, 0));
    add_sfr_registerR(&ccp2con,    0x29a, RegisterValue(0, 0));
    add_sfr_register(&pwm2con,     0x29b, RegisterValue(0, 0));
    add_sfr_register(&ccp2as,      0x29c, RegisterValue(0, 0));
    add_sfr_register(&pstr2con,    0x29d, RegisterValue(1, 0));
    ccptmrs.set_tmr246(&tmr2, &tmr4, &tmr6);
    ccptmrs.set_ccp(&ccp1con, &ccp2con, &ccp3con, &ccp4con);
    add_sfr_registerR(&ccptmrs,    0x29e, RegisterValue(0, 0));
    tmr2.add_ccp(&ccp2con);
    add_sfr_register(&ccpr3l,      0x311, RegisterValue(0, 0));
    add_sfr_register(&ccpr3h,      0x312, RegisterValue(0, 0));
    add_sfr_registerR(&ccp3con,    0x313, RegisterValue(0, 0));
    add_sfr_register(&ccpr4l,      0x318, RegisterValue(0, 0));
    add_sfr_register(&ccpr4h,      0x319, RegisterValue(0, 0));
    add_sfr_registerR(&ccp4con,    0x31a, RegisterValue(0, 0));
    add_sfr_register(&inlvla,	   0x38c, RegisterValue(0, 0));
    add_sfr_register(&inlvlc,	   0x38e, RegisterValue(0, 0));
    add_sfr_register(&tmr4,        0x415, RegisterValue(0, 0));
    add_sfr_register(&pr4,         0x416, RegisterValue(0xff, 0));
    add_sfr_register(&t4con,       0x417, RegisterValue(0, 0));
    add_sfr_register(&tmr6,        0x41c, RegisterValue(0, 0));
    add_sfr_register(&pr6,         0x41d, RegisterValue(0xff, 0));
    add_sfr_register(&t6con,       0x41e, RegisterValue(0, 0));
    ccp1con.setBitMask(0xff);
    ccp1con.setIOpin(&(*m_portc)[5], &(*m_portc)[4], &(*m_portc)[3], &(*m_portc)[2]);
    ccp2as.setIOpin(0, 0, &(*m_porta)[2]);
    ccp2as.link_registers(&pwm2con, &ccp2con);
    ccp2con.setBitMask(0xff);
    ccp2con.setIOpin(&(*m_portc)[3], &(*m_portc)[2]);
    ccp2con.pstrcon = &pstr2con;
    ccp2con.pwm1con = &pwm2con;
    ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v1822::CCP2IF, &tmr2, &ccp2as);
    ccpr2l.ccprh  = &ccpr2h;
    ccpr2l.tmrl   = &tmr1l;
    ccpr2h.ccprl  = &ccpr2l;
    ccp3con.setCrosslinks(&ccpr3l, pir3, (1 << 4), 0, 0);
    ccp3con.setIOpin(&(*m_porta)[2]);
    ccpr3l.ccprh  = &ccpr3h;
    ccpr3l.tmrl   = &tmr1l;
    ccpr3h.ccprl  = &ccpr3l;
    ccp4con.setCrosslinks(&ccpr4l, pir3, (1 << 5), 0, 0);
    ccp4con.setIOpin(&(*m_portc)[1]);
    ccpr4l.ccprh  = &ccpr4h;
    ccpr4l.tmrl   = &tmr1l;
    ccpr4h.ccprl  = &ccpr4l;
    t4con.tmr2 = &tmr4;
    tmr4.setInterruptSource(new InterruptSource(pir3, 1 << 1));
    tmr4.pr2    = &pr4;
    tmr4.t2con  = &t4con;
    t6con.tmr2 = &tmr6;
    tmr6.setInterruptSource(new InterruptSource(pir3, 1 << 3));
    tmr6.pr2    = &pr6;
    tmr6.t2con  = &t6con;
    pr2.tmr2    = &tmr2;
    pr4.tmr2    = &tmr4;
    pr6.tmr2    = &tmr6;
    apfcon0.set_pins(2, &usart, USART_MODULE::TX_PIN, &(*m_portc)[4], &(*m_porta)[0]); //USART TX Pin
    apfcon0.set_pins(3, &t1con_g.t1gcon, 0, &(*m_porta)[4], &(*m_porta)[3]); //tmr1 gate
    apfcon0.set_pins(5, &ssp, SSP1_MODULE::SS_PIN, &(*m_portc)[3], &(*m_porta)[3]); //SSP SS
    apfcon0.set_pins(6, &ssp, SSP1_MODULE::SDO_PIN, &(*m_portc)[2], &(*m_porta)[4]); //SSP SDO
    apfcon0.set_pins(7, &usart, USART_MODULE::RX_PIN, &(*m_portc)[5], &(*m_porta)[1]); //USART RX Pin
    apfcon1.set_pins(0, &ccp2con, CCPCON::CCP_PIN, &(*m_portc)[3], &(*m_porta)[5]); //CCP2/P2A
    apfcon1.set_pins(1, &ccp2con, CCPCON::PxB_PIN, &(*m_portc)[2], &(*m_porta)[4]); //P2B
    apfcon1.set_pins(2, &ccp1con, CCPCON::PxC_PIN, &(*m_portc)[3], &(*m_portc)[1]); //P1C
    apfcon1.set_pins(3, &ccp1con, CCPCON::PxD_PIN, &(*m_portc)[2], &(*m_portc)[0]); //P1D
}


//========================================================================
Processor * P16LF1825::construct(const char *name)
{
    P16LF1825 *p = new P16LF1825(name);
    p->create(0x7f, 256, 0x2860);
    p->create_invalid_registers();
    p->create_symbols();
    p->set_Vdd(3.3);
    return p;
}


P16LF1825::P16LF1825(const char *_name, const char *desc) :
    P16F1825(_name, desc)
{
}


P16LF1825::~P16LF1825()
{
}


void  P16LF1825::create(int ram_top, int eeprom_size, int dev_id)
{
    P16F1825::create(ram_top, eeprom_size, dev_id);
}
