/*
   Copyright (C) 2003 Ralf Forsberg

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

A PAL video module.

It makes use of two digital inputs to generate PAL signal

sync	lume	result
0	0	sync
0	1	not valid
1	0	black
1	1	white

*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <stdio.h>
#include <cstring>
#include <cassert>

#include <config.h>    // get the definition for HAVE_GUI
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <glib.h>

#include "../src/gpsim_interface.h"
#include "../src/gpsim_time.h"
#include "../src/processor.h"

#include "video.h"

//--------------------------------------------------------------
//
// Create an "interface" to gpsim
//


class Video_Interface : public Interface {
private:
  Video *video;

public:
  explicit Video_Interface(Video *_video)
    : Interface((void *) _video)
  {
    video = _video;
  }

  //virtual void UpdateObject (gpointer xref,int new_value);
  //virtual void RemoveObject (gpointer xref);
  void SimulationHasStopped(gpointer /* object */ ) override
  {
    if (video) {
      video->refresh();
    }
  }

  void NewProcessor(Processor *new_cpu) override
  {
    if (video) {
      video->cpu = new_cpu;
    }
  }

  //virtual void NewModule (Module *module);
  //virtual void NodeConfigurationChanged (Stimulus_Node *node);
  //virtual void NewProgram  (unsigned int processor_id);
  virtual void GuiUpdate(gpointer /* object */ )
  {
    if (video) {
      video->refresh();
    }
  }
};


//--------------------------------------------------------------
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed.
void IOPIN_Monitor::setDrivenState(bool new_state)
{
  bool current_state = getDrivenState();
  IOPIN::setDrivenState(new_state);

  if (current_state != getDrivenState()) {
    if (video) {
      video->update_state();
    }
  }
}


static void expose(GtkWidget *widget, GdkEventExpose * , Video *video)
{
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_set_source_surface(cr, video->image, 0.0, 0.0);
  cairo_paint(cr);
  cairo_destroy(cr);
}


/*************************************************************
*
*  Video class
*/

Video::Video(const char *_name)
  : Module(_name), sync_time(0), scanline(0),
    line_nr(0), last_line_nr(0)
{
  sync_pin = new IOPIN_Monitor(this, "sync");
  lume_pin = new IOPIN_Monitor(this, "lume");
  addSymbol(sync_pin);
  addSymbol(lume_pin);

  memset(line, 0x80, XRES);
  memset(shadow, 0x42, XRES * YRES); // initalize shadow to invalid values
  //  cpu = get_processor(1);
  cpu = get_active_cpu();  //FIXME

  get_interface().add_interface(new Video_Interface(this));

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), XRES, YRES);
  gtk_window_set_title(GTK_WINDOW(window), "Video");
  da = gtk_drawing_area_new();
  g_signal_connect(da, "expose_event", G_CALLBACK(expose), this);
  gtk_container_add(GTK_CONTAINER(window), da);
  gtk_widget_show_all(window);
  image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, XRES, YRES);
}


Video::~Video()
{
  cairo_surface_destroy(image);
  gtk_widget_destroy(window);
  removeSymbol(sync_pin);
  removeSymbol(lume_pin);
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Video::create_iopin_map()
{
  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port.
  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  create_pkg(2);
  // Define the I/O pins (already done) and assign them to the package. //FIX comment
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.
  assign_pin(1, sync_pin);
  //package->set_pin_position(1,(float)0.0);
  assign_pin(2, lume_pin);
  //package->set_pin_position(2,(float)0.9999);
}


//--------------------------------------------------------------
// construct

Module *Video::construct(const char *_new_name)
{
  Video *video = new Video(_new_name) ;
  video->create_iopin_map();

  return video;
}


static int screen_y_from_line(int line)
{
  int y;

  if (line < 313) {
    y = line * 2;

  } else {
    y = (line - 313) * 2 + 1;
  }

  return y;
}


void Video::copy_scanline_to_pixmap()
{
  int i, y;
  int last = line[0];
  cairo_t *cr = cairo_create(image);
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

  // Clear any skipped lines.
  if (last_line_nr > line_nr) {
    last_line_nr = 0;  // new frame
  }

  if (last_line_nr < line_nr - 1) {
    int l;

    for (l = last_line_nr; l < line_nr; l++) {
      for (i = 0; i < XRES; i++) {
        shadow[i][l] = 0;
      }

      y = screen_y_from_line(l);
      cairo_move_to(cr, 0.0, y);
      cairo_line_to(cr, XRES - 1, y);
      cairo_stroke(cr);
    }
  }

  last_line_nr = line_nr;

  // Fill unfilled values in the line.
  for (i = 1; i < XRES; i++) {
    if (line[i] & 0x80) { // Not written, use last written value
      line[i] = last;
    }

    last = line[i];
  }

  // Get screen y position
  y = screen_y_from_line(line_nr);
  cairo_surface_flush(image);
  unsigned char *data = cairo_image_surface_get_data(image);
  int stride = cairo_image_surface_get_stride(image);

  // Draw changes compared to shadow
  for (i = 1; i < XRES; i++) {
    if (line[i] == shadow[i][line_nr]) {
      continue;
    }

    shadow[i][line_nr] = line[i];

    if (line_nr < 313) {
      y = line_nr * 2;

    } else {
      y = (line_nr - 313) * 2 + 1;
    }

    guint32 *p = (guint32 *)(data + (i + y * stride));

    if (line[i] >= 4) {
      *p = 0x00ffffff;

    } else if (line[i] > 2) {
      *p = 0x007f7f7f;

    } else {
      *p = 0x00000000;
    }
  }

  cairo_surface_mark_dirty(image);
  cairo_destroy(cr);
}


guint64 Video::cycles_to_us(guint64 cycles)
{
  double ret = 0.0;

  if (cpu) {
    ret = cycles * 4000000.0 / cpu->get_frequency();
  }

  return (guint64) ret;
}


guint64 Video::us_to_cycles(guint64 us)
{
  double ret = 0.0;

  if (cpu) {
    ret = us * cpu->get_frequency() / 4000000.0;
  }

  return (guint64) ret;
}


void Video::refresh()
{
  gdk_window_invalidate_rect(gtk_widget_get_window(da), nullptr, FALSE);
}


void Video::update_state()
{
  guint64 cycletime;
  guint64 index;
  //int val=(int)(lume_pin->get_Vth()); // needs more work to make this work
  int val = (lume_pin->getDrivenState()) ? 4 : 0; // 4 to get it above the 2 threshold for visibility maybe use Vth??
  // get the current simulation cycle time from gpsim.
  cycletime = get_cycles().get();

  if (sync_time > cycletime) {
    // Cycle counter rolled over.
    sync_time += us_to_cycles(64); // 64 us = 1 line
    assert(sync_time <= cycletime);
  }

  // Index into line buffer. Calculated from sync_time.
  index = cycles_to_us((cycletime - sync_time) * (XRES / 64));

  if (cycletime - sync_time > us_to_cycles(70)) {
    // Long time with no sync pulses
    // Shouldn't happen?
    // If there was a long time since last sync, we jump over a line.
    sync_time += us_to_cycles(64); // 64 us = 1 line
    memset(line, 0x80, XRES); // clear line buffer
  }

  if (last_port_value == 1 && (sync_pin->getDrivenState() ? 1 : 0) == 0) { // Start of sync
    // Every 32 or 64us there should be a sync.
    sync_time = cycletime;

    // Start measure on negative flank, when we have lots in buffer.
    if (index > XRES - XRES / 5) {
      // Have a full line
      if (shortsync_counter > 0) {
        // We are on first line in the frame.
        if (shortsync_counter > last_shortsync_counter) {
          line_nr = 6;
          // Draw the last full image
          refresh();

        } else if (shortsync_counter < last_shortsync_counter) {
          line_nr = 318;

        } else {
          puts("VSYNC error");
          printf("%d, %d\n", shortsync_counter, last_shortsync_counter);
          //bp.halt(); useful for debug
        }

        last_shortsync_counter = shortsync_counter;
        shortsync_counter = 0;
      }

      copy_scanline_to_pixmap(); // display last line
      line_nr++;

      if (line_nr >= YRES) {
        line_nr = 0;
      }

      memset(line, 0x80, XRES);
      index = 0;

    } else if (index > XRES / 3 && index < XRES - XRES / 3) {
      // Shortsync
      //printf("shortsync %d\n",index);
      shortsync_counter++;
    }
  }

  if (last_port_value == 0 && (sync_pin->getDrivenState() ? 1 : 0) != 0) { // End of sync
    guint64 us_time;
    // Should be 4 us (0.000004*12000000
    us_time = cycles_to_us(cycletime - sync_time);
    /*	  if(us_time > 2 &&
    us_time < 6)
    {
    // Valid HSYNC
    }*/

    if (us_time > 25 && us_time < 35) {
      // vertical sync (FIXME, how should this be?)
      shortsync_counter = 0;
    }
  }

  if (index >= XRES) {
    index = XRES - 1;
  }

  line[index] = val;
  last_port_value = (sync_pin->getDrivenState() ? 1 : 0);
}

#endif // HAVE_GUI
