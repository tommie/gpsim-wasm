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

#ifndef SRC_P16X8X_H_
#define SRC_P16X8X_H_

#include "14bit-processors.h"
#include "pic-processor.h"
class Processor;

class P16X8X : public Pic14Bit
{
public:
    P16X8X(const char *_name = nullptr, const char *desc = nullptr);
    ~P16X8X();

    void create_sfr_map() override;
    void set_out_of_range_pm(unsigned int address, unsigned int value) override;
    virtual void create_iopin_map();
    virtual void create(int ram_top);
    unsigned int register_memory_size() const override { return 0x100; }

protected:
    unsigned int ram_top;
};


class P16C84 : public P16X8X
{
public:
    P16C84(const char *_name = nullptr, const char *desc = nullptr);

    PROCESSOR_TYPE isa() override { return _P16C84_; }
    void create(int ram_top) override;

    unsigned int program_memory_size() const override{ return 0x400; }
    static Processor *construct(const char *name);
};


class P16F84 : public P16X8X
{
public:
    P16F84(const char *_name = nullptr, const char *desc = nullptr);

    static Processor *construct(const char *name);

    PROCESSOR_TYPE isa() override { return _P16F84_; }

    void create(int ram_top) override;
    unsigned int program_memory_size() const override { return 0x400; }
};


class P16CR84 : public P16F84
{
public:
    P16CR84(const char *_name = nullptr, const char *desc = nullptr);

    static Processor *construct(const char *name);

    PROCESSOR_TYPE isa() override { return _P16CR84_; }
};


class P16F83 : public P16X8X
{
public:
    P16F83(const char *_name = nullptr, const char *desc = nullptr);

    static Processor *construct(const char *name);

    PROCESSOR_TYPE isa() override { return _P16F83_; }

    unsigned int program_memory_size() const override { return 0x200; }
    void create(int ram_top) override;
};


class P16CR83 : public P16F83
{
public:
    P16CR83(const char *_name = nullptr, const char *desc = nullptr);

    static Processor *construct(const char *name);

    PROCESSOR_TYPE isa() override { return _P16CR83_; };
};


#endif
