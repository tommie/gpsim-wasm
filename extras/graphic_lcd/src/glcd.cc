/*
   Copyright (C) 2007 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <config.h>

#ifdef HAVE_GUI

#include "glcd.h"
#include <src/trace.h>

//------------------------------------------------------------------------
gLCD::gLCD(unsigned int cols,
           unsigned int rows,
           unsigned int pixel_size_x,
           unsigned int pixel_size_y,
           unsigned int pixel_gap,
           unsigned int nColors
          )
  : m_nColumns(cols), m_nRows(rows), m_border(3),
    m_xPixel(pixel_size_x), m_yPixel(pixel_size_y),
    m_pixelGap(pixel_gap)
{
  m_nColors = nColors < 2 ? 2 : nColors;
  m_Colors = new LCDColor[m_nColors];
  setColor(0, double(0x78) / 255.0, double(0xa8) / 255.0, double(0x78) / 255.0);
  setColor(1, double(0x11) / 255.0, double(0x33) / 255.0, double(0x11) / 255.0);
}


gLCD::~gLCD()
{
  delete [] m_Colors;
}


void gLCD::clear(cairo_t *cr)
{
  double r = m_Colors[0].r;
  double g = m_Colors[0].g;
  double b = m_Colors[0].b;
  cairo_set_source_rgb(cr, r, g, b);
  cairo_rectangle(cr, 0.0, 0.0,
                  m_xPixel * (m_nColumns + 2 * m_border), m_yPixel * (m_nRows + 2 * m_border));
  cairo_fill(cr);
}


void gLCD::setPixel(cairo_t *cr, unsigned int col, unsigned int row, double r, double g, double b)
{
  double x = (col + m_border) * m_xPixel;
  double y = (row + m_border) * m_yPixel;
  double px = m_xPixel - m_pixelGap;
  double py = m_yPixel - m_pixelGap;
  cairo_set_source_rgb(cr, r, g, b);
  cairo_set_line_width(cr, 0.5);
  cairo_rectangle(cr, x, y, px, py);
  cairo_fill(cr);
}


void gLCD::setPixel(cairo_t *cr, unsigned int col, unsigned int row)
{
  if (col < m_nColumns && row < m_nRows) {
    double r = m_Colors[1].r;
    double g = m_Colors[1].g;
    double b = m_Colors[1].b;
    setPixel(cr, col, row, r, g, b);
  }
}


void gLCD::setPixel(cairo_t *cr, unsigned int col, unsigned int row, unsigned int colorIdx)
{
  if (colorIdx < m_nColors) {
    setPixel(cr, col, row, m_Colors[colorIdx].r, m_Colors[colorIdx].g, m_Colors[colorIdx].b);
  }
}


void gLCD::setColor(unsigned int colorIdx, double r, double g, double b)
{
  if (colorIdx < m_nColors) {
    m_Colors[colorIdx].r = r;
    m_Colors[colorIdx].g = g;
    m_Colors[colorIdx].b = b;
  }
}


//========================================================================
//
// LCD interface to the simulator
//
gLCD_Interface::gLCD_Interface(gLCD_Module *_lcd)
  : Interface((gpointer *) _lcd), plcd(_lcd)
{
}


//--------------------------------------------------
// SimulationHasStopped (gpointer)
//
// gpsim will call this function when the simulation
// has halt (e.g. a break point was hit.)

void gLCD_Interface::SimulationHasStopped(gpointer)
{
  if (plcd) {
    plcd->Update();
  }
}


void gLCD_Interface::Update(gpointer)
{
  if (plcd) {
    plcd->Update();
  }
}


//========================================================================
//
//

gLCD_Module::gLCD_Module(const char *new_name, const char *desc,
                         unsigned int nCols, unsigned int nRows)
  : Module(new_name, desc),
    window(nullptr), darea(nullptr), m_plcd(nullptr),
    m_nColumns(nCols), m_nRows(nRows)
{
  interface_seq_no = get_interface().add_interface(new gLCD_Interface(this));
}


gLCD_Module::~gLCD_Module()
{
  get_interface().remove_interface(interface_seq_no);
  delete m_plcd;
}


void gLCD_Module::Update(GtkWidget * )
{
}


//========================================================================
//
LcdPortRegister::LcdPortRegister(gLCD_Module *plcd, const char * _name, const char *_desc)
  : PortRegister(plcd, _name, _desc, 8, 0), m_pLCD(plcd)
{
  mMTT = new ModuleTraceType(plcd, 1, " Graphic LCD");
  trace.allocateTraceType(mMTT);
  RegisterValue rv(mMTT->type(), mMTT->type() + (1 << 22));
  set_write_trace(rv);
  rv = RegisterValue(mMTT->type() + (2 << 22), mMTT->type() + (3 << 22));
  set_read_trace(rv);
}


LcdPortRegister::~LcdPortRegister()
{
  delete mMTT;
}


#endif //HAVE_GUI
