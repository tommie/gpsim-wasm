/*
   Copyright (C) 2001 Ralf Forsberg

This file is part of the libgpsim_modules library of gpsim

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


#ifndef MODULES_PARAFACE_H_
#define MODULES_PARAFACE_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/modules.h"

#include <gtk/gtk.h>

class Paraface;
class Paraface_Interface;  // defined in paraface.cc

// Create a few classes from which an PARAFACE may be constructed

// Paraface_Port is base for InputPort and OutputPort
class Paraface_Port : public IOPORT
{
public:
    Paraface_Port(unsigned int _num_iopins = 8);

    virtual void trace_register_write();
    virtual void setbit(unsigned int bit_number, bool new_value);

    Paraface *paraface;
};

class InputPort : public Paraface_Port, public TriggerObject
{
public:
    InputPort(unsigned int _num_iopins = 5);

    virtual void put(unsigned int new_value);
    virtual void callback();
    void update_pin_directions(unsigned int );

    unsigned int direction;
};

class OutputPort : public Paraface_Port
{
public:
    OutputPort(unsigned int _num_iopins = 8);

    virtual void put(unsigned int new_value);
};

// Create a class derived from the IOPIN class that
// will allow us to intercept when the I/O input is being
// driven. (This isn't done for PIC I/O pins because the
// logic for handling I/O pin changes resides in the IOPORT
// class.)

class Paraface_Input : public IOPIN
{
public:

  Paraface_Input(IOPORT *i, unsigned int b, char *opt_name = nullptr)
    : IOPIN(i,b,opt_name) { }
};

class Paraface : public Module
{
public:
    Paraface(const char *);
    ~Paraface();

    void test();
    void update();

    // Inheritances from the Package class
    virtual void create_iopin_map();

    // Inheritance from Module class
    virtual const char *type() { return ("paraface"); }
    static Module *construct(const char *new_name);

#ifdef linux
    int open_parallel_port(const char *device = "/dev/parport0");
#endif // linux
#ifdef __FreeBSD__
    int open_parallel_port(const char *device = "/dev/ppi0");
#endif // __FreeBSD__
#if !defined(linux) && !defined(__FreeBSD__)
    int open_parallel_port(const char *device = "/dev/bogus-device");
#endif
    int read_parallel_status();
    int write_parallel_data(int data);

    int fd; // file descriptor to parallel port

    unsigned char status, control, data; // last known parallel port state

    InputPort *input_port;
    OutputPort *output_port;

 private:
    Paraface_Interface *interface;
};

#endif //  MODULES_PARAFACE_H_
