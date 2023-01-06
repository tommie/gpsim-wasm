/*
   Copyright (C) 2004 Chris Emerson
   (Based on the push_button Copyright (C) 2002 Carlos Ghirardelli)

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
  encoder.cc
*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <cassert>
#include <iostream>

#include "../config.h"    // get the definition for HAVE_GUI
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <glib.h>
#include "../src/gpsim_time.h"
#include "../src/packages.h"
#include "../src/stimuli.h"
#include "encoder.h"


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Encoder::_create_iopin_map()
{
  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  create_pkg(2);
  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.
  a_pin = new IO_bi_directional("a");
  addSymbol(a_pin);
  assign_pin(1, a_pin);
  a_pin->update_direction(1, true);
  b_pin = new IO_bi_directional("b");
  b_pin->update_direction(1, true);
  addSymbol(b_pin);
  assign_pin(2, b_pin);
}


//--------------------------------------------------------------
// GUI
static void cw_cb(GtkButton * , Encoder *enc)
{
  enc->send_cw();
}


static void ccw_cb(GtkButton * , Encoder *enc)
{
  enc->send_ccw();
}


void Encoder::create_widget(Encoder *enc)
{
  GtkWidget *box1 = gtk_hbox_new(FALSE, 0);
  GtkWidget *buttonl = gtk_button_new_with_label("ccw");
  GtkWidget *buttonr = gtk_button_new_with_label(" cw");
  gtk_container_set_border_width(GTK_CONTAINER(buttonl), 5);
  gtk_container_set_border_width(GTK_CONTAINER(buttonr), 5);
  g_signal_connect(buttonl, "pressed",
                   G_CALLBACK(ccw_cb), (gpointer)enc);
  g_signal_connect(buttonr, "pressed",
                   G_CALLBACK(cw_cb), (gpointer)enc);
  gtk_widget_show(buttonl);
  gtk_widget_show(buttonr);
  gtk_box_pack_start(GTK_BOX(box1), buttonl, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), buttonr, FALSE, FALSE, 0);
  // Tell gpsim which widget to use in breadboard.
  enc->set_widget(box1);
}


//--------------------------------------------------------------
// construct
Module *Encoder::construct(const char *_new_name = nullptr)
{
  Encoder *enc_p = new Encoder(_new_name);
  enc_p->create_widget(enc_p);
  return enc_p;
}


Encoder::Encoder(const char * _name)
  : Module(_name, "Encoder"), rs(rot_detent)
{
  _create_iopin_map();
}


Encoder::~Encoder()
{
  /* done elsewhere RRR
  delete a_pin;
  delete b_pin;
  */
}


void Encoder::send_cw()
{
  switch (rs) {
  case rot_detent:
    rs = rot_moving_cw;
    toggle_a(); /* A toggles before B going clockwise */
    schedule_tick();
    break;

  default:
    // XXX: ought to do something here as well
    break;
  }
}


void Encoder::send_ccw()
{
  switch (rs) {
  case rot_detent:
    rs = rot_moving_ccw;
    toggle_b();
    schedule_tick();
    break;

  default:
    // XXX: ought to do something here as well
    break;
  }
}


void Encoder::toggle_a()
{
  a_pin->toggle();
  a_pin->update();
}


void Encoder::toggle_b()
{
  b_pin->toggle();
  b_pin->update();
}


void Encoder::schedule_tick()
{
  /* XXX: make the time delay configurable */
  if (!get_cycles().set_break_delta(100, this)) {
    std::cerr << "Encoder: error setting breakpoint.\n";
  }
}


void Encoder::callback()
{
  switch (rs) {
  case rot_detent:
    assert(false);
    break;

  case rot_moving_cw:
    toggle_b();
    rs = rot_detent;
    break;

  case rot_moving_ccw:
    toggle_a();
    rs = rot_detent;
    break;
  }
}

#endif // HAVE_GUI
