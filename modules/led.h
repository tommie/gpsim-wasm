/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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


#ifndef MODULES_LED_H_
#define MODULES_LED_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <gtk/gtk.h>

#include "../src/modules.h"

namespace Leds {

class LED_Interface;  // Defined in led.cc
class Led_Input;

enum ActiveStates {
  HIGH,
  LOW
};

enum Colors {
  RED = 0,
  ORANGE,
  GREEN,
  YELLOW,
  BLUE		// Must be last
};


//------------------------------------------------------------------------
// LED base class

class Led_base {
public:
  virtual ~Led_base() {}

  virtual void build_window() = 0;
  virtual void update() = 0;

  unsigned int interface_seq_no;
};


//------------------------------------------------------------------------
// 7-segment leds

// define a point
typedef struct {
  double x;
  double y;
} XfPoint;


#define MAX_PTS         6       /* max # of pts per segment polygon */
#define NUM_SEGS        7       /* number of segments in a digit */
/*
 * These constants give the bit positions for the segmask[]
 * digit masks.
 */
#define TOP             0
#define TOP_RIGHT       1
#define BOT_RIGHT       2
#define BOTTOM          3
#define BOT_LEFT        4
#define TOP_LEFT        5
#define MIDDLE          6
//#define DECIMAL_POINT   7

typedef XfPoint segment_pts[NUM_SEGS][MAX_PTS];


class Led_7Segments : public Module, public Led_base {
public:
  guint w_width;
  guint w_height;

  segment_pts seg_pts;

  GtkWidget *darea;

  explicit Led_7Segments(const char *);
  ~Led_7Segments();

  void build_segments(int w, int h);

  virtual void build_window();
  virtual void update();
  unsigned int getPinState();

  // Inheritances from the Package class
  virtual void create_iopin_map();

  // Inheritance from Module class
  const virtual char *type()
  {
    return "led_7segments";
  }
  static Module *construct(const char *new_name);

private:
  static gboolean led7_expose_event(GtkWidget *widget, GdkEvent *event,
                                    gpointer user_data);

  //unsigned int m_segmentStates;
  Led_Input *m_pins[8];
  int m_nPins;
};


//------------------------------------------------------------------------
// Simple LED
//
class ColorAttribute;
class ActiveStateAttribute;


class Led: public Module, public Led_base {
public:
  GtkWidget *darea;

  GdkColor led_on_color[BLUE + 1];
  GdkColor led_segment_off_color;
  int w_width, w_height;

  explicit Led(const char *);
  ~Led();

  virtual void build_window();
  virtual void update();

  // Inheritances from the Package class
  virtual void create_iopin_map();

  // Inheritance from Module class
  const virtual char *type()
  {
    return "led";
  }
  static Module *construct(const char *new_name);
  Colors get_on_color()
  {
    return on_color;
  }
  void set_on_color(Colors color);
  ActiveStates get_the_activestate()
  {
    return the_activestate;
  }
  void set_the_activestate(ActiveStates activestate);

private:
  static gboolean led_expose_event(GtkWidget *widget, GdkEvent *event,
                                   gpointer user_data);

  Led_Input *m_pin;
  Colors on_color;
  ColorAttribute *m_colorAttribute;
  ActiveStates the_activestate;
  ActiveStateAttribute *m_activestateAttribute;
};


} // end of namespace Led
#endif //  MODULES_LED_H_
