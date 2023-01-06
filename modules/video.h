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


#ifndef MODULES_VIDEO_H_
#define MODULES_VIDEO_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/stimuli.h"
#include "../src/modules.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <glib.h>

class Video;
class Processor;

/*********************************************************
 *
 * Create a class derived from the IOPIN class that
 * will allow us to intercept when the I/O input is being
 * driven. (This isn't done for PIC I/O pins because the
 * logic for handling I/O pin changes resides in the ???
 * class.)
 */


class IOPIN_Monitor : public IOPIN {
private:
  Video *video;

public:
  IOPIN_Monitor(Video *v, const char *opt_name)
    : IOPIN(opt_name), video(v)
  {
  }

  void setDrivenState(bool new_state) override;
};


#define XRES 640
#define YRES 625

class Video : public Module {
public:
  explicit Video(const char *);
  ~Video();

  // Inheritances from the Package class
  virtual void create_iopin_map();

  virtual void update_state();
  virtual int get_num_of_pins()
  {
    return 2;
  }
  void copy_scanline_to_pixmap();
  int check_for_vrt1();
  int check_for_vrt2();
  guint64 cycles_to_us(guint64 cycles);
  guint64 us_to_cycles(guint64 cycles);
  void refresh();
  static Module *construct(const char *new_name);

  IOPIN *sync_pin;
  IOPIN *lume_pin;
  guint64 sync_time; // gpsim cycle counter at last H-sync
  int scanline;
  unsigned char line[XRES];         // buffer for one line
  unsigned char shadow[XRES][YRES]; // pixmap mirror
  Processor *cpu;

  GtkWidget *window;
  GtkWidget *da;
  cairo_surface_t *image;

  int line_nr;
  int last_line_nr;

private:
  int last_port_value = 0;
  int shortsync_counter = 0;
  int last_shortsync_counter = 0;
};

#endif // MODULES_VIDEO_H_

