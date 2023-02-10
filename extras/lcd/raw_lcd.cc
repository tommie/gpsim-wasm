/*
   Copyright (C) 2017 Roy R. Rankin
   Copyright (C) 2000 T. Scott Dattalo

This file is part of the libgpsim_extras library of gpsim

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

  lcd.cc

  This is an example module illustrating how gpsim modules may be created.
  Additional examples may also be found with the gpsim.

  This particular example creates a 7-segment, common cathode LCD display.

  Pin Numbering of LCD:
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

  Once the LCD module has been built (and optionally installcd), you
  can include it in your .stc file. See the examples subdirectory.

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <config.h>    // get the definition for HAVE_GUI

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#include <cmath>

#include "../src/stimuli.h"
#include "../src/value.h"
#include "../src/gpsim_interface.h"
#include "../src/gpsim_time.h"

#include "raw_lcd.h"
#include "../src/packages.h"


//--------------------------------------------------------------
//
// Create an "interface" to gpsim
//


class RAW_LCD_Interface : public Interface {
private:
  RAW_LCD_base *lcd;

public:
  explicit RAW_LCD_Interface(RAW_LCD_base *_lcd)
    : Interface((gpointer *) _lcd), lcd(_lcd)
  {
  }

  virtual void SimulationHasStopped(gpointer object)
  {
    Update(object);
  }
  virtual void Update(gpointer /* object */ )
  {
    if (lcd) {
      lcd->update();
    }
  }
};


//------------------------------------------------------------------------
void LCD_7Segments::update()
{
  if (get_interface().bUsingGUI()) {
    gtk_widget_queue_draw(darea);
  }
}


gboolean LCD_7Segments::lcd7_expose_event(GtkWidget *widget, GdkEvent * ,
    gpointer user_data)
{
  LCD_7Segments *lcd = static_cast<LCD_7Segments *>(user_data);
  unsigned int segment_states;
  g_return_val_if_fail(widget != nullptr, TRUE);
  g_return_val_if_fail(GTK_IS_DRAWING_AREA(widget), TRUE);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  guint max_width = allocation.width;
  guint max_height = allocation.height;
  // not a very O-O way of doing it... but here we go directly
  // to the I/O port and get the values of the segments
  lcd->set_cc_stimulus();
  segment_states =  lcd->segments;
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_rectangle(cr, 0.0, 0.0, max_width, max_height);
  cairo_fill(cr);

  for (int i = 0; i < 7; ++i) {
    if ((segment_states & 1) == 0 && segment_states & (2 << i)) {
      cairo_set_source_rgb(cr, 0.750, 0.750, 0.750);

    } else {
      cairo_set_source_rgb(cr, 0.00, 0.0, 0.0);
    }

    XfPoint *pts = &(lcd->seg_pts[i][0]);
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
// Each segment on the LCD is comprised of a 6 point polygon.
// This routine will calculate what those points should be and
// store them an arrary.

void LCD_7Segments::build_segments(int w, int h)
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


void LCD_7Segments::build_window()
{
  darea = gtk_drawing_area_new();
  gtk_widget_set_size_request(darea, 100, 110);
  g_signal_connect(darea, "expose_event", G_CALLBACK(lcd7_expose_event), this);
  gtk_widget_set_events(darea, GDK_EXPOSURE_MASK);
  gtk_widget_show(darea);
  set_widget(darea);
}


//--------------------------------------------------------------

LCD_7Segments::LCD_7Segments(const char *name)
  : Module(name, "7 Segment LCD"), cc_stimulus(nullptr)
{
  segments = 0;

  if (get_interface().bUsingGUI()) {
    build_segments(100, 110);
    build_window();
  }

  interface_seq_no = get_interface().add_interface(new RAW_LCD_Interface(this));
  create_iopin_map();
}


LCD_7Segments::~LCD_7Segments()
{
  if (m_pins[0]->snode) {
    (m_pins[0]->snode)->detach_stimulus(cc_stimulus);
    delete cc_stimulus;
  }

  for (int i = 0; i < 8; i++) {
    removeSymbol(m_pins[i]);
    delete m_pins[i];
  }

  get_interface().remove_interface(interface_seq_no);
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void LCD_7Segments::create_iopin_map()
{
  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   The 7-segment LCD has 8 pins
  create_pkg(8);
  float ypos = 6.0;

  for (int i = 1; i <= 8; i++) {
    package->setPinGeometry(i, 0.0, ypos, 0, false);
    ypos += 12.0;
  }

  // Here, we create and name the I/O pins. In gpsim, we will reference
  //   the bit positions as LCD.seg0, LCD.seg1, ..., where LCD is the
  //   user-assigned name of the 7-segment LCD
  m_pins[0] = new IOPIN("cc", 0.0);
  addSymbol(m_pins[0]);
  assign_pin(1, m_pins[0]);
  int i;
  char ch;

  for (ch = '0', i = 1; i < 8; i++, ch++) {
    char seg_name[] = "seg0";
    seg_name[3] = ch;
    //m_pins[i] = new PinModule(seg_name, 0.0);
    m_pins[i] = new IOPIN(seg_name, 0.0);
    addSymbol(m_pins[i]);
    assign_pin(i + 1, m_pins[i]);
  }
}


void LCD_7Segments::new_cc_voltage(double Vcc)
{
  if (Vcc > 2.5) {
    unsigned int s = 0;

    for (int i = 1; i < 8; i++) {
      double delta_v = Vcc - m_pins[i]->get_nodeVoltage();
      s = (s >> 1) | (delta_v > 1.5 ? 0x80 : 0);
    }

    if (s != segments) {
      segments = s;
      lcd7_expose_event(darea, 0, this);
    }
  }
}


//--------------------------------------------------------------
void LCD_7Segments::set_cc_stimulus()
{
  if (m_pins[0]->snode && ! cc_stimulus) {
    cc_stimulus = new CC_stimulus(this, "cc_stimulus", 0, 1e12);
    (m_pins[0]->snode)->attach_stimulus(cc_stimulus);
  }
}


//--------------------------------------------------------------
// construct

Module * LCD_7Segments::construct(const char *_new_name = nullptr)
{
  return new LCD_7Segments(_new_name);
}


CC_stimulus::CC_stimulus(LCD_7Segments * arg, const char *cPname, double _Vth, double _Zth)
  : stimulus(cPname, _Vth, _Zth), future_cycle(0)
{
  _lcd_7seg = arg;
}


CC_stimulus::~CC_stimulus()
{
}


void CC_stimulus::set_nodeVoltage(double v)
{
  if (nodeVoltage != v) {
    nodeVoltage = v;

    if (future_cycle) {
      get_cycles().clear_break(future_cycle);
    }

    future_cycle = get_cycles().get(2e-3); // 2ms
    get_cycles().set_break(future_cycle, this);
  }
}


void CC_stimulus::callback()
{
  Dprintf(("callback %s\n", name().c_str()));
  future_cycle = 0;
  Dprintf(("set_nodeVoltage %s _lcd_7seg %p %s v=%.2f\n", name().c_str(), _lcd_7seg, _lcd_7seg->name().c_str(), nodeVoltage));
  _lcd_7seg->new_cc_voltage(nodeVoltage);
}


#endif //HAVE_GUI
