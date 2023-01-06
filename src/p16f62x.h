/*
   Copyright (C) 1998-2002 T. Scott Dattalo

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

#ifndef SRC_P16F62X_H_
#define SRC_P16F62X_H_

#include <assert.h>

#include "comparator.h"
#include "eeprom.h"
#include "p16x6x.h"
#include "pic-processor.h"
#include "uart.h"

class Processor;

/***************************************************************************
 *
 * Include file for:  P16F627, P16F628, P16F648
 *
 *
 * The F62x devices are quite a bit different from the other PICs. The class
 * heirarchy is similar to the 16F84.
 *
 *
 ***************************************************************************/

class P16F62x : public P16X6X_processor
{
public:
    P16F62x(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F62x();

    USART_MODULE usart;
    ComparatorModule comparator;

    virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

    virtual PROCESSOR_TYPE isa() { return _P16F627_; }
    virtual void create_symbols();
    virtual unsigned int register_memory_size() const { return 0x200; }

    virtual unsigned int program_memory_size() { return 0; }

    virtual void create_sfr_map();

    // The f628 (at least) I/O pins depend on the Fosc Configuration bits.
    virtual bool set_config_word(unsigned int address, unsigned int cfg_word);

    virtual void create(int ram_top, unsigned int eeprom_size);
    virtual void create_iopin_map();

    virtual void set_eeprom(EEPROM *)
    {
        // Use set_eeprom_pir as P16F62x expects to have a PIR capable EEPROM
        assert(0);
    }
    virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
    virtual EEPROM_PIR *get_eeprom() { return (EEPROM_PIR *)eeprom; }
};


class P16F627 : public P16F62x
{
public:
    P16F627(const char *_name = nullptr, const char *desc = nullptr);

    virtual PROCESSOR_TYPE isa() { return _P16F627_; }

    virtual unsigned int program_memory_size() const { return 0x400; }

    static Processor *construct(const char *name);
};


class P16F628 : public P16F627
{
public:
    P16F628(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F628();

    virtual PROCESSOR_TYPE isa() { return _P16F628_; }

    virtual unsigned int program_memory_size() const { return 0x800; }

    static Processor *construct(const char *name);
};


class P16F648 : public P16F628
{
public:
    P16F648(const char *_name = nullptr, const char *desc = nullptr);
    ~P16F648();

    virtual PROCESSOR_TYPE isa() { return _P16F648_; }

    virtual unsigned int program_memory_size() const { return 0x1000; }
    virtual void create_sfr_map();

    static Processor *construct(const char *name);
};


#endif
