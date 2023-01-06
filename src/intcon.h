/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian
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


#ifndef SRC_INTCON_H_
#define SRC_INTCON_H_

#include <vector>

#include "gpsim_classes.h"
#include "registers.h"
#include "pic-ioports.h"

class IOCxF;
class PicPortGRegister;
class Processor;
class PIR_SET;
class RCON;

//---------------------------------------------------------
// INTCON - Interrupt control register

class INTCON : public sfr_register
{
public:
    unsigned int interrupt_trace = 0;

    enum
    {
        RBIF = 1 << 0,
        INTF = 1 << 1,
        T0IF = 1 << 2,
        RBIE = 1 << 3,
        INTE = 1 << 4,
        T0IE = 1 << 5,
        XXIE = 1 << 6,  // Processor dependent
        PEIE = 1 << 6,
        GIE  = 1 << 7
    };

    INTCON(Processor *pCpu, const char *pName, const char *pDesc);
    virtual void set_gie() { put(value.get() | GIE); }

    virtual void clear_gie() { put(value.get() & ~GIE); }

    void set_T0IF();

    /*
    // Bit 6 of intcon depends on the processor that's being simulated,
    // This generic function will get called whenever interrupt flag upon
    // which bit 6 enables becomes true. (e.g. for the c84, this
    // routine is called when EEIF goes high.)
    */
    virtual void peripheral_interrupt(bool hi_pri = false);

    virtual void set_rbif(bool b);

    inline void set_intf(bool b)
    {
        bool current = (value.get() & INTF) == INTF;

        if (b && !current)
        {
            put(value.get() | INTF);
        }

        if (!b && current)
        {
            put(value.get() & ~INTF);
        }
    }

    inline void set_t0if() { put(value.get() | T0IF); }

    inline void set_rbie() { put(value.get() | RBIE); }

    inline void set_inte() { put(value.get() | INTE); }

    inline void set_t0ie() { put(value.get() | T0IE); }

    void set_portGReg(PicPortGRegister *_portGReg) { portGReg = _portGReg; }
    virtual int check_peripheral_interrupt() = 0;
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    virtual void aocxf_val(IOCxF *, unsigned int /* val */ ) {}

    PicPortGRegister *portGReg = nullptr;
};


//---------------------------------------------------------
class INTCON2 :  public sfr_register
{
public:
    INTCON2(Processor *pCpu, const char *pName, const char *pDesc);
    ~INTCON2();

    void put_value(unsigned int new_value) override;
    void put(unsigned int new_value) override;

    bool assignBitSink(unsigned int bitPosition, BitSink *) override;
    bool releaseBitSink(unsigned int bitPosition, BitSink *) override;

    enum
    {
        RBIP    = 1 << 0,
        INT3IP  = 1 << 1,
        TMR0IP  = 1 << 2,
        INTEDG3 = 1 << 3,
        INTEDG2 = 1 << 4,
        INTEDG1 = 1 << 5,
        INTEDG0 = 1 << 6,
        RBPU    = 1 << 7
    };

private:
    std::vector<BitSink *> bitsink_list;
};


class INTCON3 :  public sfr_register
{
public:
    INTCON3(Processor *pCpu, const char *pName, const char *pDesc);

    void put_value(unsigned int new_value) override;
    void put(unsigned int new_value) override;

    inline void set_int1f(bool b)
    {
        bool current = (value.get() & INT1IF) == INT1IF;

        if (b && !current)
        {
            put(value.get() | INT1IF);
        }

        if (!b && current)
        {
            put(value.get() & ~INT1IF);
        }
    }
    inline void set_int2f(bool b)
    {
        bool current = (value.get() & INT2IF) == INT2IF;

        if (b && !current)
        {
            put(value.get() | INT2IF);
        }

        if (!b && current)
        {
            put(value.get() & ~INT2IF);
        }
    }
    inline void set_int3f(bool b)
    {
        bool current = (value.get() & INT3IF) == INT3IF;

        if (b && !current)
        {
            put(value.get() | INT3IF);
        }

        if (!b && current)
        {
            put(value.get() & ~INT3IF);
        }
    }
    inline void set_int1e() { put(value.get() | INT1IE); }
    inline void set_int2e() { put(value.get() | INT2IE); }
    inline void set_int3e() { put(value.get() | INT3IE); }
    enum
    {
        INT1IF  = 1 << 0,
        INT2IF  = 1 << 1,
        INT3IF  = 1 << 2,
        INT1IE  = 1 << 3,
        INT2IE  = 1 << 4,
        INT3IE  = 1 << 5,
        INT1IP  = 1 << 6,
        INT2IP  = 1 << 7
    };
};


// A 14-bit intcon with pir registers
class INTCON_14_PIR : public INTCON
{
public:
    INTCON_14_PIR(Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    inline void set_pir_set(PIR_SET *p) { pir_set = p; }

    int check_peripheral_interrupt() override;
    void set_rbif(bool b) override;
    void set_gie() override { put_value(value.get() | GIE); }
    void clear_gie() override { put_value(value.get() & ~GIE); }
    void aocxf_val(IOCxF *, unsigned int val) override;
    void reset(RESET_TYPE r) override;

    enum
    {
        IOCIF = 1 << 0,
        INTF  = 1 << 1,
        T0IF  = 1 << 2,
        IOCIE = 1 << 3,
        INTE  = 1 << 4,
        T0IE  = 1 << 5,
        PEIE  = 1 << 6,
        GIE   = 1 << 7
    };

    //private:
    PIR_SET *pir_set = nullptr;
    unsigned int write_mask;    // Bits that instructions can modify
    struct aocxf
    {
        IOCxF *ptr_iocxf;
        unsigned int val;
    };
    std::vector<aocxf> aocxf_list;
};



//---------------------------------------------------------
// INTCON_16 - Interrupt control register for the 16-bit core

class INTCON_16 : public INTCON
{
public:
    INTCON_16(Processor *pCpu, const char *pName, const char *pDesc);

    enum
    {
        GIEH = GIE,
        GIEL = XXIE,
        TMR0IE = T0IE,
        INT0IE = INTE,
        TMR0IF = T0IF,
        INT0IF = INTF
    };
#define INTERRUPT_VECTOR_LO       (0x18 >> 1)
#define INTERRUPT_VECTOR_HI       (0x08 >> 1)

    inline void set_rcon(RCON *r) { rcon = r; }
    inline void set_intcon2(INTCON2 *ic) { intcon2 = ic; }
    inline void set_pir_set(PIR_SET *p) { pir_set = p; }

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

    void peripheral_interrupt(bool hi_pri = false) override;
    virtual void general_interrupt(bool hi_pri = false);

    void clear_gies();
    void set_gies();
    int check_peripheral_interrupt() override;
    unsigned int get_interrupt_vector()
    {
        return interrupt_vector;
    }
    bool isHighPriorityInterrupt()
    {
        return interrupt_vector == INTERRUPT_VECTOR_HI;
    }
    void set_interrupt_vector(unsigned int new_int_vect)
    {
        interrupt_vector = new_int_vect;
    }

private:
    unsigned int interrupt_vector = 0;        // Starting address of the interrupt
    RCON *rcon = nullptr;
    INTCON2 *intcon2 = nullptr;
    PIR_SET *pir_set = nullptr;
};


#endif /* INTCON_H */
