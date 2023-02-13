/*
   Copyright (C) 2000 T. Scott Dattalo

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
  Logic.cc

This is an example module library for interfacing with gpsim.

In here you'll find some simple logic devices:

  AND2Gate - A 2-input AND gate
  OR2Gate - A 2-input OR gate

*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <config.h>    // get the definition for HAVE_GUI
#include <iostream>

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <cairo.h>

/* XPM */
static const char * and2_pixmap[] = {
  "32 32 3 1",
  "       c black",
  ".      c None",
  "X      c white",
  "                    ............",
  "                        ........",
  "  XXXXXXXXXXXXXXXXXX    ........",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  "  XXXXXXXXXXXXXXXXXXXXXXXX  ....",
  "  XXXXXXXXXXXXXXXXXXXXXXXX   ...",
  "  XXXXXXXXXXXXXXXXXXXXXXXXX  ...",
  "  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "  XXXXXX XXX XXX X    XXXXXXX  .",
  "  XXXXX X XX  XX XX XX XXXXXX  .",
  "  XXXX XXX X  XX XX XX XXXXXXX  ",
  "  XXXX XXX X X X XX XX XXXXXXX  ",
  "  XXXX XXX X X X XX XX XXXXXXX  ",
  "  XXXX     X XX  XX XX XXXXXXX  ",
  "  XXXX XXX X XX  XX XX XXXXXXX  ",
  "  XXXX XXX X XXX XX XX XXXXXXX  ",
  "  XXXX XXX X XXX X    XXXXXXX  .",
  "  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "  XXXXXXXXXXXXXXXXXXXXXXXXX  ...",
  "  XXXXXXXXXXXXXXXXXXXXXXXX   ...",
  "  XXXXXXXXXXXXXXXXXXXXXXXX  ....",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  "  XXXXXXXXXXXXXXXXXX    ........",
  "                        ........",
  "                    ............"
};

/* XPM */
static const char * or2_pixmap[] = {
  "32 32 3 1",
  "       c black",
  ".      c None",
  "X      c white",
  "                    ............",
  "                        ........",
  "  XXXXXXXXXXXXXXXXXX    ........",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  ".  XXXXXXXXXXXXXXXXXXXXXXX  ....",
  ".  XXXXXXXXXXXXXXXXXXXXXXX   ...",
  ".  XXXXXXXXXXXXXXXXXXXXXXXX  ...",
  "..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "..  XXXXXXX   XX    XXXXXXXXX  .",
  "...  XXXXX XXX X XXX XXXXXXXX  .",
  "...  XXXXX XXX X XXX XXXXXXXXX  ",
  "...  XXXXX XXX X XXX XXXXXXXXX  ",
  "...  XXXXX XXX X    XXXXXXXXXX  ",
  "...  XXXXX XXX X X XXXXXXXXXXX  ",
  "...  XXXXX XXX X XX XXXXXXXXXX  ",
  "...  XXXXX XXX X XXX XXXXXXXXX  ",
  "...  XXXXXX   XX XXX XXXXXXXX  .",
  "..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
  "..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
  ".  XXXXXXXXXXXXXXXXXXXXXXXX  ...",
  ".  XXXXXXXXXXXXXXXXXXXXXXX   ...",
  ".  XXXXXXXXXXXXXXXXXXXXXXX  ....",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  "  XXXXXXXXXXXXXXXXXXXXXX  ......",
  "  XXXXXXXXXXXXXXXXXX    ........",
  "                        ........",
  "                    ............"
};

static const char * xor2_pixmap[] = {
  "40 32 3 1",
  "       c None",
  ".      c black",
  "X      c white",
  "        ....................            ",
  "  ..    ....................            ",
  " ....   ........................        ",
  "  ...   ..XXXXXXXXXXXXXXXXXX....        ",
  "  ...   ..XXXXXXXXXXXXXXXXXXXXXX..      ",
  "   ...  ..XXXXXXXXXXXXXXXXXXXXXX..      ",
  "   ....  ..XXXXXXXXXXXXXXXXXXXXXXX..    ",
  "    ...  ..XXXXXXXXXXXXXXXXXXXXXXX...   ",
  "    ...  ..XXXXXXXXXXXXXXXXXXXXXXXX..   ",
  "    ...   ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
  "    ....  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
  "     ...  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
  "     ...  ..XXXX.XXX.XX...XX....XXXXX.. ",
  "      ..   ..XXX.XXX.X.XXX.X.XXX.XXXX.. ",
  "      ..   ..XXXX.X.XX.XXX.X.XXX.XXXXX..",
  "      ..   ..XXXX.X.XX.XXX.X.XXX.XXXXX..",
  "      ...  ..XXXXX.XXX.XXX.X....XXXXXX..",
  "      ...  ..XXXX.X.XX.XXX.X.X.XXXXXXX..",
  "      ...  ..XXXX.X.XX.XXX.X.XX.XXXXXX..",
  "      ...  ..XXX.XXX.X.XXX.X.XXX.XXXXX..",
  "      ...  ..XXX.XXX.XX...XX.XXX.XXXX.. ",
  "      ... ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
  "      ..  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
  "     ...  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
  "     ...  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
  "    ...  ..XXXXXXXXXXXXXXXXXXXXXXXX..   ",
  "    ...  ..XXXXXXXXXXXXXXXXXXXXXXX...   ",
  "    ...  ..XXXXXXXXXXXXXXXXXXXXXXX..    ",
  "   .... ..XXXXXXXXXXXXXXXXXXXXXX..      ",
  "  ..... ..XXXXXXXXXXXXXXXXXXXXXX..      ",
  " ....   ..XXXXXXXXXXXXXXXXXX....        ",
  "  ..    ........................        "
};

static const char * not_pixmap[] = {
  "32 32 3 1",
  "       c black",
  ".      c None",
  "X      c white",
  " ...............................",
  "   .............................",
  "     ...........................",
  "  X   ..........................",
  "  XXX   ........................",
  "  XXXX    ......................",
  "  XXXXXX    ....................",
  "  XXXXXXXX   ...................",
  "  XXXXXXXXXX   .................",
  "  XXXXXXXXXXX    ...............",
  "  XXXXXXXXXXXXX   ..............",
  "  XXXXXXXXXXXXXXX   ............",
  "  XXXXXXXXXXXXXXXX    .....   ..",
  "  XXXXXXXXXXXXXXXXXX    .. XXX .",
  "  XXXXXXXXXXXXXXXXXXXX    XXXXX ",
  "  XXXXXXXXXXXXXXXXXXXXXX  XXXXX ",
  "  XXXXXXXXXXXXXXXXXXXX    XXXXX ",
  "  XXXXXXXXXXXXXXXXXXX   .. XXX .",
  "  XXXXXXXXXXXXXXXXX   .....   ..",
  "  XXXXXXXXXXXXXXX    ...........",
  "  XXXXXXXXXXXXXX   .............",
  "  XXXXXXXXXXXX   ...............",
  "  XXXXXXXXXXX   ................",
  "  XXXXXXXXX   ..................",
  "  XXXXXXX    ...................",
  "  XXXXXX   .....................",
  "  XXXX   .......................",
  "  XXX   ........................",
  "  X   ..........................",
  "     ...........................",
  "   .............................",
  " ..............................."
};
#endif

#include <stdio.h>
#include <string>

#include "logic.h"
#include "../src/packages.h"
#include "../src/gpsim_interface.h"
#include "../src/modules.h"
#include "../src/stimuli.h"
#include "../src/ui.h"

//--------------------------------------------------------------
// Led_Input
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed.

void Logic_Input::setDrivenState(bool new_state)
{
  if (verbose)
    std::cout << name() << " setDrivenState= "
              << (new_state ? "high\n" : "low\n");

  if (new_state != getDrivenState()) {
    bDrivingState = new_state;
    bDrivenState  = new_state;

    if (LGParent) {
      LGParent->update_input_pin(m_iobit, new_state);
      LGParent->update_state();
    }
  }
}


#ifdef RRR


void Logic_Input::get(char *return_str, int len)
{
  if (return_str) {
    strncpy(return_str, IOPIN::getState() ? "1" : "0", len);
  }
}


void Logic_Output::get(char *return_str, int len)
{
  if (return_str) {
    strncpy(return_str, IOPIN::getDrivingState() ? "1" : "0", len);
  }
}


#endif


ANDGate::ANDGate(const char *name, const char *desc)
  : LogicGate(name, desc)
{
}


ANDGate::~ANDGate()
{
}


ORGate::ORGate(const char *name, const char *desc)
  : LogicGate(name, desc)
{
}


ORGate::~ORGate()
{
}


XORGate::XORGate(const char *name, const char *desc)
  : LogicGate(name, desc)
{
}


XORGate::~XORGate()
{
}


/*************************************************************
*
*  LogicGate class
*/

LogicGate::LogicGate(const char *name, const char *desc)
  : Module(name, desc),
    number_of_pins(0),
    input_bit_mask(0),
    input_state(0),
    pInputPins(nullptr),
    pOutputPin(nullptr)
{
#ifdef HAVE_GUI
  pixbuf = nullptr;
#endif
}


LogicGate::~LogicGate()
{
#ifdef HAVE_GUI

  if (pixbuf) {
    g_object_unref(pixbuf);
  }

#endif
}


//--------------------------------------------------------------
void LogicGate::update_input_pin(unsigned int pin, bool bValue)
{
  unsigned int mask = 1 << pin;
  input_state &= ~mask;
  input_state |= bValue ? mask : 0;
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void LogicGate::create_iopin_map()
{
  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  create_pkg(number_of_pins);

  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.
  // all logic gates have one or more inputs, but only one
  // output. The output is arbitrarily assigned to position
  // 0 on the I/O port while the inputs go to positions 1 and above
#define OUTPUT_BITPOSITION 0
#define INPUT_FIRST_BITPOSITION (OUTPUT_BITPOSITION + 1)

  // Here, we name the port `pin'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the logic gate (which is assigned by the user and
  //   obtained with the name() member function call).

  //std::string outname = name() + ".out";
  //RRRpOutputPin = new Logic_Output(this, OUTPUT_BITPOSITION, outname.c_str());
  pOutputPin = new Logic_Output(this, OUTPUT_BITPOSITION, "out");
  addSymbol(pOutputPin);
  pOutputPin->update_direction(1, true); // make the bidirectional an output
  assign_pin(OUTPUT_BITPOSITION + 1, pOutputPin);
  int i, j;

  pInputPins = new IOPIN *[number_of_pins - 1];
  char inname[14];

  for (i = j = INPUT_FIRST_BITPOSITION; i < number_of_pins; i++) {
    int pin_number = i - j;
    snprintf(inname, sizeof(inname), "in%d", pin_number);

    auto LIP = new Logic_Input(this, i - INPUT_FIRST_BITPOSITION, inname);
    pInputPins[i - INPUT_FIRST_BITPOSITION] = LIP;

    addSymbol(LIP);
    assign_pin(i + 1, LIP);      //  Pin numbers begin at 1
  }

  // Form the logic gate bit masks
  input_bit_mask = (1 << (number_of_pins - 1)) - 1;
  //initializeAttributes();
}


#ifdef HAVE_GUI


static gboolean expose(GtkWidget *widget, GdkEventExpose * , LogicGate *lg)
{
  if (!lg->pixbuf) {
    puts("LogicGate has no pixmap");
    return 0;
  }

  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  gdk_cairo_set_source_pixbuf(cr, lg->pixbuf, 0.0, 0.0);
  cairo_paint(cr);
  cairo_destroy(cr);
  return 0;
}


GtkWidget *LogicGate::create_pixmap(const char **pixmap_data)
{
  pixbuf = gdk_pixbuf_new_from_xpm_data(pixmap_data);
  int width = gdk_pixbuf_get_width(pixbuf);
  int height = gdk_pixbuf_get_height(pixbuf);
  GtkWidget *da = gtk_drawing_area_new();
  gtk_widget_set_size_request(da, width, height);
  g_signal_connect(da, "expose_event", G_CALLBACK(expose), this);
  return da;
}


#endif

//--------------------------------------------------------------
// construct

Module * AND2Gate::construct(const char *_new_name)
{
  AND2Gate *a2gP = new AND2Gate(_new_name);
  a2gP->set_number_of_pins(3);
  a2gP->create_iopin_map();
  return a2gP;
}


AND2Gate::AND2Gate(const char *name)
  : ANDGate(name, "And2Gate")
{
#ifdef HAVE_GUI

  if (get_interface().bUsingGUI()) {
    set_widget(create_pixmap(and2_pixmap));
  }

#endif
}


AND2Gate::~AND2Gate()
{
}


void ANDGate::update_state()
{
  pOutputPin->putState((input_state & input_bit_mask) == input_bit_mask);

  if (pOutputPin->snode) {
    pOutputPin->snode->update();
  }
}


//--------------------------------------------------------------
// construct


OR2Gate::OR2Gate(const char *name)
  : ORGate(name, "OR2Gate")
{
#ifdef HAVE_GUI

  if (get_interface().bUsingGUI()) {
    set_widget(create_pixmap(or2_pixmap));
  }

#endif
}


OR2Gate::~OR2Gate()
{
}


Module * OR2Gate::construct(const char *_new_name)
{
  OR2Gate *o2gP = new OR2Gate(_new_name);
  o2gP->set_number_of_pins(3);
  o2gP->create_iopin_map();
  return o2gP;
}


void ORGate::update_state()
{
  pOutputPin->putState((input_state & input_bit_mask) != 0);
}


//--------------------------------------------------------------
// construct NOT
Module * NOTGate::construct(const char *_new_name)
{
  NOTGate *a2gP = new NOTGate(_new_name);
  a2gP->set_number_of_pins(2);
  a2gP->create_iopin_map();
  a2gP->update_state();
  return a2gP;
}


NOTGate::NOTGate(const char *name)
  : LogicGate(name, "NOTGate")
{
#ifdef HAVE_GUI

  if (get_interface().bUsingGUI()) {
    set_widget(create_pixmap(not_pixmap));
  }

#endif
}


NOTGate::~NOTGate()
{
}


void NOTGate::update_state()
{
  if (verbose) {
    std::cout << name() << " update_state\n";
  }

  pOutputPin->putState((input_state & input_bit_mask) == 0);
}


//--------------------------------------------------------------
// construct


XOR2Gate::XOR2Gate(const char *name)
  : XORGate(name, "XOR2Gate")
{
#ifdef HAVE_GUI

  if (get_interface().bUsingGUI()) {
    set_widget(create_pixmap(xor2_pixmap));
  }

#endif
}


XOR2Gate::~XOR2Gate()
{
}


Module * XOR2Gate::construct(const char *_new_name)
{
  XOR2Gate *o2gP = new XOR2Gate(_new_name);
  o2gP->set_number_of_pins(3);
  o2gP->create_iopin_map();
  return o2gP;
}


void XORGate::update_state()
{
  bool bNewOutputState = false;
  unsigned int mask = input_bit_mask;

  while (mask) {
    unsigned int lsb = (~mask + 1) & mask;
    mask ^= lsb;
    bNewOutputState ^= (lsb & input_state) ? true : false;
  }

  pOutputPin->putState(bNewOutputState);
}
