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


//
// p16x6x
//
//  This file supports:
//    P16C61
//    P16C63
//    P16C64
//    P16C65
//    P16F630
//    P16F676
//    P16F610
//    P16F616

#include <stdio.h>
#include <iostream>
#include <string>

#include "p16x6x.h"

#include "intcon.h"
#include "packages.h"
#include "pic-ioports.h"
#include "pic-registers.h"
#include "stimuli.h"
#include "tmr0.h"
#include "ui.h"

class Processor;

void P16C61::create()
{
    create_iopin_map();
    _14bit_processor::create();
    add_file_registers(0x0c, 0x2f, 0x80);
    Pic14Bit::create_sfr_map();
}


Processor * P16C61::construct(const char *name)
{
    P16C61 *p = new P16C61(name);

    if (verbose)
    {
        std::cout << " c61 construct\n";
    }

    p->create();
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


P16C61::P16C61(const char *_name, const char *desc)
    : P16X8X(_name, desc)
{
    ram_top = 0x2f;
}


P16C61::~P16C61()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }
}


//------------------------------------------------------------------------
//
//

void P16C62::create_iopin_map()
{
    package = new Package(28);

    package->assign_pin(1, 0);
    package->assign_pin(2, m_porta->addPin(new IO_bi_directional("porta0"), 0));
    package->assign_pin(3, m_porta->addPin(new IO_bi_directional("porta1"), 1));
    package->assign_pin(4, m_porta->addPin(new IO_bi_directional("porta2"), 2));
    package->assign_pin(5, m_porta->addPin(new IO_bi_directional("porta3"), 3));
    package->assign_pin(6, m_porta->addPin(new IO_open_collector("porta4"), 4));
    package->assign_pin(7, m_porta->addPin(new IO_bi_directional("porta5"), 5));
    package->assign_pin(8, 0); //VSS
    package->assign_pin(9, 0);  // OSC
    package->assign_pin(10, 0); // OSC
    package->assign_pin(11, m_portc->addPin(new IO_bi_directional("portc0"), 0));
    package->assign_pin(12, m_portc->addPin(new IO_bi_directional("portc1"), 1));
    package->assign_pin(13, m_portc->addPin(new IO_bi_directional("portc2"), 2));
    package->assign_pin(14, m_portc->addPin(new IO_bi_directional("portc3"), 3));
    package->assign_pin(15, m_portc->addPin(new IO_bi_directional("portc4"), 4));
    package->assign_pin(16, m_portc->addPin(new IO_bi_directional("portc5"), 5));
    package->assign_pin(17, m_portc->addPin(new IO_bi_directional("portc6"), 6));
    package->assign_pin(18, m_portc->addPin(new IO_bi_directional("portc7"), 7));
    package->assign_pin(19, 0); //VSS
    package->assign_pin(20, 0); //VDD
    package->assign_pin(21, m_portb->addPin(new IO_bi_directional_pu("portb0"), 0));
    package->assign_pin(22, m_portb->addPin(new IO_bi_directional_pu("portb1"), 1));
    package->assign_pin(23, m_portb->addPin(new IO_bi_directional_pu("portb2"), 2));
    package->assign_pin(24, m_portb->addPin(new IO_bi_directional_pu("portb3"), 3));
    package->assign_pin(25, m_portb->addPin(new IO_bi_directional_pu("portb4"), 4));
    package->assign_pin(26, m_portb->addPin(new IO_bi_directional_pu("portb5"), 5));
    package->assign_pin(27, m_portb->addPin(new IO_bi_directional_pu("portb6"), 6));
    package->assign_pin(28, m_portb->addPin(new IO_bi_directional_pu("portb7"), 7));

    if (hasSSP())
    {
        ssp.initialize(
            get_pir_set(),    // PIR
            & (*m_portc)[3],  // SCK
            & (*m_porta)[5],  // SS
            & (*m_portc)[5],  // SDO
            & (*m_portc)[4],  // SDI
            m_trisc,         // I2C port
            SSP_TYPE_BSSP
        );
    }

    tmr1l.setIOpin(&(*m_portc)[0]);
}


//------------------------------------------------------------------------
//
//

void P16C64::create_iopin_map()
{
    package = new Package(40);

    // Now Create the package and place the I/O pins
    package->assign_pin(1, 0);
    package->assign_pin(2, m_porta->addPin(new IO_bi_directional("porta0"), 0));
    package->assign_pin(3, m_porta->addPin(new IO_bi_directional("porta1"), 1));
    package->assign_pin(4, m_porta->addPin(new IO_bi_directional("porta2"), 2));
    package->assign_pin(5, m_porta->addPin(new IO_bi_directional("porta3"), 3));
    package->assign_pin(6, m_porta->addPin(new IO_open_collector("porta4"), 4));
    package->assign_pin(7, m_porta->addPin(new IO_bi_directional("porta5"), 5));
    package->assign_pin(8, m_porte->addPin(new IO_bi_directional("porte0"), 0));
    package->assign_pin(9, m_porte->addPin(new IO_bi_directional("porte1"), 1));
    package->assign_pin(10, m_porte->addPin(new IO_bi_directional("porte2"), 2));
    package->assign_pin(11, 0);
    package->assign_pin(12, 0);
    package->assign_pin(13, 0);
    package->assign_pin(14, 0);
    package->assign_pin(15, m_portc->addPin(new IO_bi_directional("portc0"), 0));
    package->assign_pin(16, m_portc->addPin(new IO_bi_directional("portc1"), 1));
    package->assign_pin(17, m_portc->addPin(new IO_bi_directional("portc2"), 2));
    package->assign_pin(18, m_portc->addPin(new IO_bi_directional("portc3"), 3));
    package->assign_pin(23, m_portc->addPin(new IO_bi_directional("portc4"), 4));
    package->assign_pin(24, m_portc->addPin(new IO_bi_directional("portc5"), 5));
    package->assign_pin(25, m_portc->addPin(new IO_bi_directional("portc6"), 6));
    package->assign_pin(26, m_portc->addPin(new IO_bi_directional("portc7"), 7));
    package->assign_pin(19, m_portd->addPin(new IO_bi_directional("portd0"), 0));
    package->assign_pin(20, m_portd->addPin(new IO_bi_directional("portd1"), 1));
    package->assign_pin(21, m_portd->addPin(new IO_bi_directional("portd2"), 2));
    package->assign_pin(22, m_portd->addPin(new IO_bi_directional("portd3"), 3));
    package->assign_pin(27, m_portd->addPin(new IO_bi_directional("portd4"), 4));
    package->assign_pin(28, m_portd->addPin(new IO_bi_directional("portd5"), 5));
    package->assign_pin(29, m_portd->addPin(new IO_bi_directional("portd6"), 6));
    package->assign_pin(30, m_portd->addPin(new IO_bi_directional("portd7"), 7));
    package->assign_pin(31, 0);
    package->assign_pin(32, 0);
    package->assign_pin(33, m_portb->addPin(new IO_bi_directional_pu("portb0"), 0));
    package->assign_pin(34, m_portb->addPin(new IO_bi_directional_pu("portb1"), 1));
    package->assign_pin(35, m_portb->addPin(new IO_bi_directional_pu("portb2"), 2));
    package->assign_pin(36, m_portb->addPin(new IO_bi_directional_pu("portb3"), 3));
    package->assign_pin(37, m_portb->addPin(new IO_bi_directional_pu("portb4"), 4));
    package->assign_pin(38, m_portb->addPin(new IO_bi_directional_pu("portb5"), 5));
    package->assign_pin(39, m_portb->addPin(new IO_bi_directional_pu("portb6"), 6));
    package->assign_pin(40, m_portb->addPin(new IO_bi_directional_pu("portb7"), 7));

    if (hasSSP())
    {
        ssp.initialize(
            get_pir_set(),    // PIR
            & (*m_portc)[3],  // SCK
            & (*m_porta)[5],  // SS
            & (*m_portc)[5],  // SDO
            & (*m_portc)[4],  // SDI
            m_trisc,         // I2C port
            SSP_TYPE_BSSP
        );
    }

    psp.initialize(get_pir_set(),    // PIR
                   m_portd,           // Parallel port
                   m_trisd,           // Parallel tris
                   m_trise,           // Control tris
                   & (*m_porte)[0],   // NOT RD
                   & (*m_porte)[1],   // NOT WR
                   & (*m_porte)[2]);  // NOT CS
    tmr1l.setIOpin(&(*m_portc)[0]);
}


//---------------------------------------------------------
//
//  P16x6x::create_sfr_map(void) - Here's where all of the
//  registers are defined for a p16c63 and greater...

void P16X6X_processor::create_sfr_map()
{
    if (verbose)
    {
        std::cout << "P16X6X_processor::create_sfr_map\n";
    }

    Pic14Bit::create_sfr_map();
    // P16x63 and higher have porta5
    m_porta->setEnableMask(0x3f);
    m_porta->setTris(m_trisa);
    // The 16c62,c64 have general purpose registers
    // at addresses 20-7f and a0-bf
    add_file_registers(0x20, 0x7f, 0);
    add_file_registers(0xa0, 0xbf, 0);
    add_sfr_register(pir1,   0x0c, RegisterValue(0, 0), "pir1");
    add_sfr_register(&pie1,   0x8c, RegisterValue(0, 0));
    add_sfr_register(&tmr1l,  0x0e, RegisterValue(0, 0), "tmr1l");
    add_sfr_register(&tmr1h,  0x0f, RegisterValue(0, 0), "tmr1h");
    add_sfr_register(&pcon,   0x8e, RegisterValue(0, 0), "pcon");
    add_sfr_register(&t1con,  0x10, RegisterValue(0, 0));
    add_sfr_register(&tmr2,   0x11, RegisterValue(0, 0));
    add_sfr_register(&t2con,  0x12, RegisterValue(0, 0));
    add_sfr_register(&pr2,    0x92, RegisterValue(0xff, 0));

    if (hasSSP())
    {
        add_sfr_register(&ssp.sspbuf,  0x13, RegisterValue(0, 0), "sspbuf");
        add_sfr_register(&ssp.sspcon,  0x14, RegisterValue(0, 0), "sspcon");
        add_sfr_register(&ssp.sspadd,  0x93, RegisterValue(0, 0), "sspadd");
        add_sfr_register(&ssp.sspstat, 0x94, RegisterValue(0, 0), "sspstat");
        tmr2.ssp_module[0] = &ssp;
    }

    add_sfr_register(&ccpr1l,  0x15, RegisterValue(0, 0));
    add_sfr_register(&ccpr1h,  0x16, RegisterValue(0, 0));
    add_sfr_register(&ccp1con, 0x17, RegisterValue(0, 0));
    // get_pir_set()->set_pir1(get_pir1());
    pir_set_def.set_pir1(pir1);
    intcon = &intcon_reg;
    intcon_reg.set_pir_set(get_pir_set());
    // Maybe there's a better place for this, but let's go ahead and link all
    // of the registers together (there's probably a better way too) :
    tmr1l.tmrh = &tmr1h;
    tmr1l.t1con = &t1con;
    // FIXME -- can't delete this new'd item
    tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v1::TMR1IF));
    //  tmr1l.ccpcon = &ccp1con;
    tmr1h.tmrl  = &tmr1l;
    t1con.tmrl  = &tmr1l;
    t2con.tmr2  = &tmr2;
    tmr2.pir_set   = get_pir_set();
    tmr2.pr2    = &pr2;
    tmr2.t2con  = &t2con;
    tmr2.add_ccp(&ccp1con);
    tmr2.add_ccp(&ccp2con);
    pr2.tmr2    = &tmr2;
    ccp1con.setCrosslinks(&ccpr1l, pir1, PIR1v1::CCP1IF, &tmr2);
    ccp1con.setIOpin(&((*m_portc)[2]));
    ccpr1l.ccprh  = &ccpr1h;
    ccpr1l.tmrl   = &tmr1l;
    ccpr1h.ccprl  = &ccpr1l;
    //  portc->ccp1con = &ccp1con;
    ccpr1l.new_name("ccpr1l");
    ccpr1h.new_name("ccpr1h");
    ccp1con.new_name("ccp1con");

    if (pir1)
    {
        pir1->set_intcon(&intcon_reg);
        pir1->set_pie(&pie1);
    }

    pie1.setPir(pir1);
    int_pin.setIOpin(&((*m_portb)[0]), 0);
}


//--------------------------------------------------

void P16X6X_processor::create_symbols()
{
    Pic14Bit::create_symbols();
}


//--------------------------------------------------

P16X6X_processor::P16X6X_processor(const char *_name, const char *_desc)
    : Pic14Bit(_name, _desc),
      t1con(this, "t1con", "TMR1 Control"),
      pie1(this, "PIE1", "Peripheral Interrupt Enable"),
      pie2(this, "PIE2", "Peripheral Interrupt Enable"),
      t2con(this, "t2con", "TMR2 Control"),
      pr2(this, "pr2", "TMR2 Period Register"),
      tmr2(this, "tmr2", "TMR2 Register"),
      tmr1l(this, "tmr1l", "TMR1 Low"),
      tmr1h(this, "tmr1h", "TMR1 High"),
      ccp1con(this, "ccp1con", "Capture Compare Control"),
      ccpr1l(this, "ccpr1l", "Capture Compare 1 Low"),
      ccpr1h(this, "ccpr1h", "Capture Compare 1 High"),
      ccp2con(this, "ccp2con", "Capture Compare Control"),
      ccpr2l(this, "ccpr2l", "Capture Compare 2 Low"),
      ccpr2h(this, "ccpr2h", "Capture Compare 2 High"),
      pcon(this, "pcon", "pcon"),
      ssp(this)
{
    if (verbose)
    {
        std::cout << "generic 16X6X constructor, type = " << isa() << '\n';
    }

    m_portc = new PicPortRegister(this, "portc", "", 8, 0xff);
    m_trisc = new PicTrisRegister(this, "trisc", "", m_portc, false);
    pir1 = new PIR1v1(this, "pir1", "Peripheral Interrupt Register", &intcon_reg, &pie1);
    pir2 = new PIR2v1(this, "pir2", "Peripheral Interrupt Register", &intcon_reg, &pie2);
}


P16X6X_processor::~P16X6X_processor()
{
    remove_sfr_register(&tmr1l);
    remove_sfr_register(&tmr1h);
    remove_sfr_register(&pcon);
    remove_sfr_register(&t1con);
    remove_sfr_register(&tmr2);
    remove_sfr_register(&t2con);
    remove_sfr_register(&pr2);

    if (hasSSP())
    {
        remove_sfr_register(&ssp.sspbuf);
        remove_sfr_register(&ssp.sspcon);
        remove_sfr_register(&ssp.sspadd);
        remove_sfr_register(&ssp.sspstat);
    }

    remove_sfr_register(&ccpr1l);
    remove_sfr_register(&ccpr1h);
    remove_sfr_register(&ccp1con);
    remove_sfr_register(&pie1);
    delete_file_registers(0x20, 0x7f);
    delete_file_registers(0xa0, 0xbf);
    delete_sfr_register(m_portc);
    delete_sfr_register(m_trisc);

    if (verbose)
    {
        std::cout << "deleting PIR2:\n";
    }

    delete_sfr_register(pir2);

    if (verbose)
    {
        std::cout << "deleting PIR1:\n";
    }

    delete_sfr_register(pir1);
}


/*******************************************************************
 *
 *        Definitions for the various P16x6x processors
 *
 */


P16C62::P16C62(const char *_name, const char *desc)
    : P16X6X_processor(_name, desc)
{
    if (verbose)
    {
        std::cout << "c62 constructor, type = " << isa() << '\n';
    }

    set_hasSSP();
}


void P16C62::create_sfr_map()
{
    if (verbose)
    {
        std::cout << "creating c62 registers\n";
    }

    P16X6X_processor::create_sfr_map();
    add_sfr_register(m_portc, 0x07);
    add_sfr_register(m_trisc, 0x87, RegisterValue(0xff, 0));
    //1((PORTC*)portc)->ccp1con = &ccp1con;
}


void P16C62::create_symbols()
{
    if (verbose)
    {
        std::cout << "creating c62 symbols\n";
    }

    P16X6X_processor::create_symbols();
}


void  P16C62::create()
{
    if (verbose)
    {
        std::cout << " c62 create \n";
    }

    create_iopin_map();
    _14bit_processor::create();
    P16C62::create_sfr_map();
    // Build the links between the I/O Pins and the internal peripherals
    //1ccp1con.iopin = portc->pins[2];
}


Processor * P16C62::construct(const char *name)
{
    P16C62 *p = new P16C62(name);
    std::cout << " c62 construct\n";
    p->create();
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


//------------------------------------------------------------------------
//
//

void P16C63::create_sfr_map(void)
{
    if (verbose)
    {
        std::cout << "creating c63 registers\n";
    }

    add_file_registers(0xc0, 0xff, 0);
    add_sfr_register(pir2,    0x0d, RegisterValue(0, 0), "pir2");
    add_sfr_register(&pie2,   0x8d, RegisterValue(0, 0));
    add_sfr_register(&ccpr2l, 0x1b, RegisterValue(0, 0));
    add_sfr_register(&ccpr2h, 0x1c, RegisterValue(0, 0));
    add_sfr_register(&ccp2con, 0x1d, RegisterValue(0, 0));
    // get_pir_set()->set_pir2(get_pir2());
    pir_set_def.set_pir2(pir2);
    ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v1::CCP2IF, &tmr2);
    ccp2con.setIOpin(&((*m_portc)[1]));
    ccpr2l.ccprh  = &ccpr2h;
    ccpr2l.tmrl   = &tmr1l;
    ccpr2h.ccprl  = &ccpr2l;
    usart.initialize(pir1, &(*m_portc)[6], &(*m_portc)[7],
                     new _TXREG(this, "txreg", "USART Transmit Register", &usart),
                     new _RCREG(this, "rcreg", "USART Receiver Register", &usart));
    add_sfr_register(&usart.rcsta, 0x18, RegisterValue(0, 0), 0);
    add_sfr_register(&usart.txsta, 0x98, RegisterValue(2, 0), "txsta");
    add_sfr_register(&usart.spbrg, 0x99, RegisterValue(0, 0), "spbrg");
    add_sfr_register(usart.txreg,  0x19, RegisterValue(0, 0), "txreg");
    add_sfr_register(usart.rcreg,  0x1a, RegisterValue(0, 0), "rcreg");

    if (pir2)
    {
        pir2->set_intcon(&intcon_reg);
        pir2->set_pie(&pie2);
    }

    pie2.setPir(get_pir2());
}


void P16C63::create_symbols()
{
    if (verbose)
    {
        std::cout << "creating c63 symbols\n";
    }

    // There's nothing to create...
}


//------------------------------------------------------------------------
//
// P16C63 constructor
//
// Note: Since the 'C63 is derived from the 'C62. So before this constructor
// is called, the C62 constructor will be called. Most of the initialization
// is done within the 'C62 constructor.

P16C63::P16C63(const char *_name, const char *desc)
    : P16C62(_name, desc),
      usart(this)
{
    if (verbose)
    {
        std::cout << "c63 constructor, type = " << isa() << '\n';
    }
}


P16C63::~P16C63()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }

    remove_sfr_register(&pie2);
    remove_sfr_register(&ccpr2l);
    remove_sfr_register(&ccpr2h);
    remove_sfr_register(&ccp2con);

    if (registers[0xf0]->alias_mask & 0x80)
    {
        delete_file_registers(0xc0, 0xef);

    }
    else
    {
        delete_file_registers(0xc0, 0xff);
    }

    remove_sfr_register(&usart.rcsta);
    remove_sfr_register(&usart.txsta);
    remove_sfr_register(&usart.spbrg);
    delete_sfr_register(usart.txreg);
    delete_sfr_register(usart.rcreg);
    //delete_sfr_register(pir2,0x0d);
}


void P16C63::create()
{
    if (verbose)
    {
        std::cout << " c63 create \n";
    }

    P16C62::create();
    P16C63::create_sfr_map();
    // Build the links between the I/O Pins and the internal peripherals
    //1ccp2con.iopin = portc->pins[1];
}


Processor * P16C63::construct(const char *name)
{
    P16C63 *p = new P16C63(name);

    if (verbose)
    {
        std::cout << " c63 construct\n";
    }

    p->create();
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


//----------------------------------------------------------
//
//

void P16C64::create_sfr_map()
{
    if (verbose)
    {
        std::cout << "creating c64 registers\n";
    }

    pir_set_2_def.set_pir1(pir1_2_reg);
    P16X6X_processor::create_sfr_map();
    add_sfr_register(m_portc, 0x07);
    add_sfr_register(m_trisc, 0x87, RegisterValue(0xff, 0));
    add_sfr_register(m_portd, 0x08);
    add_sfr_register(m_trisd, 0x88, RegisterValue(0xff, 0));
    add_sfr_register(m_porte, 0x09);
    add_sfr_register(m_trise, 0x89, RegisterValue(0x07, 0));
    //1((PORTC*)portc)->ccp1con = &ccp1con;
}


void P16C64::create_symbols()
{
    if (verbose)
    {
        std::cout << "creating c64 symbols\n";
    }

    P16X6X_processor::create_symbols();
    addSymbol(m_portd);
    addSymbol(m_porte);
    addSymbol(m_trisd);
    addSymbol(m_trise);
}


void  P16C64::create()
{
    if (verbose)
    {
        std::cout << " c64 create \n";
    }

    create_iopin_map();
    _14bit_processor::create();
    //P16X6X_processor::create_sfr_map();
    P16C64::create_sfr_map();
    // Build the links between the I/O Pins and the internal peripherals
    //1ccp1con.iopin = portc->pins[2];
}


Processor * P16C64::construct(const char *name)
{
    P16C64 *p = new P16C64(name);
    p->create();
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


P16C64::P16C64(const char *_name, const char *desc)
    : P16X6X_processor(_name, desc)
{
    if (verbose)
    {
        std::cout << "c64 constructor, type = " << isa() << '\n';
    }

    set_hasSSP();
    pir1_2_reg = new PIR1v2(this, "pir1", "Peripheral Interrupt Register", &intcon_reg, &pie1);
    delete pir1;
    pir1 = pir1_2_reg;
    m_portd = new PicPSP_PortRegister(this, "portd", "", 8, 0xff);
    m_trisd = new PicTrisRegister(this, "trisd", "", (PicPortRegister *)m_portd, false);
    m_porte = new PicPortRegister(this, "porte", "", 8, 0x07);
    m_trise =  new PicPSP_TrisRegister(this, "trise", "", m_porte, false);
}


P16C64::~P16C64()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }

    delete_sfr_register(m_portd);
    delete_sfr_register(m_trisd);
    delete_sfr_register(m_porte);
    delete_sfr_register(m_trise);
}


//------------------------------------------------------------------------
//
//

void P16C65::create_sfr_map()
{
    if (verbose)
    {
        std::cout << "creating c65 registers\n";
    }

    //P16C64::create_sfr_map();
    add_file_registers(0xc0, 0xff, 0);
    add_sfr_register(pir2,    0x0d, RegisterValue(0, 0), "pir2");
    add_sfr_register(&pie2,   0x8d, RegisterValue(0, 0));
    add_sfr_register(&ccpr2l, 0x1b, RegisterValue(0, 0));
    add_sfr_register(&ccpr2h, 0x1c, RegisterValue(0, 0));
    add_sfr_register(&ccp2con, 0x1d, RegisterValue(0, 0));
    // get_pir_set()->set_pir2(&get_pir2());
    pir_set_def.set_pir2(pir2);
    ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2);
    ccp2con.setIOpin(&((*m_portc)[1]));
    ccpr2l.ccprh  = &ccpr2h;
    ccpr2l.tmrl   = &tmr1l;
    ccpr2h.ccprl  = &ccpr2l;
    usart.initialize(pir1, &(*m_portc)[6], &(*m_portc)[7],
                     new _TXREG(this, "txreg", "USART Transmit Register", &usart),
                     new _RCREG(this, "rcreg", "USART Receiver Register", &usart));
    add_sfr_register(&usart.rcsta, 0x18, RegisterValue(0, 0), "rcsta");
    add_sfr_register(&usart.txsta, 0x98, RegisterValue(2, 0), "txsta");
    add_sfr_register(&usart.spbrg, 0x99, RegisterValue(0, 0), "spbrg");
    add_sfr_register(usart.txreg, 0x19, RegisterValue(0, 0), "txreg");
    add_sfr_register(usart.rcreg, 0x1a, RegisterValue(0, 0), "rcreg");

    if (pir2)
    {
        pir2->set_intcon(&intcon_reg);
        pir2->set_pie(&pie2);
    }

    pie2.setPir(get_pir2());
}


void P16C65::create_symbols()
{
    if (verbose)
    {
        std::cout << "creating c65 symbols\n";
    }

    // There's nothing to create...
}


//------------------------------------------------------------------------
//
// P16C65 constructor
//
// Note: Since the 'C65 is derived from the 'C64. So before this constructor
// is called, the C64 constructor will be called. Most of the initialization
// is done within the 'C64 constructor.

P16C65::P16C65(const char *_name, const char *desc)
    : P16C64(_name, desc),
      usart(this)
{
    if (verbose)
    {
        std::cout << "c65 constructor, type = " << isa() << '\n';
    }
}


P16C65::~P16C65()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }

    if (registers[0xf0]->alias_mask & 0x80)
    {
        delete_file_registers(0xc0, 0xef);

    }
    else
    {
        delete_file_registers(0xc0, 0xff);
    }

    remove_sfr_register(&ccpr2l);
    remove_sfr_register(&ccpr2h);
    remove_sfr_register(&ccp2con);
    remove_sfr_register(&pie2);
    remove_sfr_register(&usart.rcsta);
    remove_sfr_register(&usart.txsta);
    remove_sfr_register(&usart.spbrg);
    delete_sfr_register(usart.txreg);
    delete_sfr_register(usart.rcreg);
}


void P16C65::create()
{
    if (verbose)
    {
        std::cout << " c65 create \n";
    }

    P16C64::create();
    P16C65::create_sfr_map();
    // Build the links between the I/O Pins and the internal peripherals
    // ccp1con.iopin = portc.pins[2];
    //1ccp2con.iopin = portc->pins[1];
}


Processor * P16C65::construct(const char *name)
{
    P16C65 *p = new P16C65(name);

    if (verbose)
    {
        std::cout << " c65 construct\n";
    }

    p->create();
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


//========================================================================
//
// Configuration Memory for 16F630/676

class ConfigF630 : public ConfigWord
{
public:
    explicit ConfigF630(P16F630 *pCpu)
        : ConfigWord("CONFIG", 0x3fff, "Configuration Word", pCpu, 0x2007)
    {
        //Dprintf(("ConfigF630::ConfigF630 %p\n", m_pCpu));
        if (m_pCpu)
        {
            m_pCpu->set_config_word(0x2007, 0x3fff);
        }
    }

    enum
    {
        FOSC0  = 1 << 0,
        FOSC1  = 1 << 1,
        FOSC2  = 1 << 2,
        WDTEN  = 1 << 3,
        PWRTEN = 1 << 4,
        MCLRE =  1 << 5,
        BODEN =  1 << 6,
        CP =     1 << 7,
        CPD =    1 << 8
    };

    std::string toString() override
    {
        int64_t i64 = get();
        int i = i64 & 0xfff;
        char buff[356];
        const char *OSCdesc[8] =
        {
            "LP oscillator",
            "XT oscillator",
            "HS oscillator",
            "EC oscillator w/ OSC2 configured as I/O",
            "INTOSC oscillator: I/O on RA4 pin, I/O on RA5",
            "INTOSC oscillator: CLKOUT on RA4 pin, I/O on RA5",
            "RC oscillator: I/O on RA4 pin, RC on RA5",
            "RC oscillator: CLKOUT on RA4 pin, RC on RA5"
        };
        snprintf(buff, sizeof(buff),
                 " $%04x\n"
                 " FOSC=%d - Clk source = %s\n"
                 " WDTEN=%d - WDT is %s\n"
                 " PWRTEN=%d - Power up timer is %s\n"
                 " MCLRE=%d - RA3 Pin %s\n"
                 " BODEN=%d -  Brown-out Detect %s\n"
                 " CP=%d - Code Protection %s\n"
                 " CPD=%d -  Data Code Protection %s\n",
                 i,
                 i & (FOSC0 | FOSC1 | FOSC2), OSCdesc[i & (FOSC0 | FOSC1 | FOSC2)],
                 ((i & WDTE) ? 1 : 0), ((i & WDTE) ? "enabled" : "disabled"),
                 ((i & PWRTEN) ? 1 : 0), ((i & PWRTEN) ? "disabled" : "enabled"),
                 ((i & MCLRE) ? 1 : 0), ((i & MCLRE) ? "MCLR" : "Input"),
                 ((i & BODEN) ? 1 : 0), ((i & BODEN) ? "enabled" : "disabled"),
                 ((i & CP) ? 1 : 0), ((i & CP) ? "disabled" : "enabled"),
                 ((i & CPD) ? 1 : 0), ((i & CPD) ? "disabled" : "enabled")
                );
        return buff;
    }
};


//------------------------------------------------------------------------
//
//

P16F630::P16F630(const char *_name, const char *desc)
    : _14bit_processor(_name, desc),
      t1con(this, "t1con", "TMR1 Control"),
      pie1(this, "PIE1", "Peripheral Interrupt Enable"),
      tmr1l(this, "tmr1l", "TMR1 Low"),
      tmr1h(this, "tmr1h", "TMR1 High"),
      osccal(this, "osccal", "Oscillator Calibration Register", 0xfc),
      intcon_reg(this, "intcon", "Interrupt Control"),
      comparator(this), int_pin(this, &intcon_reg, 0)
{
    if (verbose)
    {
        std::cout << "P16F630 constructor, type = " << isa() << '\n';
    }

    pir1_3_reg = new PIR1v3(this, "pir1", "Peripheral Interrupt Register", &intcon_reg, &pie1);
    pir1 = pir1_3_reg;
    m_ioc = new IOC(this, "ioc", "Interrupt-On-Change GPIO Register");

    m_porta = new PicPortGRegister(this,"porta","",&intcon_reg, m_ioc, 8,0x3f);
    m_trisa = new PicTrisRegister(this,"trisa","", m_porta, false);

    m_wpu = new WPU(this, "wpu", "Weak Pull-up Register", m_porta, 0x37);
    tmr0.link_cpu(m_porta, 4, option_reg);
    tmr0.start(0);
    m_portc = new PicPortRegister(this, "portc", "", 8, 0x3f);
    m_trisc = new PicTrisRegister(this, "trisc", "", m_portc, false);
}


P16F630::~P16F630()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }

    unassignMCLRPin();
    delete_file_registers(0x20, 0x5f);
    delete_sfr_register(m_portc);
    delete_sfr_register(m_trisc);
    delete_sfr_register(m_porta);
    delete_sfr_register(m_trisa);
    delete_sfr_register(m_ioc);
    delete_sfr_register(m_wpu);
    delete_sfr_register(pir1_3_reg);
    remove_sfr_register(&tmr0);
    remove_sfr_register(&intcon_reg);
    remove_sfr_register(&tmr1l);
    remove_sfr_register(&tmr1h);
    remove_sfr_register(&t1con);
    remove_sfr_register(&comparator.cmcon);
    remove_sfr_register(&comparator.vrcon);
    remove_sfr_register(&osccal);
    remove_sfr_register(&pie1);
    delete e;
}


void P16F630::create_iopin_map()
{
    package = new Package(14);

    package->assign_pin(1, 0);	// Vdd
    package->assign_pin(2, m_porta->addPin(new IO_bi_directional_pu("porta5"), 5));
    package->assign_pin(3, m_porta->addPin(new IO_bi_directional_pu("porta4"), 4));
    package->assign_pin(4, m_porta->addPin(new IOPIN("porta3"), 3));
    package->assign_pin(5, m_portc->addPin(new IO_bi_directional_pu("portc5"), 5));
    package->assign_pin(6, m_portc->addPin(new IO_bi_directional("portc4"), 4));
    package->assign_pin(7, m_portc->addPin(new IO_bi_directional("portc3"), 3));
    package->assign_pin(8, m_portc->addPin(new IO_bi_directional("portc2"), 2));
    package->assign_pin(9, m_portc->addPin(new IO_bi_directional("portc1"), 1));
    package->assign_pin(10, m_portc->addPin(new IO_bi_directional("portc0"), 0));
    package->assign_pin(11, m_porta->addPin(new IO_bi_directional_pu("porta2"), 2));
    package->assign_pin(12, m_porta->addPin(new IO_bi_directional_pu("porta1"), 1));
    package->assign_pin(13, m_porta->addPin(new IO_bi_directional_pu("porta0"), 0));
    package->assign_pin(14, 0); //VSS
    tmr1l.setIOpin(&(*m_portc)[0]);
}


Processor * P16F630::construct(const char *name)
{
    P16F630 *p = new P16F630(name);

    if (verbose)
    {
        std::cout << " P16F630 construct\n";
    }

    p->create(128);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


void P16F630::create(int eesize)
{
    create_iopin_map();
    _14bit_processor::create();
    e = new EEPROM_WIDE(this, pir1);
    e->initialize(eesize);
    e->set_intcon(&intcon_reg);
    e->get_reg_eecon1()->set_valid_bits(0x0f);
    set_eeprom_wide(e);
    P16F630::create_sfr_map();
}


void P16F630::create_symbols()
{
    pic_processor::create_symbols();
    addSymbol(Wreg);
}


//-------------------------------------------------------------------
void P16F630::create_sfr_map()
{
    pir_set_def.set_pir1(pir1);
    add_file_registers(0x20, 0x5f, 0);
    alias_file_registers(0x20, 0x5f, 0x80);
    add_sfr_register(indf,    0x00);
    alias_file_registers(0x00, 0x00, 0x80);
    add_sfr_register(&tmr0,   0x01);
    add_sfr_register(option_reg,  0x81, RegisterValue(0xff, 0));
    add_sfr_register(pcl,     0x02, RegisterValue(0, 0));
    add_sfr_register(status,  0x03, RegisterValue(0x18, 0));
    add_sfr_register(fsr,     0x04);
    alias_file_registers(0x02, 0x04, 0x80);
    add_sfr_register(m_porta, 0x05);
    add_sfr_register(m_trisa, 0x85, RegisterValue(0x3f, 0));
    add_sfr_register(m_portc, 0x07);
    add_sfr_register(m_trisc, 0x87, RegisterValue(0xff, 0));
    add_sfr_register(pclath,  0x0a, RegisterValue(0, 0));
    add_sfr_register(&intcon_reg, 0x0b, RegisterValue(0, 0));
    alias_file_registers(0x0a, 0x0b, 0x80);
    add_sfr_register(pir1, 0x0c, RegisterValue(0, 0));
    add_sfr_register(&tmr1l, 0x0e, RegisterValue(0, 0), "tmr1l");
    add_sfr_register(&tmr1h, 0x0f, RegisterValue(0, 0), "tmr1h");
    add_sfr_register(&t1con, 0x10, RegisterValue(0, 0));
    intcon = &intcon_reg;
    intcon_reg.set_pir_set(get_pir_set());
    tmr1l.tmrh = &tmr1h;
    tmr1l.t1con = &t1con;
    // FIXME -- can't delete this new'd item
    tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v3::TMR1IF));
    tmr1h.tmrl  = &tmr1l;
    t1con.tmrl  = &tmr1l;
    tmr1l.setIOpin(&(*m_porta)[5]);
    tmr1l.setGatepin(&(*m_porta)[4]);
    add_sfr_register(&pie1,   0x8c, RegisterValue(0, 0));

    if (pir1)
    {
        pir1->set_intcon(&intcon_reg);
        pir1->set_pie(&pie1);
    }

    pie1.setPir(pir1);
    // Link the comparator and voltage ref to porta
    comparator.initialize(get_pir_set(), nullptr,
                          &(*m_porta)[0], &(*m_porta)[1],
                          nullptr, nullptr,
                          &(*m_porta)[2], nullptr);
    comparator.cmcon.set_configuration(1, 0, AN0, AN1, AN0, AN1, ZERO);
    comparator.cmcon.set_configuration(1, 1, AN0, AN1, AN0, AN1, OUT0);
    comparator.cmcon.set_configuration(1, 2, AN0, AN1, AN0, AN1, NO_OUT);
    comparator.cmcon.set_configuration(1, 3, AN1, VREF, AN1, VREF, OUT0);
    comparator.cmcon.set_configuration(1, 4, AN1, VREF, AN1, VREF, NO_OUT);
    comparator.cmcon.set_configuration(1, 5, AN1, VREF, AN0, VREF, OUT0);
    comparator.cmcon.set_configuration(1, 6, AN1, VREF, AN0, VREF, NO_OUT);
    comparator.cmcon.set_configuration(1, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 0, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 1, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 2, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 3, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 4, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 5, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 6, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    comparator.cmcon.set_configuration(2, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
    add_sfr_register(&comparator.cmcon, 0x19, RegisterValue(0, 0), "cmcon");
    add_sfr_register(&comparator.vrcon, 0x99, RegisterValue(0, 0), "cvrcon");
    add_sfr_register(get_eeprom()->get_reg_eedata(),  0x9a);
    add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x9b);
    add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x9c, RegisterValue(0, 0));
    add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x9d);
    add_sfr_register(m_wpu, 0x95, RegisterValue(0x37, 0), "wpua");
    add_sfr_register(m_ioc, 0x96, RegisterValue(0, 0), "ioca");
    add_sfr_register(&osccal, 0x90, RegisterValue(0x80, 0));
    int_pin.setIOpin(&(*m_porta)[2], 0);
}


//-------------------------------------------------------------------
void P16F630::option_new_bits_6_7(unsigned int bits)
{
    m_wpu->set_wpu_pu((bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
    m_porta->setIntEdge((bits & OPTION_REG::BIT6) == OPTION_REG::BIT6);
}


//-------------------------------------------------------------------
void P16F630::create_config_memory()
{
    m_configMemory = new ConfigMemory(this, 1);
    m_configMemory->addConfigWord(0, new ConfigF630(this));
}


//-------------------------------------------------------------------
bool P16F630::set_config_word(unsigned int address, unsigned int cfg_word)
{
    enum
    {
        CFG_FOSC0 = 1 << 0,
        CFG_FOSC1 = 1 << 1,
        CFG_FOSC2 = 1 << 2,
        CFG_WDTE  = 1 << 3,
        CFG_MCLRE = 1 << 5,
    };

    if (address == config_word_address())
    {
        unsigned int valid_pins = m_porta->getEnableMask();

        if ((cfg_word & CFG_MCLRE) == CFG_MCLRE)
        {
            assignMCLRPin(4);

        }
        else
        {
            unassignMCLRPin();
        }

        wdt->initialize((cfg_word & CFG_WDTE) == CFG_WDTE);
        set_int_osc(false);
        // AnalogReq is used so ADC does not change clock names
        // set_config_word is first called with default and then
        // often called a second time. the following call is to
        // reset porta so next call to AnalogReq sill set the pin name
        //
        (&(*m_porta)[4])->AnalogReq((Register *)this, false, "porta4");
        valid_pins |= 0x20;

        switch (cfg_word & (CFG_FOSC0 | CFG_FOSC1 | CFG_FOSC2))
        {
        case 0:  // LP oscillator: low power crystal is on RA4 and RA5
        case 1:     // XT oscillator: crystal/resonator is on RA4 and RA5
        case 2:     // HS oscillator: crystal/resonator is on RA4 and RA5
            (&(*m_porta)[4])->AnalogReq((Register *)this, true, "OSC2");
            (m_porta->getPin(5))->newGUIname("OSC1");
            valid_pins &= 0xcf;
            break;

        case 3:	// EC I/O on RA4 pin, CLKIN on RA5
            (m_porta->getPin(5))->newGUIname("CLKIN");
            valid_pins &= 0xef;
            break;

        case 5: // INTOSC CLKOUT on RA4 pin
            (&(*m_porta)[4])->AnalogReq((Register *)this, true, "CLKOUT");

        case 4: // INTOSC
            (m_porta->getPin(5))->newGUIname("porta5");
            set_int_osc(true);
            osccal.set_freq(4e6);
            break;

        case 6: //RC oscillator: I/O on RA4 pin, RC on RA5
            (m_porta->getPin(5))->newGUIname("RC");
            valid_pins &= 0xdf;
            break;

        case 7: // RC oscillator: CLKOUT on RA4 pin, RC on RA5
            (&(*m_porta)[4])->AnalogReq((Register *)this, true, "CLKOUT");
            (m_porta->getPin(5))->newGUIname("RC");
            valid_pins &= 0xdf;
            break;
        };

        if (valid_pins != m_porta->getEnableMask())   // enable new pins for IO
        {
            m_porta->setEnableMask(valid_pins);
            m_trisa->setEnableMask(valid_pins);
        }

        return (true);
    }

    return false;
}


//------------------------------------------------------------------------
//
//

P16F676::P16F676(const char *_name, const char *desc)
    : P16F630(_name, desc),
      ansel(this, "ansel", "Analog Select"),
      adcon0(this, "adcon0", "A2D Control 0"),
      adcon1(this, "adcon1", "A2D Control 1"),
      adresh(this, "adresh", "A2D Result High"),
      adresl(this, "adresl", "A2D Result Low")

{
}


P16F676::~P16F676()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }

    remove_sfr_register(&adresl);
    remove_sfr_register(&adresh);
    remove_sfr_register(&adcon0);
    remove_sfr_register(&adcon1);
    remove_sfr_register(&ansel);
}


Processor * P16F676::construct(const char *name)
{
    P16F676 *p = new P16F676(name);

    if (verbose)
    {
        std::cout << " P16F676 construct\n";
    }

    p->create(128);
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


void P16F676::create(int ram_top)
{
    P16F630::create(ram_top);
    create_sfr_map();
}


void P16F676::create_sfr_map()
{
    add_sfr_register(&adresl,  0x9e, RegisterValue(0, 0));
    add_sfr_register(&adresh,  0x1e, RegisterValue(0, 0));
    add_sfr_register(&adcon0, 0x1f, RegisterValue(0, 0));
    add_sfr_register(&adcon1, 0x9f, RegisterValue(0, 0));
    add_sfr_register(&ansel, 0x91, RegisterValue(0xff, 0));
    ansel.setAdcon1(&adcon1);
    //  ansel.setAdcon0(&adcon0);
    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    adcon0.setIntcon(&intcon_reg);
    adcon0.setA2DBits(10);
    adcon0.setPir(pir1);
    adcon0.setChannel_Mask(7);
    adcon0.setChannel_shift(2);
    adcon1.setAdcon0(&adcon0);
    adcon1.setNumberOfChannels(8);
    adcon1.setIOPin(0, &(*m_porta)[0]);
    adcon1.setIOPin(1, &(*m_porta)[1]);
    adcon1.setIOPin(2, &(*m_porta)[2]);
    adcon1.setIOPin(3, &(*m_porta)[4]);
    adcon1.setIOPin(4, &(*m_portc)[0]);
    adcon1.setIOPin(5, &(*m_portc)[1]);
    adcon1.setIOPin(6, &(*m_portc)[2]);
    adcon1.setIOPin(7, &(*m_portc)[3]);
    adcon1.setVrefHiConfiguration(2, 1);
    /* Channel Configuration done dynamiclly based on ansel */
}


//------------------------------------------------------------------------
//
//

class ConfigF610 : public ConfigWord
{
public:
    explicit ConfigF610(P16F610 *pCpu)
        : ConfigWord("CONFIG", 0x3fff, "Configuration Word", pCpu, 0x2007)
    {
        //Dprintf(("ConfigF610::ConfigF610 %p\n", m_pCpu));
        if (m_pCpu)
        {
            m_pCpu->set_config_word(0x2007, 0x3fff);
        }
    }

    enum
    {
        FOSC0  = 1 << 0,
        FOSC1  = 1 << 1,
        FOSC2  = 1 << 2,
        WDTEN  = 1 << 3,
        PWRTEN = 1 << 4,
        MCLRE =  1 << 5,
        CP =     1 << 6,
        IOSCFS = 1 << 7,
        BOREN0=  1 << 8,
        BOREN1=  1 << 9
    };

    std::string toString() override
    {
        int64_t i64 = get();
        int i = i64 & 0xfff;
        char buff[356];
        const char *OSCdesc[8] =
        {
            "LP oscillator",
            "XT oscillator",
            "HS oscillator",
            "EC oscillator w/ OSC2 configured as I/O",
            "INTOSC oscillator: I/O on RA4 pin, I/O on RA5",
            "INTOSC oscillator: CLKOUT on RA4 pin, I/O on RA5",
            "RC oscillator: I/O on RA4 pin, RC on RA5",
            "RC oscillator: CLKOUT on RA4 pin, RC on RA5"
        };
        const char *BORENdesc[4] =
        {
            "disabled",
            "disabled",
            "enabled except in Sleep",
            "enabled always"
        };
        snprintf(buff, sizeof(buff),
                 " $%04x\n"
                 " FOSC=%d - Clk source = %s\n"
                 " WDTEN=%d - WDT is %s\n"
                 " PWRTEN=%d - Power up timer is %s\n"
                 " MCLRE=%d - RA3 Pin %s\n"
                 " CP=%d - Code Protection %s\n"
                 " IOSCFS=%d - Internal Oscillator Frequency %s\n"
                 " BOREN=%d -  Brown-out Detect %s\n",
                 i,
                 i & (FOSC0 | FOSC1 | FOSC2), OSCdesc[i & (FOSC0 | FOSC1 | FOSC2)],
                 ((i & WDTE) ? 1 : 0), ((i & WDTE) ? "enabled" : "disabled"),
                 ((i & PWRTEN) ? 1 : 0), ((i & PWRTEN) ? "disabled" : "enabled"),
                 ((i & MCLRE) ? 1 : 0), ((i & MCLRE) ? "MCLR" : "Input"),
                 ((i & CP) ? 1 : 0), ((i & CP) ? "disabled" : "enabled"),
                 ((i & IOSCFS) ? 1 : 0), ((i & IOSCFS) ? "8 MHz" : "4 MHz"),
                 i & (BOREN0 | BOREN1), BORENdesc[i & (BOREN0 | BOREN1)]
                );
        return buff;
    }
};

P16F610::P16F610(const char *_name, const char *desc)
    : _14bit_processor(_name, desc),
      t1con(this, "t1con", "TMR1 Control"),
      tmr1l(this, "tmr1l", "TMR1 Low"),
      tmr1h(this, "tmr1h", "TMR1 High"),
      pcon(this, "pcon", "pcon"),
      osctune(this, "osctune", "Oscillator Tuning Register"),
      intcon_reg(this, "intcon", "Interrupt Control"),
      pie1(this, "pie1", "Peripheral Interrupt Enable"),
      pir1(this, "pir1", "Peripheral Interrupt Register", &intcon_reg, &pie1),
      ansel(this, "ansel", "Analog Select"),
      adcon1(this, "adcon1", "A2D Control 1"),
      ioca(this, "ioca", "Interrupt-On-Change GPIO Register"),
      porta(this, "porta", "", &intcon_reg, &ioca, 6, 0x3f),
      trisa(this, "trisa", "", &porta, false),
      wpua(this, "wpua", "Weak Pull-up Register", &porta, 0x37),
      portc(this, "portc", "", 6, 0x3f),
      trisc(this, "trisc", "", &portc, false),
      comparator(this),
      vrcon(this, "vrcon", "Voltage Reference Control Register"),
      sr_module(this),

      int_pin(this, &intcon_reg, 0)
{
    if (verbose)
    {
        std::cout << "P16F610 constructor, type = " << isa() << '\n';
    }

    osctune.valid_bits=0x1f;

    tmr0.link_cpu(&porta, 4, option_reg);
    tmr0.start(0);
    comparator.cmxcon0[0] = new CMxCON0_V2(this, "cm1con0",
                        " Comparator C1 Control Register 0", 0, &comparator);
    comparator.cmxcon0[1] = new CMxCON0_V2(this, "cm2con0",
                        " Comparator C2 Control Register 0", 1, &comparator);
    comparator.cmxcon1[0] = new CM2CON1_V4(this, "cm2con1",
                        " Comparator Control Register 2", 1, &comparator);
    comparator.cmxcon1[1] = comparator.cmxcon1[0];
    comparator.cmxcon1[0]->set_vrcon(&vrcon);
    comparator.cmxcon1[1]->set_vrcon(&vrcon);

}


P16F610::~P16F610()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }
    unassignMCLRPin();
    adcon1.detach_fvr();
    delete_file_registers(0x70, 0x7f);
    delete_file_registers(0x40, 0x6f);
    remove_sfr_register(&portc);
    remove_sfr_register(&trisc);
    remove_sfr_register(&porta);
    remove_sfr_register(&trisa);
    remove_sfr_register(&ioca);
    remove_sfr_register(&wpua);
    remove_sfr_register(&pir1);
    remove_sfr_register(&tmr0);
    remove_sfr_register(&intcon_reg);
    remove_sfr_register(&tmr1l);
    remove_sfr_register(&tmr1h);
    remove_sfr_register(&t1con);
    remove_sfr_register(&osctune);
    remove_sfr_register(&pcon);
    remove_sfr_register(&pie1);
    remove_sfr_register(&ansel);
    remove_sfr_register(&vrcon);
    remove_sfr_register(comparator.cmxcon0[0]);
    remove_sfr_register(comparator.cmxcon0[1]);
    remove_sfr_register(comparator.cmxcon1[1]);
    remove_sfr_register(sr_module.srcon0);
    remove_sfr_register(sr_module.srcon1);

}


void P16F610::create_iopin_map()
{
    package = new Package(14);

    package->assign_pin(1, 0);	// Vdd
    package->assign_pin(2, porta.addPin(new IO_bi_directional_pu("porta5"), 5));
    package->assign_pin(3, porta.addPin(new IO_bi_directional_pu("porta4"), 4));
    package->assign_pin(4, porta.addPin(new IOPIN("porta3"), 3));
    package->assign_pin(5, portc.addPin(new IO_bi_directional("portc5"), 5));
    package->assign_pin(6, portc.addPin(new IO_bi_directional("portc4"), 4));
    package->assign_pin(7, portc.addPin(new IO_bi_directional("portc3"), 3));
    package->assign_pin(8, portc.addPin(new IO_bi_directional("portc2"), 2));
    package->assign_pin(9, portc.addPin(new IO_bi_directional("portc1"), 1));
    package->assign_pin(10, portc.addPin(new IO_bi_directional("portc0"), 0));
    package->assign_pin(11, porta.addPin(new IO_bi_directional_pu("porta2"), 2));
    package->assign_pin(12, porta.addPin(new IO_bi_directional_pu("porta1"), 1));
    package->assign_pin(13, porta.addPin(new IO_bi_directional_pu("porta0"), 0));
    package->assign_pin(14, 0); //VSS
}


Processor * P16F610::construct(const char *name)
{
    P16F610 *p = new P16F610(name);

    if (verbose)
    {
        std::cout << " P16F610 construct\n";
    }

    p->create();
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


void P16F610::create(void)
{
    create_iopin_map();
    _14bit_processor::create();
    P16F610::create_sfr_map();
}


void P16F610::create_symbols()
{
    pic_processor::create_symbols();
    addSymbol(Wreg);
}


//-------------------------------------------------------------------
void P16F610::create_sfr_map()
{
    pir_set_def.set_pir1(&pir1);
    add_file_registers(0x40, 0x6f, 0);
    add_file_registers(0x70, 0x7f, 0);
    alias_file_registers(0x70, 0x7f, 0x80);
    add_sfr_register(indf, 0x00);
    alias_file_registers(0x00, 0x00, 0x80);
    add_sfr_register(&tmr0, 0x01);
    add_sfr_register(option_reg, 0x81, RegisterValue(0xff, 0));
    add_sfr_register(pcl, 0x02, RegisterValue(0, 0));
    add_sfr_register(status, 0x03, RegisterValue(0x18, 0));
    add_sfr_register(fsr, 0x04);
    alias_file_registers(0x02, 0x04, 0x80);
    add_sfr_register(&porta, 0x05);
    add_sfr_register(&trisa, 0x85, RegisterValue(0x3f, 0));
    add_sfr_register(&portc, 0x07);
    add_sfr_register(&trisc, 0x87, RegisterValue(0x3f, 0));
    add_sfr_register(pclath,  0x0a, RegisterValue(0, 0));
    add_sfr_register(&intcon_reg, 0x0b, RegisterValue(0, 0));
    alias_file_registers(0x0a, 0x0b, 0x80);

    add_sfr_register(&pir1, 0x0c, RegisterValue(0, 0));
    add_sfr_register(&pie1, 0x8c, RegisterValue(0, 0));
    pir1.valid_bits = PIR1v3::TMR1IF | PIR1v3::C1IF | PIR1v3::C2IF;
    pir1.writable_bits = pir1.valid_bits;
    pir1.set_intcon(&intcon_reg);
    pir1.set_pie(&pie1);
    pie1.setPir(&pir1);
    intcon = &intcon_reg;
    intcon_reg.set_pir_set(get_pir_set());
    int_pin.setIOpin(&porta[2], 0);

    add_sfr_register(&tmr1l, 0x0e, RegisterValue(0, 0));
    add_sfr_register(&tmr1h, 0x0f, RegisterValue(0, 0));
    add_sfr_register(&t1con, 0x10, RegisterValue(0, 0));
    tmr1l.tmrh = &tmr1h;
    tmr1l.t1con = &t1con;
    tmr1l.setInterruptSource(new InterruptSource(&pir1, PIR1v3::TMR1IF));
    tmr1h.tmrl = &tmr1l;
    t1con.tmrl = &tmr1l;
    tmr1l.setIOpin(&porta[5]);
    tmr1l.setGatepin(&porta[4]);

    comparator.assign_tmr1l(&tmr1l);
    comparator.cmxcon1[1]->set_vrcon(&vrcon);
    add_sfr_register(&vrcon, 0x19, RegisterValue(0, 0), "vrcon");
    add_sfr_register(comparator.cmxcon0[0], 0x1a, RegisterValue(0, 0), "cm1con0");
    add_sfr_register(comparator.cmxcon0[1], 0x1b, RegisterValue(0, 0), "cm2con0");
    add_sfr_register(comparator.cmxcon1[1], 0x1c, RegisterValue(2, 0), "cm2con1");

    add_sfr_register(&pcon, 0x8e, RegisterValue(0x03, 0));
    add_sfr_register(&osctune, 0x90, RegisterValue(0x00, 0), "osctune");
    add_sfr_register(&ansel, 0x91, RegisterValue(0xff, 0));
    add_sfr_register(&wpua, 0x95, RegisterValue(0x37, 0));
    add_sfr_register(&ioca, 0x96, RegisterValue(0, 0));

    sr_module.srcon0 = (SRCON0 *)new SRCON0_V2(this, "srcon0", "SR Latch Control 0 Register", &sr_module);
    sr_module.srcon1 = (SRCON1 *)new SRCON1_V2(this, "srcon1", "SR Latch Control 1 Register", &sr_module);
    add_sfr_register(sr_module.srcon0, 0x99, RegisterValue(0x00, 0));
    add_sfr_register(sr_module.srcon1, 0x9a, RegisterValue(0x00, 0));

    ansel.setAdcon1(&adcon1);
    ansel.setValidBits(0xff);
    adcon1.setNumberOfChannels(15);
    adcon1.setIOPin(0, &porta[0]);
    adcon1.setIOPin(1, &porta[1]);
    adcon1.setIOPin(2, &porta[2]);
    adcon1.setIOPin(3, &porta[4]);
    adcon1.setIOPin(4, &portc[0]);
    adcon1.setIOPin(5, &portc[1]);
    adcon1.setIOPin(6, &portc[2]);
    adcon1.setIOPin(7, &portc[3]);

    comparator.cmxcon1[1]->set_OUTpin(&porta[2], &portc[4]);
    comparator.cmxcon1[1]->set_INpinNeg(&(porta)[1], &portc[1],
                                        &portc[2], &portc[3]);
    comparator.cmxcon1[1]->set_INpinPos(&porta[0], &portc[0]);
    comparator.cmxcon1[1]->setBitMask(0x1f);
    comparator.cmxcon0[0]->setBitMask(0xb7);
    comparator.cmxcon0[0]->setIntSrc(new InterruptSource(&pir1, PIR1v3::C1IF));
    comparator.cmxcon0[1]->setBitMask(0xb7);
    comparator.cmxcon0[1]->setIntSrc(new InterruptSource(&pir1, PIR1v3::C2IF));
    comparator.assign_sr_module(&sr_module);
    sr_module.setPins(nullptr, &porta[2], &portc[4]);
}


//-------------------------------------------------------------------
void P16F610::option_new_bits_6_7(unsigned int bits)
{
    wpua.set_wpu_pu((bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
    porta.setIntEdge((bits & OPTION_REG::BIT6) == OPTION_REG::BIT6);
}


//-------------------------------------------------------------------
void P16F610::create_config_memory()
{
    m_configMemory = new ConfigMemory(this, 1);
    m_configMemory->addConfigWord(0, new ConfigF610(this));
}


//-------------------------------------------------------------------
bool P16F610::set_config_word(unsigned int address, unsigned int cfg_word)
{
    enum
    {
        CFG_FOSC0  = 1 << 0,
        CFG_FOSC1  = 1 << 1,
        CFG_FOSC2  = 1 << 2,
        CFG_WDTE   = 1 << 3,
        CFG_MCLRE  = 1 << 5,
        CFG_IOSCFS = 1 << 7,
    };

    if (address == config_word_address())
    {
        unsigned int valid_pins = porta.getEnableMask();

        if ((cfg_word & CFG_MCLRE) == CFG_MCLRE)
        {
            assignMCLRPin(4);

        }
        else
        {
            unassignMCLRPin();
        }

        wdt->initialize((cfg_word & CFG_WDTE) == CFG_WDTE);
        set_int_osc(false);
        // AnalogReq is used so ADC does not change clock names
        // set_config_word is first called with default and then
        // often called a second time. the following call is to
        // reset porta so next call to AnalogReq sill set the pin name
        //
        (&porta[4])->AnalogReq((Register *)this, false, "porta4");
        valid_pins |= 0x20;

        switch (cfg_word & (CFG_FOSC0 | CFG_FOSC1 | CFG_FOSC2))
        {
        case 0:  // LP oscillator: low power crystal is on RA4 and RA5
        case 1:     // XT oscillator: crystal/resonator is on RA4 and RA5
        case 2:     // HS oscillator: crystal/resonator is on RA4 and RA5
            (&porta[4])->AnalogReq((Register *)this, true, "OSC2");
            porta.getPin(5)->newGUIname("OSC1");
            valid_pins &= 0xcf;
            break;

        case 3:	// EC I/O on RA4 pin, CLKIN on RA5
            porta.getPin(5)->newGUIname("CLKIN");
            valid_pins &= 0xef;
            break;

        case 5: // INTOSC CLKOUT on RA4 pin
            (&porta[4])->AnalogReq((Register *)this, true, "CLKOUT");

        case 4: // INTOSC
            porta.getPin(5)->newGUIname("porta5");
            set_int_osc(true);
            osctune.set_freq((cfg_word & CFG_IOSCFS) ? 8e6 : 4e6);
            break;

        case 6: //RC oscillator: I/O on RA4 pin, RC on RA5
            porta.getPin(5)->newGUIname("RC");
            valid_pins &= 0xdf;
            break;

        case 7: // RC oscillator: CLKOUT on RA4 pin, RC on RA5
            (&porta[4])->AnalogReq((Register *)this, true, "CLKOUT");
            (porta.getPin(5))->newGUIname("RC");
            valid_pins &= 0xdf;
            break;
        };

        if (valid_pins != porta.getEnableMask())   // enable new pins for IO
        {
            porta.setEnableMask(valid_pins);
            trisa.setEnableMask(valid_pins);
        }

        return (true);
    }

    return false;
}


//------------------------------------------------------------------------
//
//

P16F616::P16F616(const char *_name, const char *desc)
    : P16F610(_name, desc),
      t2con(this, "t2con", "TMR2 Control"),
      pr2(this, "pr2", "TMR2 Period Register"),
      tmr2(this, "tmr2", "TMR2 Register"),
      eccp1as(this, "eccpas", "ECCP Auto-Shutdown Control Register"),
      pwm1con(this, "pwm1con", "Enhanced PWM 1 Control Register"),
      ccp1con(this, "ccp1con", "Enhanced Capture Compare Control"),
      ccpr1l(this, "ccpr1l", "Capture Compare 3 Low"),
      ccpr1h(this, "ccpr1h", "Capture Compare 3 High"),
      adcon0(this, "adcon0", "A2D Control 0"),
      adresh(this, "adresh", "A2D Result High"),
      adresl(this, "adresl", "A2D Result Low")
{
}


P16F616::~P16F616()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << '\n';
    }

    delete_file_registers(0x20, 0x3f);
    delete_file_registers(0xa0, 0xbf);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&eccp1as);
    remove_sfr_register(&ccpr1l);
    remove_sfr_register(&ccpr1h);
    remove_sfr_register(&ccp1con);
    remove_sfr_register(&adresl);
    remove_sfr_register(&adresh);
    remove_sfr_register(&adcon0);
    remove_sfr_register(&adcon1);
    remove_sfr_register(&tmr2);
    remove_sfr_register(&pr2);
    remove_sfr_register(&t2con);
}


Processor * P16F616::construct(const char *name)
{
    P16F616 *p = new P16F616(name);

    if (verbose)
    {
        std::cout << " P16F616 construct\n";
    }

    p->create();
    p->create_invalid_registers();
    p->create_symbols();
    return p;
}


void P16F616::create(void)
{
    P16F610::create();
    create_sfr_map();
}


void P16F616::create_sfr_map()
{

    pir1.valid_bits |= PIR1v3::TMR2IF | PIR1v3::CCP1IF | PIR1v3::ADIF;
    pir1.writable_bits = pir1.valid_bits;

    add_file_registers(0x20, 0x3f, 0);
    add_file_registers(0xa0, 0xbf, 0);

    // TODO: CCPR1L, CCPR1H, CCP1CON, PWM1CON, ECCPAS @ 0x13-0x17

    add_sfr_register(&tmr2,     0x11, RegisterValue(0, 0));
    add_sfr_register(&t2con,    0x12, RegisterValue(0, 0));
    add_sfr_register(&ccpr1l,   0x13, RegisterValue(0, 0));
    add_sfr_register(&ccpr1h,   0x14, RegisterValue(0, 0));
    add_sfr_register(&ccp1con,  0x15, RegisterValue(0, 0));

    add_sfr_register(&pwm1con, 0x16, RegisterValue(0, 0));
    add_sfr_register(&eccp1as, 0x17, RegisterValue(0, 0));
    add_sfr_register(&pr2, 0x92, RegisterValue(0xff, 0));
    t2con.tmr2  = &tmr2;
    tmr2.pir_set = get_pir_set();
    tmr2.pr2 = &pr2;
    tmr2.t2con = &t2con;
    tmr2.add_ccp(&ccp1con);
    pr2.tmr2 = &tmr2;
    ccp1con.setADCON(&adcon0);
    ccp1con.setIOpin(&portc[5], &portc[4], &portc[3], &portc[2]);
    eccp1as.link_registers(&pwm1con, &ccp1con);
    ccpr1l.ccprh  = &ccpr1h;
    ccpr1l.tmrl = &tmr1l;
    ccpr1h.ccprl = &ccpr1l;
    ccp1con.pwm1con = &pwm1con;


    add_sfr_register(&adresl, 0x9e, RegisterValue(0, 0));
    add_sfr_register(&adresh, 0x1e, RegisterValue(0, 0));
    add_sfr_register(&adcon0, 0x1f, RegisterValue(0, 0));
    add_sfr_register(&adcon1, 0x9f, RegisterValue(0, 0));
    ansel.setValidBits(0xff);
    //  ansel.setAdcon0(&adcon0);
    adcon0.setAdresLow(&adresl);
    adcon0.setAdres(&adresh);
    adcon0.setAdcon1(&adcon1);
    adcon0.setIntcon(&intcon_reg);
    adcon0.setA2DBits(10);
    adcon0.setPir(&pir1);
    adcon0.setChannel_Mask(0xf);
    adcon0.setChannel_shift(2);
    adcon0.setValidBits(0xff);
    adcon1.setAdcon0(&adcon0);
    /*
    adcon1.setIOPin(2, &porta[2]);
    adcon1.setIOPin(3, &porta[4]);
    */
    adcon1.attach_ad_fvr(((Processor *)this)->CVREF, 12);
    adcon1.setVoltRef(13, 0.6);
    adcon1.setVoltRef(14, 1.2);
    adcon1.setVrefHiConfiguration(2, 1);

    eccp1as.setBitMask(0xff);
    eccp1as.link_registers(&pwm1con, &ccp1con);
    eccp1as.setIOpin(0, 0, &porta[2]);
    ccp1con.setBitMask(0xff);
    ccp1con.setCrosslinks(&ccpr1l, &pir1, PIR1v3::CCP1IF, &tmr2, &eccp1as);

}
