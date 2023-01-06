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

  led.cc

  This is an example module illustrating how gpsim modules may be created.
  Additional examples may also be found with the gpsim.

  This particular example creates a 7-segment, common cathode LED display.

  Pin Numbering of LED:
  --------------------
       a
      ---
   f | g | b
      ---
   e |   | c
      ---
       d
  cc = common cathode


  Electrical:
  ----------

   a  ---|>|---+
   b  ---|>|---+
   c  ---|>|---+
   d  ---|>|---+
   e  ---|>|---+
   f  ---|>|---+
   g  ---|>|---+
               |
              cc

  How It Works:
  ------------

  Once the Led module has been built (and optionally installed), you
  can include it in your .stc file. See the examples subdirectory.

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <typeinfo>

#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#include <cmath>

#include "../src/stimuli.h"
#include "../src/value.h"
#include "../src/gpsim_interface.h"

#include "led.h"
#include "../src/packages.h"

namespace Leds {

//--------------------------------------------------------------
//
// Create an "interface" to gpsim
//


class LED_Interface : public Interface {
private:
  Led_base *led;

public:
  virtual void SimulationHasStopped(gpointer object)
  {
    Update(object);
  }
  virtual void Update(gpointer /* object */ )
  {
    if (led) {
      led->update();
    }
  }

  explicit LED_Interface(Led_base *_led)
    : Interface((gpointer *) _led), led(_led)
  {
  }
};


class Led_Input : public IOPIN {
public:
  Led_Input(const std::string &n, Led_base *pParent);

  virtual void setDrivenState(bool);
  virtual void get(char *return_str, int len);

private:
  Led_base *m_pParent;
};


//------------------------------------------------------------------------
//
Led_Input::Led_Input(const std::string &n, Led_base *pParent)
  : IOPIN(n.c_str()), m_pParent(pParent)
{
}


void Led_Input::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);
}


void Led_Input::get(char *return_str, int len)
{
  if (return_str) {
    strncpy(return_str, IOPIN::getState() ? "1" : "0", len);
  }
}


//------------------------------------------------------------------------
void Led_7Segments::update()
{
  if (get_interface().bUsingGUI()) {
    gtk_widget_queue_draw(darea);
  }
}


gboolean Led_7Segments::led7_expose_event(GtkWidget *widget, GdkEvent * ,
    gpointer user_data)
{
  Led_7Segments *led = static_cast<Led_7Segments *>(user_data);
  g_return_val_if_fail(widget != nullptr, TRUE);
  g_return_val_if_fail(GTK_IS_DRAWING_AREA(widget), TRUE);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  guint max_width = allocation.width;
  guint max_height = allocation.height;
  // not a very O-O way of doing it... but here we go directly
  // to the I/O port and get the values of the segments
  int segment_states = led->getPinState();
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_rectangle(cr, 0.0, 0.0, max_width, max_height);
  cairo_fill(cr);

  for (int i = 0; i < 7; ++i) {
    // common cathode, cathode must be low to turn
    //digits on.
    if ((segment_states & 1) == 0 && segment_states & (2 << i)) {
      cairo_set_source_rgb(cr, 0.75, 0.0, 0.0);

    } else {
      cairo_set_source_rgb(cr, 0.25, 0.0, 0.0);
    }

    XfPoint *pts = &(led->seg_pts[i][0]);
    cairo_move_to(cr, pts[0].x, pts[0].y);

    for (int j = 1; j < MAX_PTS; ++j) {
      cairo_line_to(cr, pts[j].x, pts[j].y);
    }

    cairo_line_to(cr, pts[0].x, pts[0].y);
    cairo_fill(cr);
  }

  cairo_destroy(cr);
  return TRUE;
}


//-------------------------------------------------------------------
// build_segments
//
// from Dclock.c (v.2.0) -- a digital clock widget.
// Copyright (c) 1988 Dan Heller <argv@sun.com>
// Modifications 2/93 by Tim Edwards <tim@sinh.stanford.edu>
// And further modifications by Scott Dattalo <scott@dattalo.com>
//
// Each segment on the LED is comprised of a 6 point polygon.
// This routine will calculate what those points should be and
// store them an arrary.

void Led_7Segments::build_segments(int w, int h)
{
  XfPoint *pts;
  float spacer, hskip, fslope, bslope, midpt, seg_width, segxw;
  float invcosphi, invsinphi, invcospsi, invsinpsi, slope;
  float dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy5, dy6;
  float xfactor, temp_xpts[4];
  w_width = w;
  w_height = h;
  // Hard code the display parameters...
  float space_factor = 0.13;
  float width_factor = 0.13;
  float sxw = 0.13;
  float angle = 6;
  /* define various useful constants */
  segxw = sxw * w;
  slope = angle;
  seg_width = width_factor * w;
  spacer = w * space_factor;
  hskip = seg_width * 0.125;
  fslope = 1 / (segxw / seg_width + 1 / slope);
  bslope = -1 / (segxw / seg_width - 1 / slope);
  midpt = h / 2;
  /* define some trigonometric values */
  /*  phi is the forward angle separating two segments;
      psi is the reverse angle separating two segments. */
  invsinphi = sqrt(1 + fslope * fslope) / fslope;
  invcosphi = sqrt(1 + 1 / (fslope * fslope)) * fslope;
  invsinpsi = sqrt(1 + bslope * bslope) / -bslope;
  invcospsi = sqrt(1 + 1 / (bslope * bslope)) * bslope;
  /* define offsets from easily-calculated points for 6 situations */
  dx1 = hskip * invsinphi / (slope / fslope - 1);
  dy1 = hskip * invcosphi / (1 - fslope / slope);
  dx2 = hskip * invsinpsi / (1 - slope / bslope);
  dy2 = hskip * invcospsi / (bslope / slope - 1);
  dx3 = hskip * invsinphi;
  dx4 = hskip * invsinpsi;
  dx5 = hskip * invsinpsi / (1 - fslope / bslope);
  dy5 = hskip * invcospsi / (bslope / fslope - 1);
  dx6 = dy5;
  dy6 = dx5;
  /* calculate some simple reference points */
  temp_xpts[0] = spacer + (h - seg_width) / slope;
  temp_xpts[1] = spacer + (h - seg_width / 2) / slope + segxw / 2;
  temp_xpts[2] = spacer + h / slope + segxw;
  temp_xpts[3] = temp_xpts[0] + segxw;
  xfactor = w - 2 * spacer - h / slope - segxw;
  // calculate the digit positions
  pts = seg_pts[TOP];
  pts[0].y = pts[1].y = 0;
  pts[0].x = temp_xpts[2] - dx3;
  pts[1].x = w - spacer - segxw + dx4;
  pts[2].y = pts[5].y = (seg_width / 2) - dy5 - dy6;
  pts[5].x = temp_xpts[1] + dx5 - dx6;
  pts[2].x = pts[5].x + xfactor;
  pts[3].y = pts[4].y = seg_width;
  pts[4].x = temp_xpts[3] + dx4;
  pts[3].x = temp_xpts[0] + xfactor - dx3;
  pts = &(seg_pts[MIDDLE][0]);
  pts[0].y = pts[1].y = midpt - seg_width / 2;
  pts[0].x = spacer + (h - pts[0].y) / slope + segxw;
  pts[1].x = pts[0].x - segxw + xfactor;
  pts[2].y = pts[5].y = midpt;
  pts[3].y = pts[4].y = midpt + seg_width / 2;
  pts[5].x = spacer + (h - pts[5].y) / slope + segxw / 2;
  pts[2].x = pts[5].x + xfactor;
  pts[4].x = pts[0].x - seg_width / slope;
  pts[3].x = spacer + (h - pts[3].y) / slope + xfactor;
  pts = &(seg_pts[BOTTOM][0]);
  pts[3].y = pts[4].y = (float)h;
  pts[2].y = pts[5].y = h - (seg_width / 2) + dy5 + dy6;
  pts[0].y = pts[1].y = h - seg_width;
  pts[0].x = spacer + segxw + seg_width / slope + dx3;
  pts[1].x = spacer + (h - pts[1].y) / slope + xfactor - dx4;
  pts[4].x = spacer + segxw - dx4;
  pts[5].x = spacer + segxw / 2 + (h - pts[5].y) / slope + dx6 - dx5;
  pts[2].x = pts[5].x + xfactor;
  pts[3].x = spacer + xfactor + dx3;
  pts = &(seg_pts[TOP_LEFT][0]);
  pts[0].y = seg_width / 2 - dy6 + dy5;
  pts[1].y = seg_width + dy2;
  pts[2].y = seg_pts[MIDDLE][0].y - 2 * dy1;
  pts[3].y = seg_pts[MIDDLE][5].y - 2 * dy6;
  pts[4].y = seg_pts[MIDDLE][0].y;
  pts[5].y = seg_width - dy1;
  pts[0].x = temp_xpts[1] - dx5 - dx6;
  pts[1].x = temp_xpts[3] - dx2;
  pts[2].x = seg_pts[MIDDLE][0].x + 2 * dx1;
  pts[3].x = seg_pts[MIDDLE][5].x - 2 * dx6;
  pts[4].x = spacer + (h - pts[4].y) / slope;
  pts[5].x = temp_xpts[0] + dx1;
  pts = &(seg_pts[BOT_LEFT][0]);
  pts[0].y = seg_pts[MIDDLE][5].y + 2 * dy5;
  pts[1].y = seg_pts[MIDDLE][4].y + 2 * dy2;
  pts[2].y = seg_pts[BOTTOM][0].y - dy1;
  pts[3].y = seg_pts[BOTTOM][5].y - 2 * dy6;
  pts[4].y = h - seg_width + dy2;
  pts[5].y = midpt + seg_width / 2;
  pts[0].x = seg_pts[MIDDLE][5].x - 2 * dx5;
  pts[1].x = seg_pts[MIDDLE][4].x - 2 * dx2;
  pts[2].x = seg_pts[BOTTOM][0].x - dx3 + dx1;
  pts[3].x = seg_pts[BOTTOM][5].x - 2 * dx6;
  pts[4].x = spacer + seg_width / slope - dx2;
  pts[5].x = spacer + (midpt - seg_width / 2) / slope;
  pts = &(seg_pts[TOP_RIGHT][0]);
  pts[0].y = seg_width / 2 - dy5 + dy6;
  pts[1].y = seg_width - dy2;
  pts[2].y = midpt - seg_width / 2;
  pts[3].y = midpt - 2 * dy5;
  pts[4].y = pts[2].y - 2 * dy2;
  pts[5].y = seg_width + dy1;
  pts[0].x = temp_xpts[1] + xfactor + dx5 + dx6;
  pts[1].x = temp_xpts[3] + xfactor + dx1;
  pts[2].x = seg_pts[MIDDLE][0].x + xfactor;
  pts[3].x = seg_pts[MIDDLE][5].x + xfactor + dx5 * 2;
  pts[4].x = seg_pts[TOP_LEFT][4].x + xfactor + dx2 * 2;
  pts[5].x = temp_xpts[0] + xfactor - dx1;
  pts = &(seg_pts[BOT_RIGHT][0]);
  pts[0].y = seg_pts[MIDDLE][2].y + 2 * dy6;
  pts[1].y = midpt + seg_width / 2;
  pts[2].y = h - seg_width + dy1;
  pts[3].y = h - (seg_width / 2) + dy6 - dy5;
  pts[4].y = h - seg_width - dy2;
  pts[5].y = seg_pts[MIDDLE][3].y + 2 * dy1;
  pts[0].x = seg_pts[MIDDLE][2].x + 2 * dx6;
  pts[1].x = seg_pts[MIDDLE][3].x + segxw;
  pts[2].x = seg_pts[BOTTOM][1].x + dx4 + segxw - dx1;
  pts[3].x = seg_pts[BOTTOM][2].x + 2 * dx5;
  pts[4].x = seg_pts[BOTTOM][1].x + dx4 + dx2;
  pts[5].x = seg_pts[MIDDLE][3].x - 2 * dx1;
}


void Led_7Segments::build_window()
{
  darea = gtk_drawing_area_new();
  gtk_widget_set_size_request(darea, 100, 110);
  g_signal_connect(darea, "expose_event", G_CALLBACK(led7_expose_event), this);
  gtk_widget_set_events(darea, GDK_EXPOSURE_MASK);
  gtk_widget_show(darea);
  set_widget(darea);
}


//--------------------------------------------------------------

Led_7Segments::Led_7Segments(const char *name)
  : Module(name, "7 Segment LED")
{
  if (get_interface().bUsingGUI()) {
    build_segments(100, 110);
    build_window();
  }

  interface_seq_no = get_interface().add_interface(new LED_Interface(this));
  create_iopin_map();
}


Led_7Segments::~Led_7Segments()
{
  for (int i = 0; i < 8; i++) {
    removeSymbol(m_pins[i]);
  }

  get_interface().remove_interface(interface_seq_no);
  //RRRgtk_widget_destroy(darea);
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Led_7Segments::create_iopin_map()
{
  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   The 7-segment LED has 8 pins
  create_pkg(8);
  float ypos = 6.0;

  for (int i = 1; i <= 8; i++) {
    package->setPinGeometry(i, 0.0, ypos, 0, false);
    ypos += 12.0;
  }

  // Here, we create and name the I/O pins. In gpsim, we will reference
  //   the bit positions as LED.seg0, LED.seg1, ..., where LED is the
  //   user-assigned name of the 7-segment LED
  m_pins[0] = new Led_Input("cc", this);
  addSymbol(m_pins[0]);
  assign_pin(1, m_pins[0]);

  std::string seg = "seg";
  int i;
  char ch;
  for (ch = '0', i = 1; i < 8; i++, ch++) {
    m_pins[i] = new Led_Input(seg + ch, this);
    addSymbol(m_pins[i]);
    assign_pin(i + 1, m_pins[i]);
  }
}


//--------------------------------------------------------------
unsigned int Led_7Segments::getPinState()
{
  unsigned int s = 0;

  for (int i = 1; i < 8; i++) {
    double delta_v = m_pins[i]->get_nodeVoltage() - m_pins[0]->get_nodeVoltage();
    s = (s >> 1) | (delta_v > 1.5 ? 0x80 : 0);
  }

  return s;
}


//--------------------------------------------------------------
// construct

Module * Led_7Segments::construct(const char *_new_name = nullptr)
{
  return new Led_7Segments(_new_name);
}


class ColorAttribute : public Value {
public:
  explicit ColorAttribute(Led *_led)
    : Value("color", "On color of LED"), m_led(_led)
  {
  }

  virtual void get(char *return_str, int len);
  virtual void set(const char *buffer, int buf_size = 0);
  virtual void set(Value *v);
  virtual bool Parse(const char *pValue, Colors &bValue);

private:
  Led *m_led;
};


void ColorAttribute::set(Value *v)
{
  if (typeid(*v) == typeid(String)) {
    char buff[20];
    v->get(buff, sizeof(buff));
    set(buff);

  } else {
    throw TypeMismatch("set ", "ColorAttribute", v->showType());
  }
}


void ColorAttribute::set(const char *buffer, int /* len */ )
{
  if (buffer) {
    Colors color;

    if (Parse(buffer, color)) {
      m_led->set_on_color(color);

    } else {
      std::cout << "ColorAttribute::set " << buffer << " unknown color\n";
    }
  }
}


void ColorAttribute::get(char *return_str, int len)
{
  if (return_str) {
    switch (m_led->get_on_color()) {
    case RED:
      g_strlcpy(return_str, "red", len);
      break;

    case ORANGE:
      g_strlcpy(return_str, "orange", len);
      break;

    case GREEN:
      g_strlcpy(return_str, "green", len);
      break;

    case YELLOW:
      g_strlcpy(return_str, "yellow", len);
      break;

    case BLUE:
      g_strlcpy(return_str, "blue", len);
      break;
    }
  }
}


bool ColorAttribute::Parse(const char *pValue, Colors &bValue)
{
  std::string s(pValue);

  if (s == "red") {
    bValue = RED;
    return true;

  } else if (s == "orange") {
    bValue = ORANGE;
    return true;

  } else if (s == "green") {
    bValue = GREEN;
    return true;

  } else if (s == "yellow") {
    bValue = YELLOW;
    return true;

  } else if (s == "blue") {
    bValue = BLUE;
    return true;
  }

  return false;
}


class ActiveStateAttribute : public Value {
public:
  explicit ActiveStateAttribute(Led *_led)
    : Value("ActiveState", "high or low"), m_led(_led)
  {
  }

  virtual void get(char *return_str, int len);
  virtual void set(const char *buffer, int buf_size = 0);
  virtual void set(Value *v);
  virtual bool Parse(const char *pValue, ActiveStates &bValue);

private:
  Led *m_led;
};


void ActiveStateAttribute::set(Value *v)
{
  if (typeid(*v) == typeid(String)) {
    char buff[20];
    v->get(buff, sizeof(buff));
    set(buff);

  } else {
    throw TypeMismatch("set ", "ActiveStateAttribute", v->showType());
  }
}


void ActiveStateAttribute::set(const char *buffer, int /* len */ )
{
  if (buffer) {
    ActiveStates activestate;

    if (Parse(buffer, activestate)) {
      m_led->set_the_activestate(activestate);

    } else {
      std::cout << "ActiveStateAttribute::set " << buffer << " unknown active state\n";
    }
  }
}


void ActiveStateAttribute::get(char *return_str, int len)
{
  if (return_str) {
    switch (m_led->get_the_activestate()) {
    case HIGH:
      g_strlcpy(return_str, "high", len);
      break;

    case LOW:
      g_strlcpy(return_str, "low", len);
      break;
    }
  }
}


bool ActiveStateAttribute::Parse(const char *pValue, ActiveStates &bValue)
{
  if (strncmp("high", pValue, sizeof("high")) == 0) {
    bValue = HIGH;
    return true;

  } else if (strncmp("low", pValue, sizeof("low")) == 0) {
    bValue = LOW;
    return true;
  }

  return false;
}


//-------------------------------------------------------------
// Led (simple)
//-------------------------------------------------------------
void Led::update()
{
  if (get_interface().bUsingGUI()) {
    gtk_widget_queue_draw(darea);
  }
}


void Led::set_on_color(Colors color)
{
  if (color != on_color) {
    on_color = color;

    if (get_interface().bUsingGUI()) {
      update();
    }
  }
}


void Led::set_the_activestate(ActiveStates activestate)
{
  if (activestate != the_activestate) {
    if (activestate == HIGH) {
      m_pin->set_Vth(0.0);

    } else {
      m_pin->set_Vth(3.5);  // includes LED voltage drop
    }

    the_activestate = activestate;

    if (get_interface().bUsingGUI()) {
      update();
    }
  }
}


gboolean Led::led_expose_event(GtkWidget *widget, GdkEvent * ,
                               gpointer user_data)
{
  Led *led = static_cast<Led *>(user_data);
  g_return_val_if_fail(widget != nullptr, TRUE);
  g_return_val_if_fail(GTK_IS_DRAWING_AREA(widget), TRUE);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  guint max_width = allocation.width;
  guint max_height = allocation.height;
  GdkWindow *gdk_win = gtk_widget_get_window(widget);
  cairo_t *cr = gdk_cairo_create(gdk_win);
  // Led is on when DrivenState=TRUE in current HIGH active state OR
  // when DrivenState=FALSE in current LOW active state.
  double delta_v ;

  if (led->get_the_activestate() == HIGH) {
    delta_v = led->m_pin->get_nodeVoltage() - led->m_pin->get_Vth();

  } else {
    delta_v = led->m_pin->get_Vth() - led->m_pin->get_nodeVoltage();
  }

  if (delta_v > 1.5) {
    gdk_cairo_set_source_color(cr, &led->led_on_color[led->on_color]);

  } else {
    gdk_cairo_set_source_color(cr, &led->led_segment_off_color);
  }

  cairo_arc(cr, max_width / 2, max_height / 2, max_width / 2, 0.0, 2 * G_PI);
  cairo_fill(cr);
  cairo_destroy(cr);
  return FALSE;
}


void Led::build_window()
{
  darea = gtk_drawing_area_new();
  w_height = 20;
  w_width = 20;
  gtk_widget_set_size_request(darea, w_height, w_width);
  g_signal_connect(darea,
                   "expose_event",
                   G_CALLBACK(led_expose_event),
                   this);
  gtk_widget_set_events(darea, GDK_EXPOSURE_MASK);
  gtk_widget_show(darea);
  set_widget(darea);
  // The default 'on' color is bright red
  gdk_color_parse("red3", &led_on_color[RED]);
  gdk_color_parse("orange", &led_on_color[ORANGE]);
  gdk_color_parse("green", &led_on_color[GREEN]);
  gdk_color_parse("yellow", &led_on_color[YELLOW]);
  gdk_color_parse("blue", &led_on_color[BLUE]);
  // The `off' color is dark red
  led_segment_off_color.red = 0x4000;
  led_segment_off_color.green = 0x0000;
  led_segment_off_color.blue = 0x0000;
}


//--------------------------------------------------------------

Led::Led(const char *name)
  : Module(name, "Simple LED"), on_color(RED), the_activestate(HIGH)
{
  create_iopin_map();
  // the following will load the driver of the input as would a real
  // LED
  m_pin->set_Zth(150.0);
  m_pin->set_Vth(0.0);

  if (get_interface().bUsingGUI()) {
    build_window();
  }

  m_colorAttribute = new ColorAttribute(this);
  addSymbol(m_colorAttribute);
  m_activestateAttribute = new ActiveStateAttribute(this);
  addSymbol(m_activestateAttribute);
  interface_seq_no = get_interface().add_interface(new LED_Interface(this));
}


Led::~Led()
{
  removeSymbol(m_pin);
  removeSymbol(m_colorAttribute);
  removeSymbol(m_activestateAttribute);
  get_interface().remove_interface(interface_seq_no);
  /*
    if (darea)
      gtk_widget_destroy(darea);
    darea = 0;
  */
  delete m_activestateAttribute;
  delete m_colorAttribute;
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Led::create_iopin_map()
{
  create_pkg(1);
  // Position pin on left side of package
  package->set_pin_position(1, 0.5);
  // Define the LED Cathode. (The anode is implicitly tied to VCC)
  m_pin = new Led_Input("in", this);
  addSymbol(m_pin);
  assign_pin(1, m_pin);
}


//--------------------------------------------------------------
// construct

Module * Led::construct(const char *_new_name = nullptr)
{
  return new Led(_new_name);
}

}
#endif //HAVE_GUI
