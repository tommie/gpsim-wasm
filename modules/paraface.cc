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

/*

  paraface.cc

  This is an interface module that sits between gpsim and a parallel
  port. It implements eight outputs (out to parallel port) using
  the data pins, and five inputs (in from parallel port) using the
  status pins.

  To use it you create a .stc file that maps the signals to the
  right place. There is an example in gpsim/examples/module.

*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <time.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef linux
#include <linux/parport.h>
#include <linux/ppdev.h>
#endif // linux

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <dev/ppbus/ppi.h>
#include <dev/ppbus/ppbconf.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include <config.h>    // get the definition for HAVE_GUI


#ifdef HAVE_GUI

#include <gtk/gtk.h>

#include "../src/gpsim_interface.h"
#include "../src/trace.h"
#include "../src/gpsim_time.h"       // Cycle counter interface.

#include "paraface.h"

//--------------------------------------------------------------
//
// Create an "interface" to gpsim
//


class Paraface_Interface : public Interface
{
private:
    Paraface *paraface;

public:

    //virtual void UpdateObject (gpointer xref,int new_value);
    //virtual void RemoveObject (gpointer xref);
    virtual void SimulationHasStopped(void *object)
    {
        std::cout << "stopped\n";
        if (paraface)
        {
            paraface->update();
        }
    }

    //virtual void NewProcessor (unsigned int processor_id);
    //virtual void NewModule (Module *module);
    //virtual void NodeConfigurationChanged (Stimulus_Node *node);
    //virtual void NewProgram  (unsigned int processor_id);
    //virtual void GuiUpdate  (gpointer object);

    Paraface_Interface(Paraface *_paraface)
        : Interface((void **) _paraface)
    {
        paraface = _paraface;
    }
};

//--------------------------------------------------------------
// ParafacePort class
//
// The ParafacePort class is derived from the gpsim class "IOPORT".
// As such, it inherits all of the IOPROT's functionality (like
// tracing, stimulus interface, etc.). The ParafacePort class extends
// IOPORT by redirecting changes to the paraface.

Paraface_Port::Paraface_Port(unsigned int _num_iopins)
    : IOPORT(_num_iopins), paraface(nullptr)
{
}

void Paraface_Port::setbit(unsigned int bit_number, bool new_value)
{
    IOPORT::setbit(bit_number, new_value);
}

OutputPort::OutputPort(unsigned int _num_iopins)
    : Paraface_Port(_num_iopins)
{

}

void Paraface_Port::trace_register_write()
{
  //get_trace().module1(value.get());
    get_trace().raw(value.get());
}

//-----------------------------------------------------

InputPort::InputPort(unsigned int _num_iopins)
    : Paraface_Port(_num_iopins), direction(0)
{
}

void InputPort::put(unsigned int new_value)
{
    Paraface_Port::put(new_value);
}

// FIXME. Move to Paraface::callback
void InputPort::callback()
{
    static unsigned char parallel_input;

    //    cout << "InputPort::callback(void)\n";

    get_cycles().set_break_delta(1, this);


    if (paraface->output_port->value.get() != paraface->data)
    {
        paraface->write_parallel_data(paraface->output_port->value.get());
    }

    parallel_input = paraface->read_parallel_status();

    if (value.get() != parallel_input)
    {
        //        cout << "value = "<<value<<endl;
        put(parallel_input);

        for (int i = 0; i < 5; i++) {
            if (pins[i]) {

                // cout << "Update stimuli pin = "<<i<<endl;

                if (pins[i]->snode)
                    pins[i]->snode->update();
            }
        }
    }

    parallel_input++;
}

void InputPort::update_pin_directions(unsigned int new_direction)
{

    if ((new_direction ^ direction) & 1) {
        direction = new_direction & 1;

        // Change the directions of the I/O pins
        for (int i = 0; i < 5; i++) {
            if(pins[i]) {
                pins[i]->update_direction(direction, true);

                // cout << "Direction = "<<direction<<endl;

                if (pins[i]->snode)
                    pins[i]->snode->update();
            }
        }
    }
}

void OutputPort::put(unsigned int new_value)
{
  Paraface_Port::put(new_value);
}

//--------------------------------------------------------------
// Paraface_Input
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed.
/*
void Paraface_Input::put_node_state( int new_state)
{

    int current_state = state;


    IO_input::put_node_state(new_state);

    if(current_state ^ state) {

    }

}
*/

void Paraface::update()
{
    //    cout << "Update" <<endl;
    //  update(darea, w_width,w_height);
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Paraface::create_iopin_map()
{
    // Create an I/O port to which the I/O pins can interface
    //   The module I/O pins are treated in a similar manner to
    //   the pic I/O pins. Each pin has a unique pin number that
    //   describes it's position on the physical package. This
    //   pin can then be logically grouped with other pins to define
    //   an I/O port.


    input_port = new InputPort(5);
    input_port->value.put(0);
    input_port->paraface = this;

    output_port = new OutputPort(8);
    output_port->value.put(0);
    output_port->paraface = this;


    // Here, we name the port `pin'. So in gpsim, we will reference
    //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
    //   name of the logic gate (which is assigned by the user and
    //   obtained with the name() member function call).

    const char *pin_name = name().c_str();   // Get the name of this paraface

    output_port->new_name(pin_name);
    input_port->new_name(pin_name);

    // Define the physical package.
    //   The Package class, which is a parent of all of the modules,
    //   is responsible for allocating memory for the I/O pins.
    //

    create_pkg(8 + 5);


    // Define the I/O pins and assign them to the package.
    //   There are two things happening here. First, there is
    //   a new I/O pin that is being created. For the binary
    //   indicator, both pins are inputs. The second thing is
    //   that the pins are "assigned" to the package. If we
    //   need to reference these newly created I/O pins (like
    //   below) then we can call the member function 'get_pin'.

    assign_pin(1, new Paraface_Input(output_port, 0, "out0"));  // Data lines
    assign_pin(2, new Paraface_Input(output_port, 1, "out1"));
    assign_pin(3, new Paraface_Input(output_port, 2, "out2"));
    assign_pin(4, new Paraface_Input(output_port, 3, "out3"));
    assign_pin(5, new Paraface_Input(output_port, 4, "out4"));
    assign_pin(6, new Paraface_Input(output_port, 5, "out5"));
    assign_pin(7, new Paraface_Input(output_port, 6, "out6"));
    assign_pin(8, new Paraface_Input(output_port, 7, "out7"));

    assign_pin(9,  new IO_bi_directional(input_port, 0, "in0"));  // Status lines
    assign_pin(10, new IO_bi_directional(input_port, 1, "in1"));
    assign_pin(11, new IO_bi_directional(input_port, 2, "in2"));
    assign_pin(12, new IO_bi_directional(input_port, 3, "in3"));
    assign_pin(13, new IO_bi_directional(input_port, 4, "in4"));

    input_port->update_pin_directions(1);

    // Create an entry in the symbol table for the new I/O pins.
    // This is how the pins are accessed at the higher levels (like
    // in the CLI).

    for (int i = 1; i < get_pin_count(); i++) {
        IOPIN *p = get_pin(i);
        if (p)
            get_symbol_table().add_stimulus(p);
    }

    write_parallel_data(output_port->value.get());
    input_port->callback();
}

//--------------------------------------------------------------
// construct

Module * Paraface::construct(const char *_new_name = nullptr)
{
//    cout << " Parport constructor\n";

    Paraface *parafaceP = new Paraface(_new_name) ;
    parafaceP->open_parallel_port();
    parafaceP->new_name(_new_name);
    parafaceP->create_iopin_map();

    return parafaceP;
}

Paraface::Paraface(const char *_name)
    : Module(_name), fd(-1), status(0), control(0), data(0),
      input_port(nullptr), output_port(nullptr)
{
//    cout << "Paraface constructor\n";
  name_str = "Paraface";

  interface = new Paraface_Interface(this);
  get_interface().add_interface(interface);

  // interface_id = gpsim_register_interface((gpointer)this);

  //  gpsim_register_simulation_has_stopped(interface_id, simulation_has_stopped);
}

Paraface::~Paraface()
{
    //gpsim_unregister_interface(interface_id);
    //gpsim_clear_break(input_port); // Hmmmm. FIXME.
    delete input_port;
    delete output_port;
    if (fd != -1)
        close(fd);
}

int Paraface::open_parallel_port(const char *device)
{
    fd = open(device, O_RDWR);
    if (fd == -1) {
        perror("open");
#ifdef linux
        std::cout << "Could not open parallel port, parallel interface won't work.\n";
        std::cout << "Make sure your kernel has ppdev.\n";
#else
        std::cout << "Could not open parallel port, parallel interface won't work.\n";
#endif
        return -1;
    }

#ifdef linux
    int mode;

    if (ioctl(fd, PPCLAIM)) {
        perror("PPCLAIM");
        close(fd);
        fd = -1;
        std::cout << "Could not open parallel port, parallel interface won't work.\n";
        return -1;
    }

    /* Switch to compatibility mode.  (In fact we don't need
     * to do this, since we start off in compatibility mode
     * anyway, but this demonstrates PPNEGOT.)*/
    mode = IEEE1284_MODE_COMPAT;
    if (ioctl(fd, PPNEGOT, &mode)) {
        perror("PPNEGOT");
        close(fd);
        fd = -1;
        std::cout << "Could not open parallel port, parallel interface won't work.\n";
        return -1;
    }
#endif // linux

    std::cout << "Parallel port was successfully opened.\n";
    return 1;
}

// Return low five bits containing:
// bit0 - ACK (inverted compared to parallel port register)
// bit1 - BUSY
// bit2 - PAPEREND
// bit3 - SELECTIN
// bit4 - ERROR (inverted compared to parallel port register)
// Bit levels are inverted when needed, so they match parallel input states
int Paraface::read_parallel_status()
{
#ifdef linux
    unsigned int ppstatus;
#endif // linux
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    u_int8_t ppstatus;
#endif // __FreeBSD__

    if (fd == -1)
        return -1;

#ifdef linux
    if (ioctl(fd, PPRSTATUS, &ppstatus) == -1)
        perror("ioctl");

    if (ppstatus & PARPORT_STATUS_ACK)
        status |= 0x01;
    else
        status &= ~0x01;

    if (ppstatus & PARPORT_STATUS_BUSY)
        status &= ~0x02;
    else
        status |= 0x02;

    if (ppstatus & PARPORT_STATUS_PAPEROUT)
        status |= 0x04;
    else
        status &= ~0x04;

    if (ppstatus & PARPORT_STATUS_SELECT)
        status |= 0x08;
    else
        status &= ~0x08;

    if (ppstatus & PARPORT_STATUS_ERROR)
        status |= 0x10;
    else
        status &= ~0x10;

#endif // linux

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    if (ioctl(fd, PPIGSTATUS, &ppstatus) == -1) {
      perror("ioctl");
    }

    if (ppstatus & nACK)
        status |= 0x01;
    else
        status &= ~0x01;

    if (ppstatus & nBUSY)
        status &= ~0x02;
    else
        status |= 0x02;

    if (ppstatus & PERROR)
        status |= 0x04;
    else
        status &= ~0x04;

    if (ppstatus & SELECT)
        status |= 0x08;
    else
        status &= ~0x08;

    if (ppstatus & nFAULT)
        status |= 0x10;
    else
        status &= ~0x10;

#endif // __FreeBSD__

    //    cout << "status" << (int)status << endl;

    return status;
}

int Paraface::write_parallel_data(int newdata)
{
    struct timespec ts;

    if (fd == -1)
        return -11;

    /* Set the data lines */
    data = newdata;
#ifdef linux
    if (ioctl(fd, PPWDATA, &data) == -1)
        perror("ioctl");
#endif // linux

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    if (ioctl(fd, PPISDATA, &data) == -1)
        perror("ioctl");
#endif // __FreeBSD__

    //    cout << "data" << (int)data << endl;

    /* Delay for a bit */
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;
//    nanosleep (&ts, NULL);
    return 0;
}

#endif //  HAVE_GUI
