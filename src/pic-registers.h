
/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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

#ifndef SRC_PIC_REGISTERS_H_
#define SRC_PIC_REGISTERS_H_

#include "gpsim_classes.h"
#include "registers.h"

class Processor;
class ProgramMemoryAccess;


//------------------------------------------------------------------------
//
// PCHelper
//
// The purpose of this class is to provide a register wrapper around the
// program counter. On the low and mid range pics, the program counter spans
// two registers. On the high end ones it spans 3. This class allows the
// gui to treat the program counter as though if it's a single register.

class PCHelper : public Register
{
public:
    PCHelper(Processor *pCpu, ProgramMemoryAccess *);

    void put_value(unsigned int new_value) override;
    unsigned int get_value() override;
    unsigned int register_size() const override
    {
        return 2;
    }

    ProgramMemoryAccess *pma;
};


//---------------------------------------------------------
// OPTION_REG -

class OPTION_REG : public sfr_register
{
public:
    OPTION_REG(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    inline unsigned int get_prescale() { return value.get() & (PS0 | PS1 | PS2); }

    inline unsigned int get_psa() { return value.get() & PSA; }

    inline unsigned int get_t0cs() { return value.get() & T0CS; }

    inline unsigned int get_t0se() { return value.get() & T0SE; }

    void put(unsigned int new_value) override;
    void reset(RESET_TYPE r) override;
    void initialize() override;

    enum
    {
        PS0    = 1 << 0,
        PS1    = 1 << 1,
        PS2    = 1 << 2,
        PSA    = 1 << 3,
        T0SE   = 1 << 4,
        T0CS   = 1 << 5,
        INTEDG = 1 << 6,
        BIT6   = 1 << 6,
        NOT_WPUEN   = 1 << 7,
        BIT7   = 1 << 7
    };

    unsigned int prescale = 0;
};


// For use on 14bit enhanced cores
class OPTION_REG_2 : public OPTION_REG
{
public:
    OPTION_REG_2(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    void put(unsigned int new_value) override;
    void initialize() override;
};


#endif
