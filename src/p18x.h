/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2010,2015 Roy R Rankin


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

#ifndef SRC_P18X_H_
#define SRC_P18X_H_

#include <assert.h>
#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "16bit-processors.h"
#include "comparator.h"
#include "eeprom.h"
#include "pic-processor.h"
#include "pie.h"
#include "pir.h"
#include "psp.h"
#include "registers.h"
#include "spp.h"
#include "uart.h"

#define IESO (1<<12)

class Module;
class PicPortRegister;
class PicTrisRegister;
class PicLatchRegister;
class PicPSP_PortRegister;
class PicPSP_TrisRegister;
class Processor;

class P18C2x2 : public _16bit_compat_adc
{
public:
    P18C2x2(const char *_name = nullptr, const char *desc = nullptr);

    void create() override;

    PROCESSOR_TYPE isa() override { return _P18Cxx2_; }
    PROCESSOR_TYPE base_isa() override { return _PIC18_PROCESSOR_; }
    void create_symbols() override;

    unsigned int program_memory_size() const override { return 0x400; }
    unsigned int IdentMemorySize() const override { return 2;  }	// only two words on 18C

    void create_iopin_map() override;
};


class P18C242 : public P18C2x2
{
public:
    P18C242(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18C242_; }

    static Processor *construct(const char *name);
    void create() override;

    unsigned int program_memory_size() const override { return 0x2000; }
    unsigned int last_actual_register() const override { return 0x01FF; }
};


class P18C252 : public P18C242
{
public:
    P18C252(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18C252_; }

    static Processor *construct(const char *name);
    void create() override;

    unsigned int program_memory_size() const override { return 0x4000; }
    unsigned int last_actual_register() const override { return 0x05FF; }
};


/*********************************************************************
 *  class definitions for the 18C4x2 family
 */

//class P18C4x2 : public _16bit_processor
class P18C4x2 : public _16bit_compat_adc
{
public:
    P18C4x2(const char *_name = nullptr, const char *desc = nullptr);
    ~P18C4x2();

    PicPSP_PortRegister  *m_portd;
    PicTrisRegister  *m_trisd;
    PicLatchRegister *m_latd;

    PicPortRegister  *m_porte;
    PicPSP_TrisRegister  *m_trise;
    PicLatchRegister *m_late;

    PSP               psp;

    void create() override;

    PROCESSOR_TYPE isa() override { return _P18Cxx2_; }
    PROCESSOR_TYPE base_isa() override { return _PIC18_PROCESSOR_; }
    void create_symbols() override;

    unsigned int program_memory_size() const override { return 0x400; }
    unsigned int IdentMemorySize() const override { return 2;  }	// only two words on 18C 

    void create_sfr_map() override;
    void create_iopin_map() override;
};


class P18C442 : public P18C4x2
{
public:
    P18C442(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18C442_; }

    static Processor *construct(const char *name);
    void create() override;
    unsigned int program_memory_size() const override { return 0x2000; }
    virtual unsigned int eeprom_memory_size() const { return 256; }
    unsigned int last_actual_register() const override{ return 0x01FF; }
};


class P18C452 : public P18C442
{
public:
    P18C452(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18C452_; }

    static Processor *construct(const char *name);
    void create() override;
    unsigned int program_memory_size() const override { return 0x4000; }
    unsigned int last_actual_register() const override { return 0x05FF; }
};


class P18F242 : public P18C242
{
public:
    P18F242(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F242_; }

    static Processor *construct(const char *name);
    void create() override;
    unsigned int program_memory_size() const override { return 0x2000; }
    virtual unsigned int eeprom_memory_size() const { return 256; }
    unsigned int IdentMemorySize() const override { return 4; }

    void set_eeprom(EEPROM *) override
    {
        // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
        assert(0);
    }
    virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
    EEPROM_PIR *get_eeprom() override { return ((EEPROM_PIR *)eeprom); }
};


class P18F252 : public P18F242
{
public:
    P18F252(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F252_; }

    static Processor *construct(const char *name);
    void create() override;
    unsigned int program_memory_size() const override { return 0x4000; }
    unsigned int last_actual_register() const override { return 0x05FF; }
};


class P18F442 : public P18C442
{
public:
    P18F442(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F442_; }

    static Processor *construct(const char *name);
    void create() override;
    unsigned int program_memory_size() const override { return 0x2000; }
    unsigned int IdentMemorySize() const override { return 4; }

    void set_eeprom(EEPROM *) override
    {
        // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
        assert(0);
    }
    virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
    EEPROM_PIR *get_eeprom() override { return ((EEPROM_PIR *)eeprom); }
};


//
// The P18F248 is the same as the P18F242 except it has CAN, one fewer
// CCP module and a 5/10 ADC.  For now just assume it is identical.
class P18F248 : public P18F242
{
public:
    P18F248(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F248_; }

    static Processor *construct(const char *name);
    void create() override;
};


//
// The P18F258 is the same as the P18F252 except it has CAN, one fewer
// CCP module and a 5/10 ADC.  For now just assume it is identical.
class P18F258 : public P18F252
{
public:
    P18F258(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F258_; }

    static Processor *construct(const char *name);
    void create() override;
};


//
// The P18F448 is the same as the P18F442 except it has CAN, one fewer
// CCP module and a 5/10 ADC.  For now just assume it is identical.
class P18F448 : public P18F442
{
public:
    P18F448(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F448_; }

    static Processor *construct(const char *name);
    void create() override;
};


class P18F452 : public P18F442
{
public:
    P18F452(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F452_; }

    static Processor *construct(const char *name);
    void create() override;

    unsigned int program_memory_size() const override { return 0x4000; }
    unsigned int last_actual_register() const override { return 0x05FF; }
};


//
// The P18F458 is the same as the P18F452 except it has CAN and one
// fewer CCP module. For now just assume it is identical.
class P18F458 : public P18F452
{
public:
    P18F458(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F458_; }

    static Processor *construct(const char *name);
    void create() override;
};


class P18F1220 : public  _16bit_v2_adc
{
public:
    P18F1220(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F1220();

    OSCTUNE6      osctune;           // with 6-bit trim but no PLLEN
    ECCPAS        eccpas;
    PWM1CON       pwm1con;

    PROCESSOR_TYPE base_isa() override { return _PIC18_PROCESSOR_; };
    PROCESSOR_TYPE isa() override { return _P18F1220_; };

    static Processor *construct(const char *name);
    void create();
    void create_iopin_map() override;
    unsigned int program_memory_size() const override { return 0x1000; }
    virtual unsigned int eeprom_memory_size() const { return 256; }
    void osc_mode(unsigned int value) override;
    unsigned int last_actual_register() const override { return 0x00FF; }

    // Strip down from base class
    void create_base_ports() override;
    bool HasPortC() override { return false; };
    bool HasCCP2() override { return false; };

    void set_eeprom(EEPROM *) override
    {
        // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
        assert(0);
    }
    virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
    EEPROM_PIR *get_eeprom() override { return (EEPROM_PIR *)eeprom; }
    unsigned int get_device_id() override { return (0x07 << 8) | (0x7 << 5); }
};


class P18F1320 : public P18F1220
{
public:
    P18F1320(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F1320_; }

    static Processor *construct(const char *name);
    void create() override;

    unsigned int program_memory_size() const override { return 0x2000; }
    unsigned int get_device_id() override { return (0x07 << 8) | (0x6 << 5); }
};


class P18F2x21 : public _16bit_v2_adc
{
public:
    P18F2x21(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F2x21();

    PicPortRegister  *m_porte;
    PicPSP_TrisRegister  *m_trise;
    PicLatchRegister *m_late;

    ECCPAS        eccpas;
    PWM1CON       pwm1con;

    OSCTUNEPLL    osctune;           // with 6-bit trim and PLLEN
    ComparatorModule comparator;

    void create(bool has_usb = false);

    PROCESSOR_TYPE isa() override { return _P18Cxx2_; }
    PROCESSOR_TYPE base_isa() override { return _PIC18_PROCESSOR_; }
    void create_symbols() override;

    unsigned int program_memory_size() const override { return 0x400; }
    virtual unsigned int eeprom_memory_size() const { return 0x100; }

    // Setting the correct register memory size breaks things
    //  virtual unsigned int register_memory_size () const { return 0x200;};
    unsigned int last_actual_register() const override { return 0x01FF; }

    void create_iopin_map(bool has_usb=false);
    void create_sfr_map() override;


    void set_eeprom(EEPROM *) override
    {
        // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
        assert(0);
    }
    virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
    EEPROM_PIR *get_eeprom() override { return (EEPROM_PIR *)eeprom; }

    void osc_mode(unsigned int value) override;
};


class P18F2221 : public P18F2x21
{
public:
    P18F2221(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F2221_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x800; }
    unsigned int get_device_id() override { return (0x21 << 8) | (0x3 << 5); }
};


class P18F2321 : public P18F2x21
{
public:
    P18F2321(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F2321_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x1000; }
    unsigned int get_device_id() override { return (0x21 << 8) | (0x1 << 5); }
};


class P18F2420 : public P18F2x21
{
public:
    P18F2420(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F2420_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x2000; }
    unsigned int eeprom_memory_size() const override { return 256; }
    unsigned int last_actual_register() const override { return 0x02FF; }
    unsigned int get_device_id() override { return (0x0c << 8) | (0x6 << 5); }
};


class P18F2455 : public P18F2x21
{
public:
    P18F2455(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F2455();

    sfr_register ufrml, ufrmh, uir,  uie,  ueir,  ueie,  ustat,  ucon,
                 uaddr, ucfg,  uep0, uep1, uep2,  uep3,  uep4,   uep5,
                 uep6,  uep7,  uep8, uep9, uep10, uep11, uep12,  uep13,
                 uep14,  uep15;
    PROCESSOR_TYPE isa() override { return _P18F2455_; }

    static Processor *construct(const char *name);
    void create_sfr_map() override;

    unsigned int access_gprs() override
    {
        return 0x60;
    }  // USB peripheral moves access split
    unsigned int program_memory_size() const override { return 0x3000; }
    unsigned int last_actual_register() const override { return 0x07FF; }
    unsigned int get_device_id() override { return (0x12 << 8) | (0x3 << 5); }
};


class P18F2550 : public P18F2x21
{
public:
    P18F2550(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F2550();

    sfr_register ufrml, ufrmh, uir,  uie,  ueir,  ueie,  ustat,  ucon,
                 uaddr, ucfg,  uep0, uep1, uep2,  uep3,  uep4,   uep5,
                 uep6,  uep7,  uep8, uep9, uep10, uep11, uep12,  uep13,
                 uep14,  uep15;

    PROCESSOR_TYPE isa() override { return _P18F2550_; }

    static Processor *construct(const char *name);
    void create_sfr_map() override;

    unsigned int access_gprs() override
    {
        return 0x60;
    }  // USB peripheral moves access split
    unsigned int program_memory_size() const override { return 16384; }
    unsigned int last_actual_register() const override { return 0x07FF; }
    unsigned int get_device_id() override { return (0x12 << 8) | (0x2 << 5); }
};


class P18F2520 : public P18F2x21
{
public:
    P18F2520(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F2520_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x4000; }
    unsigned int last_actual_register() const override { return 0x05FF; }
    unsigned int get_device_id() override { return (0x0c << 8) | (0x4 << 5); }
};


class P18F2525 : public P18F2x21
{
public:
    P18F2525(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F2525_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 24576; }
    unsigned int last_actual_register() const override { return 0x05FF; }
    unsigned int get_device_id() override { return (0x0c << 8) | (0x6 << 5); }
};


class P18F2620 : public P18F2x21
{
public:
    P18F2620(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F2620_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x8000; }
    unsigned int last_actual_register() const override { return 0x0F7F; }
    unsigned int get_device_id() override { return (0x0c << 8) | (0x4 << 5); }
};


class RegZero : public Register
{
public:
    explicit RegZero(Module *_cpu, const char *_name = nullptr, const char *desc = nullptr)
        : Register(_cpu, _name, desc) {}

    void put(unsigned int /* new_value */ ) override
    {
        value.put(0);
    }
    void put_value(unsigned int /* new_value */ ) override
    {
        value.put(0);
    }
};


class P18F4x21 : public P18F2x21
{
public:
    P18F4x21(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F4x21();

    PicPSP_PortRegister  *m_portd;
    PicTrisRegister  *m_trisd;
    PicLatchRegister *m_latd;

    void create(bool has_usb=false);

    void create_symbols() override;
    void create_iopin_map(bool has_usb);
    void create_sfr_map() override;
};


class P18F4221 : public P18F4x21
{
public:
    P18F4221(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F4221_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x800; }
    unsigned int get_device_id() override { return (0x21 << 8) | (0x2 << 5); }
};


class P18F4321 : public P18F4x21
{
public:
    P18F4321(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override
    {
        return _P18F4321_;
    }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x1000; }
    unsigned int get_device_id() override { return (0x21 << 8) | (0x0 << 5); }
};


class P18F4420 : public P18F4x21
{
public:
    P18F4420(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F4420_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x2000; }
    unsigned int get_device_id() override { return (0x0c << 8) | (0x2 << 5); }
};


class P18F4455 : public P18F4x21
{
public:
    P18F4455(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F4455();

    sfr_register ufrml, ufrmh, uir,  uie,  ueir,  ueie,  ustat,  ucon,
                 uaddr, ucfg,  uep0, uep1, uep2,  uep3,  uep4,   uep5,
                 uep6,  uep7,  uep8, uep9, uep10, uep11, uep12,  uep13,
                 uep14,  uep15;

    SPP		spp;
    SPPCON 	sppcon;
    SPPCFG 	sppcfg;
    SPPEPS 	sppeps;
    SPPDATA 	sppdata;

    PROCESSOR_TYPE isa() override { return _P18F4455_; }

    static Processor *construct(const char *name);
    void create() override;

    unsigned int access_gprs() override
    {
        return 0x60;
    }  // USB peripheral moves access split
    unsigned int program_memory_size() const override { return 0x3000; }
    unsigned int last_actual_register() const override { return 0x07FF; }
    unsigned int get_device_id() override { return (0x12 << 8) | (0x1 << 5); }
};


class P18F4550 : public P18F4x21
{
public:
    P18F4550(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F4550();

    sfr_register ufrml, ufrmh, uir,  uie,  ueir,  ueie,  ustat,  ucon,
                 uaddr, ucfg,  uep0, uep1, uep2,  uep3,  uep4,   uep5,
                 uep6,  uep7,  uep8, uep9, uep10, uep11, uep12,  uep13,
                 uep14,  uep15;

    SPP		spp;
    SPPCON 	sppcon;
    SPPCFG 	sppcfg;
    SPPEPS 	sppeps;
    SPPDATA 	sppdata;

    PROCESSOR_TYPE isa() override { return _P18F4550_; }

    static Processor *construct(const char *name);
    void create() override;

    unsigned int access_gprs() override
    {
        return 0x60;
    }  // USB peripheral moves access split
    unsigned int program_memory_size() const override { return 16384; }
    unsigned int last_actual_register() const override { return 0x07FF; }
    unsigned int get_device_id() override { return (0x12 << 8) | (0x0 << 5); }
};


class P18F4520 : public P18F4x21
{
public:
    P18F4520(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F4520_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x4000; }
    unsigned int last_actual_register() const override { return 0x05FF; }
    unsigned int get_device_id() override { return (0x0c << 8) | (0x0 << 5); }
};


/***
PIC18F4620
Not implemented:
  OSCFIF bit in peripheral interrupt register 2 (PIR2v2 pir2)(And Enable Bit)

***/
class P18F4620 : public P18F4x21
{
public:
    P18F4620(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F4620_; }

    static Processor *construct(const char *name);

    unsigned int program_memory_size() const override { return 0x8000; }
    unsigned int eeprom_memory_size() const override { return 1024; }
    unsigned int last_actual_register() const override { return 0x0F7F; }
    unsigned int get_device_id() override { return (0x0c << 8) | (0x4 << 5); }
};


class P18F6x20 : public _16bit_v2_adc
{
public:
    P18F6x20(const char *_name = nullptr, const char *desc = nullptr);
    ~P18F6x20();

    PicPSP_PortRegister  *m_portd;
    PicTrisRegister  *m_trisd;
    PicLatchRegister *m_latd;

    PicPortRegister  *m_porte;
    PicTrisRegister  *m_trise;
    PicLatchRegister *m_late;

    PicPortRegister  *m_portf;
    PicTrisRegister  *m_trisf;
    PicLatchRegister *m_latf;

    PicPortRegister  *m_portg;
    PicTrisRegister  *m_trisg;
    PicLatchRegister *m_latg;

    PSP               psp;
    PSPCON	    *pspcon;

    //  ECCPAS        eccpas;
    //  PWM1CON       pwm1con;
    T2CON        t4con;
    PR2          pr4;
    TMR2         tmr4;
    PIR3v1       pir3;
    PIE          pie3;
    sfr_register ipr3;
    CCPCON       ccp3con;
    CCPRL        ccpr3l;
    CCPRH        ccpr3h;
    CCPCON       ccp4con;
    CCPRL        ccpr4l;
    CCPRH        ccpr4h;
    CCPCON       ccp5con;
    CCPRL        ccpr5l;
    CCPRH        ccpr5h;
    USART_MODULE         usart2;

    ComparatorModule comparator;

    void create();

    PROCESSOR_TYPE isa() override { return _P18Cxx2_; }
    PROCESSOR_TYPE base_isa() override { return _PIC18_PROCESSOR_; }
    unsigned int access_gprs() override { return 0x60; }
    void create_symbols() override;

    unsigned int program_memory_size() const override { return 0x4000; }
    virtual unsigned int eeprom_memory_size() const { return 1024; }

    // Setting the correct register memory size breaks things
    //  virtual unsigned int register_memory_size () const { return 0x800;};
    unsigned int last_actual_register() const override { return 0x07FF; }

    void create_iopin_map() override;
    void create_sfr_map() override;

    void set_eeprom(EEPROM *) override
    {
        // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
        assert(0);
    }
    virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
    EEPROM_PIR *get_eeprom() override { return ((EEPROM_PIR *)eeprom); }
};


class P18F6520 : public P18F6x20
{
public:
    P18F6520(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P18F6520_; }

    static Processor *construct(const char *name);

    //  virtual unsigned int program_memory_size() const { return 0x4000; };
    unsigned int bugs() override { return BUG_DAW; }
    unsigned int get_device_id() override { return (0x0b << 8) | (0x1 << 5); }
};


#endif
