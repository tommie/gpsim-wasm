/*
   Copyright (C) 2004 T. Scott Dattalo

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


/*
           S C O P E

A simple waveform viewer for gpsim. Inspired from Nuno Sucena Almeida's
gpsim_la - plug in.

*/

#include <config.h>
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib-object.h>

#include <cairo.h>

#include "../src/bitlog.h"
#include "../src/gpsim_object.h"
#include "../src/gpsim_time.h"
#include "../src/symbol.h"
#include "../src/value.h"
#include "../src/stimuli.h"

#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>

#include "gui.h"
#include "gui_scope.h"

static GdkColor signal_line_color, grid_line_color;

class WaveformSink;
class Waveform;
/***********************************************************************
  timeMap - a structure that's used to map horizontal screen coordinates
  into simulation time.
*/
class timeMap {
public:
  timeMap() : time(0.0), pos(0), eventIndex(0), event(0) {}

  // simulation time
  double time;

  // pixel x-coordinate
  int     pos;

  // index into array holding event (fixme - exposes Logger implementation)
  unsigned int eventIndex;

  // The event
  int event;
};


/***********************************************************************
  WaveSource

  This is an attribute that provides the interface between the WaveForm
  class (below) and the names of nodes or stimuli that source information.

*/
class WaveformSource : public String {
public:
  WaveformSource(Waveform *pParent, const char *_name);

  // Override the String::set method so that name assignments can
  // be intercepted.
  void set(const char *cp, int len = 0) override;

private:
  Waveform *m_pParent;
  bool m_bHaveSource;
};


/***********************************************************************
 zoom
 */
class ZoomAttribute : public Integer {
public:
  explicit ZoomAttribute(Scope_Window *);
  void set(gint64 i) override;

private:
  Scope_Window *m_pSW;
};


/***********************************************************************
 pan
 */
class PanAttribute : public Integer {
public:
  explicit PanAttribute(Scope_Window *);
  void set(gint64 i) override;

private:
  Scope_Window *m_pSW;
};


/***********************************************************************
 */

GtkWidget *waveDrawingArea = nullptr;
GtkWidget *signalDrawingArea = nullptr;

/***********************************************************************
  Wavebase class

*/
class WaveBase {
public:
  WaveBase(Scope_Window *parent, const char *name);

  virtual ~WaveBase()
  {
  }

  virtual void Update(guint64 start = 0, guint64 stop = 0) = 0;
  virtual void Build(gint w, gint h, gint y);

  const char *get_text()
  {
    return m_label_name.c_str();
  }
  gint get_yoffset()
  {
    return yoffset;
  }
  virtual void setSource(const char *) {}
  virtual void setWidth ( int w )   { width = w; }

protected:
  Scope_Window *sw;          // Parent
  guint64 m_start;           // Start time of plotted waveform
  guint64 m_stop;            // Stop time of plotted waveform

  gint width;
  gint height;
  gint yoffset;

  std::string m_label_name;
};


/***********************************************************************
  TimeAxis - A special 'Waveform' for plotting the time
 */
class TimeAxis : public WaveBase {
public:
  TimeAxis(Scope_Window *parent, const char *name);

  virtual ~TimeAxis()
  {
  }

  void Build(gint w, gint h, gint y) override;
  void Update(guint64 start = 0, guint64 stop = 0) override;
  void draw(cairo_t *cr);

private:
  PangoLayout *m_TicText;
};


/***********************************************************************
  Waveform class

  This holds the gui information related with a gpsim waveform
*/
class Waveform : public WaveBase {
public:
  Waveform(Scope_Window *parent, const char *name);
  Waveform(const Waveform &) = delete;
  Waveform& operator =(const Waveform &) = delete;

  virtual ~Waveform()
  {
  }

  void Update(guint64 start = 0, guint64 stop = 0) override;
  void Build(gint w, gint h, gint y) override;
  void draw(cairo_t *cr);

  void setData(char c);
  void setSource(const char *) override;

protected:
  void PlotTo(cairo_t *cr, timeMap &left, timeMap &right);
  void SearchAndPlot(cairo_t *cr, timeMap &left, timeMap &right);
  void updateLayout();

  PinMonitor *m_ppm;
  WaveformSink *m_pSink;
  ThreeStateEventLogger m_logger;
  timeMap m_last;
  WaveformSource m_pSourceName;
};


/***********************************************************************
  WaveformSink - A "sink" is an object that receives data from a
  "source". In the context of waveform viewer, a source is a stimulus
  and the viewer is the sink.
*/
class WaveformSink : public SignalSink {
public:
  explicit WaveformSink(Waveform *pParent);
  void setSinkState(char) override;
  void release() override {}

protected:
  Waveform *m_pWaveform;
};


//
// SignalNameEntry - a class to control gui editing of signal names
//
class SignalNameEntry {
public:
  SignalNameEntry();
  ~SignalNameEntry();

  bool Select(WaveBase *);
  bool unSelect();
  bool isSelected(WaveBase *pWave)
  {
    return pWave == m_selectedWave;
  }
  WaveBase *getSelected()
  {
    return m_selectedWave;
  }
  GtkWidget *m_entry;

protected:
  WaveBase *m_selectedWave;
};


//========================================================================

gboolean Scope_Window::key_press(GtkWidget *, GdkEventKey *key,
                                 Scope_Window *sw)
{
  Dprintf(("press 0x%x\n", key->keyval));

  switch (key->keyval) {
  case 'z':
    sw->zoom(2);
    break;

  case 'Z':
    sw->zoom(-2);
    break;

  case 'l':
    sw->pan(-((gint64) sw->getSpan() / 4));
    break;

  case 'r':
    sw->pan((gint64) sw->getSpan() / 4);
    break;

  default:
    return FALSE;
  }

  return TRUE;
}


//========================================================================
// WaveformSource

WaveformSource::WaveformSource(Waveform *pParent, const char *_name)
  : String(_name, "", "view or set gui scope waveforms"),
    m_pParent(pParent), m_bHaveSource(false)
{
  assert(m_pParent);
  // Prevent removal from the symbol table (all clearable symbols are
  // removed from the symbol table when a new processor is loaded).
}


void WaveformSource::set(const char *cp, int len)
{
  if (!m_bHaveSource) {
    String::set(cp, len);
    m_pParent->setSource(cp);
    m_bHaveSource = true;
  }
}


//========================================================================

ZoomAttribute::ZoomAttribute(Scope_Window *pSW)
  : Integer("scope.zoom", 0, "Scope Zoom; positive values zoom in, negative values zoom out"),
    m_pSW(pSW)
{
  assert(m_pSW);
}


void ZoomAttribute::set(gint64 i)
{
  Integer::set(i);
  m_pSW->zoom(i);
}


//========================================================================

PanAttribute::PanAttribute(Scope_Window *pSW)
  : Integer("scope.pan", 0, "Scope Pan; positive values pan right, negative values pan left"),
    m_pSW(pSW)
{
  assert(m_pSW);
}


void PanAttribute::set(gint64 i)
{
  Integer::set(i);
  m_pSW->pan(i);
}


//========================================================================
WaveformSink::WaveformSink(Waveform *pParent)
  : SignalSink(), m_pWaveform(pParent)
{
  assert(m_pWaveform);
}


void WaveformSink::setSinkState(char c)
{
  m_pWaveform->setData(c);
}


//************************************************************************
WaveBase::WaveBase(Scope_Window *parent, const char * /* name */)
  : sw(parent), m_start(1), m_stop(1)
{
}


void WaveBase::Build(gint w, gint h, gint y)
{
  width = w;
  height = h;
  yoffset = y;
  Update(0, 0);
}


//************************************************************************
Waveform::Waveform(Scope_Window *parent, const char *name)
  : WaveBase(parent, name), m_ppm(nullptr), m_pSourceName(this, name)
{
  m_pSink = new WaveformSink(this);
  globalSymbolTable().addSymbol(&m_pSourceName);
  m_logger.event('0');
}


void Waveform::Build(gint w, gint h, gint y)
{
  WaveBase::Build(w, h, y);
  updateLayout();
}


void Waveform::setData(char c)
{
  m_logger.event(c);
}


void Waveform::updateLayout()
{
  char buffer[100];
  m_pSourceName.get(buffer, sizeof(buffer));
  m_label_name = buffer;
}


void Waveform::setSource(const char *sourceName)
{
  //IOPIN *ppin = dynamic_cast<IOPIN*>(get_symbol_table().findStimulus(sourceName));
  IOPIN *ppin = dynamic_cast<IOPIN*>(globalSymbolTable().find(std::string(sourceName)));

  if (ppin) {
    if (m_ppm) {
      m_ppm->removeSink(m_pSink);
    }

    m_ppm = ppin->getMonitor();

    if (m_ppm) {
      m_ppm->addSink(m_pSink);
    }

    updateLayout();
    // Invalidate wave area.
    m_start = m_stop = 1;
    Update(0, 0);

    if (sw) {
      if (signalDrawingArea) {
        gtk_widget_queue_draw(signalDrawingArea);
      }

      if (waveDrawingArea) {
        gtk_widget_queue_draw(waveDrawingArea);
      }
    }

  } else if (sourceName) {
    printf("'%s' is not a valid source for the scope\n", sourceName);
  }
}


//----------------------------------------
//
void Waveform::PlotTo(cairo_t *cr, timeMap &left, timeMap &right)
{
  // Event(s) has(have) been found.
  // The plotting region has been subdivided as finely as possible
  // and there are one or more events.
  // First draw a horizontal line from the last known event to here:
  cairo_move_to(cr, m_last.pos, m_last.event + yoffset);
  cairo_line_to(cr, right.pos, m_last.event + yoffset);
  // Now draw a vertical line for the event
  int nextEvent = (m_logger.get_state(right.eventIndex) == '1')
                  ? 1 : (height - 3);
  // Draw a thicker line if there is more than one event.
  unsigned int nEvents = m_logger.get_nEvents(left.eventIndex, right.eventIndex);

  if (nEvents > 1) {
    cairo_save(cr);
    guint16 c = (nEvents < 4) ? (0x4000 * nEvents + 0x8000) : 0xffff;

    if (left.pos != right.pos) {
      cairo_move_to(cr, left.pos, 1 + yoffset);
      cairo_line_to(cr, left.pos, height - 3 + yoffset);
      cairo_stroke(cr);
    }

    // variations of yellow
    cairo_set_source_rgb(cr, 1.0, 1.0, double(c) / 65535.0);
    cairo_move_to(cr, right.pos, 1 + yoffset);
    cairo_line_to(cr, right.pos, height - 3 + yoffset);
    cairo_stroke(cr);
    cairo_restore(cr);

  } else {
    cairo_move_to(cr, right.pos, m_last.event + yoffset);
    cairo_line_to(cr, right.pos, nextEvent + yoffset);
  }

  cairo_stroke(cr);
  m_last = right;
  m_last.event = nextEvent;
}


//----------------------------------------
//
// Waveform SearchAndPlot
//
//  Recursively divide the plotting area into smaller and smaller
// regions until either only one event exists or the region can
// be divided no smaller.
//

void Waveform::SearchAndPlot(cairo_t *cr, timeMap &left, timeMap &right)
{
  if (right.eventIndex == left.eventIndex)
    // The time span cannot be divided any smaller.
    // If there are no events in this subdivided region
    // So just return.
    // m_last = left;
    ;

  else if (left.pos + 1 >= right.pos) {
    PlotTo(cr, left, right);

  } else {
    // the subdivided region is larger than 1-pixel wide
    // and there is at least one event. So subdivide even smaller
    // and recursively call
    timeMap mid;
    mid.time = (left.time + right.time) / 2;
    mid.pos  = (left.pos  + right.pos)  / 2;
    mid.eventIndex = m_logger.get_index((guint64)mid.time);
    SearchAndPlot(cr, left, mid);
    SearchAndPlot(cr, mid, right);
  }
}


//----------------------------------------
//
// Waveform Update
//

void Waveform::Update(guint64 uiStart, guint64 uiEnd)
{
  if (uiEnd == 0) {
    uiEnd = get_cycles().get();
  }

  if (m_start == uiStart && m_stop == uiEnd) {
    return;
  }

  m_start = uiStart;
  m_stop  = uiEnd;
  updateLayout();
}


void Waveform::draw(cairo_t *cr)
{
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_rectangle(cr, 0.0, yoffset, width, yoffset + height);
  cairo_fill(cr);
  // Draw vertical grid lines
  gdk_cairo_set_source_color(cr, &grid_line_color);

  for (int i = 0; i < sw->MajorTicks().sze(); ++i) {
    int x = sw->MajorTicks().pixel(i);
    cairo_move_to(cr, x, yoffset + 1);
    cairo_line_to(cr, x, yoffset + height - 1);
  }

  // Draw horizontal grid lines
  cairo_move_to(cr, 0.0, yoffset + height - 1);
  cairo_line_to(cr, width, yoffset + height - 1);
  cairo_stroke(cr);

  if (m_stop == 0) {
    return;
  }

  // Draw signal
  timeMap left;
  timeMap right;
  left.pos = 0;
  left.time = m_start;
  left.eventIndex = m_logger.get_index(m_start);
  left.event = (m_logger.get_state(left.eventIndex) == '1') ? 1 : (height - 3);
  m_last = left;
  right.pos = width;
  right.time = m_stop;
  right.eventIndex = m_logger.get_index(m_stop);
  gdk_cairo_set_source_color(cr, &signal_line_color);
  SearchAndPlot(cr, left, right);

  if (right.pos > m_last.pos) {
    cairo_move_to(cr, m_last.pos, yoffset + m_last.event);
    cairo_line_to(cr, right.pos, yoffset + m_last.event);
    cairo_stroke(cr);
  }
}


//========================================================================
TimeAxis::TimeAxis(Scope_Window *parent, const char *name)
  : WaveBase(parent, name),
    m_TicText(0)
{
}


//----------------------------------------
//
// TimeAxis Update
//

void TimeAxis::Update(guint64 uiStart, guint64 uiEnd)
{
  if (uiEnd == 0) {
    uiEnd = get_cycles().get();
  }

  if (m_start == uiStart && m_stop == uiEnd) {
    return;
  }

  m_start = uiStart;
  m_stop  = uiEnd;
}


void TimeAxis::Build(gint w, gint h, gint y)
{
  WaveBase::Build(w, h, y);
  // Invalidate the start time after the build.
  // This is for the case where the scope window gets opened after
  // the simulation has already been started.
  m_start = m_stop = 0;
  /*
  if (m_layout)
    pango_layout_set_text(m_layout,"Time", -1);
  */
  m_TicText = gtk_widget_create_pango_layout(GTK_WIDGET(waveDrawingArea), nullptr);
}


//------------------------------------------------------------------------
class TimeMarker : public Integer {
public:
  TimeMarker(Scope_Window *parent, const char *_name, const char *desc);
  void set(gint64 i) override;
private:
  Scope_Window *m_pParent;
};


TimeMarker::TimeMarker(Scope_Window *parent, const char *_name, const char *desc)
  : Integer(_name, 0, desc),
    m_pParent(parent)
{
  assert(m_pParent);
}


void TimeMarker::set(gint64 i)
{
  Integer::set(i);
  m_pParent->Update();
}


//========================================================================
GridPointMapping::GridPointMapping(int nPointsToMap)
  : m_nPoints(0), m_pixel(nPointsToMap), m_cycle(nPointsToMap)
{
}


GridPointMapping::~GridPointMapping()
{
}


//========================================================================
// SignalNameEntry
SignalNameEntry::SignalNameEntry()
  : m_selectedWave(nullptr)
{
  m_entry = gtk_entry_new();
}


SignalNameEntry::~SignalNameEntry()
{
  gtk_widget_destroy(m_entry);
}


bool SignalNameEntry::Select(WaveBase *pWave)
{
  if (pWave) {
    gtk_entry_set_text(GTK_ENTRY(m_entry), pWave->get_text());
    gtk_widget_show(m_entry);
    gtk_widget_grab_focus(m_entry);
    m_selectedWave = pWave;
    return true;
  }

  return unSelect();
}


bool SignalNameEntry::unSelect()
{
  gtk_widget_hide(m_entry);
  m_selectedWave = nullptr;
  return false;
}


//========================================================================
void Scope_Window::gridPoints(guint64 *uiStart, guint64 *uiEnd)
{
  guint64 start = m_Markers[eStart]->getVal();
  guint64 stop  = m_Markers[eStop]->getVal();

  if (!stop) {
    stop = get_cycles().get();
  }

  if (uiStart) {
    *uiStart = start;
  }

  if (uiEnd) {
    *uiEnd = stop;
  }

  //
  // Draw Vertical Grid Lines:
  //
  double t1 = (double) start;
  double t2 = (double) stop;
  double dt = t2 - t1;
  int iMajor = 0;
  int iMinor = 0;
  m_MajorTicks.m_nPoints = 0;
  m_MinorTicks.m_nPoints = 0;

  if (dt > 1.0) {
    double exp1 = floor(log10(dt));
    double dt_tic = pow(10.0, exp1);
    double nTics = floor(dt / dt_tic);

    if (nTics < 5.0  && exp1 > 0.0) {
      dt_tic /= 2.0;
    }

    const int nMinorTics = 5;
    const double dt_minor = dt_tic / nMinorTics;
    double ta = ceil(t1 / dt_tic);
    double tb = floor(t2 / dt_tic);

    for (double t = ta; t <= tb; t += 1.0) {
      double ttic = t * dt_tic;
      guint64 uiTime = (guint64)(floor(ttic));
      m_MajorTicks.m_pixel[iMajor] = mapTimeToPixel(uiTime);
      m_MajorTicks.m_cycle[iMajor] = uiTime;
      /*
      printf ("t=%g dt_tic=%g tmajor=%g %d %lld\n",
              t,dt_tic,ttic,m_MajorTicks.m_pixel[iMajor],uiTime);
      */
      iMajor++;
      ttic += dt_minor;

      for (int it = 1; it < nMinorTics; it++, ttic += dt_minor) {
        uiTime = (guint64)(ttic);
        m_MinorTicks.m_pixel[iMinor] = mapTimeToPixel(uiTime);
        m_MinorTicks.m_cycle[iMinor] = uiTime;
        /*
        printf ("     tminor=%g %d %lld\n",
                ttic,m_MinorTicks.m_pixel[iMinor],uiTime);
        */
        iMinor++;
      }
    }
  }

  m_MajorTicks.m_nPoints = iMajor;
  m_MinorTicks.m_nPoints = iMinor;
}


static gdouble gNormalizedHorizontalPosition = 0.0;


int Scope_Window::waveXoffset()
{
  GtkAllocation allocation;
  gtk_widget_get_allocation(m_pHpaned, &allocation);
  return (int)((m_PixmapWidth -
                (allocation.width - gtk_paned_get_position(GTK_PANED(m_pHpaned))))
               * gNormalizedHorizontalPosition);
}


// TODO: Pan GUI widget is disabled for now.
// Work must be done to do this correctly and add zoom widgets
#if 0
static void hAdjVChange(GtkAdjustment *pAdj,
                        gpointer       user_data)
{
  gdouble width = gtk_adjustment_get_upper(pAdj)
                  - gtk_adjustment_get_lower(pAdj) - gtk_adjustment_get_page_size(pAdj);
  gNormalizedHorizontalPosition = gtk_adjustment_get_value(pAdj) / (width ? width : 1.0);
}
#endif

bool Scope_Window::endSignalNameSelection(bool bAccept)
{
  gtk_widget_grab_focus(GTK_WIDGET(waveDrawingArea));
  WaveBase *pwf = m_entry->getSelected();

  if (pwf) {
    if (bAccept) {
      pwf->setSource(gtk_entry_get_text(GTK_ENTRY(m_entry->m_entry)));
    }

    m_entry->Select(0);
    return true;
  }

  return false;
}


//------------------------------------------------------------------------
bool Scope_Window::selectSignalName(int y)
{
  int timeHeight = 15;
  int waveHeight = 20;
  bool bRet = true;
  y = (y > timeHeight) ? ((y - timeHeight) / waveHeight) : -1;

  if (y >= 0 && y < int(signals.size())) {
    if (m_entry->isSelected(signals[y])) {
      return false;
    }

    m_entry->unSelect();
    gtk_layout_move(GTK_LAYOUT(signalDrawingArea), m_entry->m_entry,
                    0, signals[y]->get_yoffset() - 2);
    bRet = m_entry->Select(signals[y]);

  } else {
    bRet = endSignalNameSelection(true);
  }

  if (bRet) {
    gtk_widget_queue_draw(signalDrawingArea);
  }

  return bRet;
}


//************************************************************************
//           S I G N A L S
//************************************************************************

gint Scope_Window::signalEntryKeyPress(GtkEntry *,
                                       GdkEventKey *key,
                                       Scope_Window *sw)
{
  Dprintf(("Entry keypress 0x%x\n", key->keyval));

  if (key->keyval == GDK_KEY_Return) {
    sw->endSignalNameSelection(true);
  }

  if (key->keyval == GDK_KEY_Escape) {
    sw->endSignalNameSelection(false);
  }

  return FALSE;
}


gint Scope_Window::signalButtonPress(GtkWidget *,
                                     GdkEventButton *pEventButton,
                                     Scope_Window *sw)
{
  Dprintf((" Signal: button:%d x=%g y=%g evt=%d modifier=0x%x\n",
           pEventButton->button,
           pEventButton->x, pEventButton->y, pEventButton->type, pEventButton->state));
  sw->selectSignalName((int)(pEventButton->y));
  return TRUE;
}


gboolean Scope_Window::signal_name_expose(GtkWidget *widget,
    GdkEventExpose *, Scope_Window *sw)
{
  cairo_t *cr = gdk_cairo_create(gtk_layout_get_bin_window(GTK_LAYOUT(widget)));
  std::vector<Waveform *>::iterator i = sw->signals.begin();
  PangoLayout *layout = gtk_widget_create_pango_layout(widget, nullptr);

  for (; i != sw->signals.end(); ++i) {
    if (!sw->m_entry->isSelected(*i)) {
      Waveform *p  = *i;
      double yoffset = p->get_yoffset();
      pango_layout_set_text(layout, p->get_text(), -1);
      cairo_move_to(cr, 0.0, yoffset);
      pango_cairo_update_layout(cr, layout);
      pango_cairo_show_layout(cr, layout);
    }
  }

  g_object_unref(layout);
  cairo_destroy(cr);
  return TRUE;
}


gboolean Scope_Window::on_configure(GtkWidget *widget,
      GdkEvent *event, Scope_Window *sw)
{
   int x, y, w, h;

   x = event->configure.x;
   y = event->configure.y;
   w = event->configure.width;
   h = event->configure.height;
   //printf ( "%d, %d, %d, %d\n", x, y, w, h );
   sw->setWidth ( w );
   return FALSE;
}


void TimeAxis::draw(cairo_t *cr)
{
  // Draw major ticks
  for (int i = 0; i < sw->MajorTicks().sze(); ++i) {
    gdk_cairo_set_source_color(cr, &grid_line_color);
    int x = sw->MajorTicks().pixel(i);
    cairo_move_to(cr, x, height - 3);
    cairo_line_to(cr, x, height - 1);
    char buff[100];
    g_snprintf(buff, sizeof(buff), "%" PRINTF_GINT64_MODIFIER "d", sw->MajorTicks().cycle(i));
    pango_layout_set_text(m_TicText, buff, -1);
    int text_height;
    int text_width;
    pango_layout_get_pixel_size(m_TicText, &text_width, &text_height);
    text_width /= 2;
    x = ((x - text_width) < 0) ? 0 : (x - text_width);
    x = ((x + text_width) > width) ? (x - text_width) : x;
    cairo_move_to(cr, x, (height - text_height) / 2);
    pango_cairo_update_layout(cr, m_TicText);
    pango_cairo_show_layout(cr, m_TicText);
  }

  gdk_cairo_set_source_color(cr, &grid_line_color);

  // Draw minor ticks
  for (int i = 0; i < sw->MinorTicks().sze(); ++i) {
    double x = sw->MinorTicks().pixel(i);
    cairo_move_to(cr, x, height - 3);
    cairo_line_to(cr, x, height - 1);
  }

  // Draw horizontal grid line
  cairo_move_to(cr, 0.0, height - 1);
  cairo_line_to(cr, width, height - 1);
  cairo_stroke(cr);
}


gboolean Scope_Window::signal_expose(GtkWidget *widget,
                                     GdkEventExpose *, Scope_Window *sw)
{
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  sw->m_TimeAxis->draw(cr);
  std::vector<Waveform *>::iterator i = sw->signals.begin();

  for (; i != sw->signals.end(); ++i) {
    (*i)->draw(cr);
  }

  double xpos = sw->mapTimeToPixel(sw->m_Markers[eLeftButton]->getVal()
                                   + sw->waveXoffset());
  cairo_move_to(cr, xpos, 0.0);
  cairo_line_to(cr, xpos, 1000.0);
  cairo_stroke(cr);
  cairo_destroy(cr);
  return TRUE;
}


//------------------------------------------------------------------------
//
// Scope_Window member functions
//
//
//         ------------------------------------------
//         |                  Window
//         |   --------------------------------------
//         |   |            Horizontal pane
//         |   |  ---------------+  +------------------------+
//         |   |  | signal name  |  |    Wave layout
//         |   |  | drawing area |  |  +---------------------
//         |   |  |
//
//------------------------------------------------------------------------

void Scope_Window::Build()
{
  gtk_window_set_title(GTK_WINDOW(window), "Scope");
  //
  // Define the drawing colors
  //
  // The signal color is bright red
  signal_line_color.red = 0xff00;
  signal_line_color.green = 0x0000;
  signal_line_color.blue = 0x0000;
  // The grid color is bright green
  grid_line_color.red = 0x4000;
  grid_line_color.green = 0x4000;
  grid_line_color.blue = 0x4000;
  waveDrawingArea = gtk_drawing_area_new();
  gint dawidth  = 400;
  gint daheight = 100;
  gtk_widget_set_size_request(waveDrawingArea, dawidth, daheight);
  gtk_widget_set_events(waveDrawingArea,
                        GDK_EXPOSURE_MASK |
                        GDK_KEY_PRESS_MASK);
  signalDrawingArea = gtk_layout_new(nullptr, nullptr);
  gtk_widget_set_size_request(signalDrawingArea, 100, daheight);
  gtk_widget_set_events(signalDrawingArea,
                        GDK_EXPOSURE_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_KEY_PRESS_MASK);
  GtkWidget *pvbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), pvbox);
  m_pHpaned = gtk_hpaned_new();
  gtk_box_pack_start(GTK_BOX(pvbox), m_pHpaned, TRUE, TRUE, 0);
  m_hAdj = gtk_adjustment_new
           (0.0,    // value
            0.0,    // lower
            m_PixmapWidth,      // upper
            m_PixmapWidth / 100.0, // step_increment
            m_PixmapWidth / 10.0, // page_increment
            m_PixmapWidth / 5.0); // page_size
  m_phScrollBar = gtk_hscrollbar_new(GTK_ADJUSTMENT(m_hAdj));
  // TODO: Pan GUI widget is disabled for now.
  // Work must be done to do this correctly and add zoom widgets
#if 0
  gtk_box_pack_start(GTK_BOX(pvbox), m_phScrollBar, TRUE, TRUE, 0);
  g_signal_connect(m_hAdj, "value-changed",
                   G_CALLBACK(hAdjVChange), this);
#endif
  // Add the drawing areas to theUnread panes
  gtk_paned_add1(GTK_PANED(m_pHpaned), signalDrawingArea);
  gtk_paned_add2(GTK_PANED(m_pHpaned), waveDrawingArea);
  gtk_paned_set_position(GTK_PANED(m_pHpaned), 50);
  guint64 start, stop;
  gridPoints(&start, &stop);
  int timeHeight = 15;
  m_TimeAxis->Build(m_PixmapWidth, timeHeight, 0);
  m_TimeAxis->Update(start, stop);
  std::vector<Waveform *>::iterator i = signals.begin();
  int yoffset = timeHeight;

  for (; i != signals.end(); ++i) {
    const int waveHeight = 20;
    yoffset += waveHeight;
    (*i)->Build(m_PixmapWidth, waveHeight, yoffset);
  }

  g_signal_connect(waveDrawingArea, "expose-event", G_CALLBACK(signal_expose),
                   this);
  g_signal_connect(signalDrawingArea, "expose-event",
                   G_CALLBACK(signal_name_expose), this);
  /* Add a signal handler for key press events. This will capture
   * key commands for single stepping, running, etc.
   */
  g_signal_connect(waveDrawingArea,
                   "key_press_event",
                   G_CALLBACK(key_press),
                   (gpointer) this);
  gtk_widget_set_can_focus(waveDrawingArea, TRUE);
  g_signal_connect(signalDrawingArea,
                   "button_press_event",
                   G_CALLBACK(signalButtonPress),
                   (gpointer) this);
  bIsBuilt = true;
  UpdateMenuItem();
  m_entry = new SignalNameEntry();
  gtk_layout_put(GTK_LAYOUT(signalDrawingArea),
                 m_entry->m_entry,
                 0, 0);
  g_signal_connect(m_entry->m_entry,
                   "key_press_event",
                   G_CALLBACK(signalEntryKeyPress),
                   (gpointer) this);
  g_signal_connect(waveDrawingArea,
                   "configure-event",
                   G_CALLBACK(on_configure),
                   (gpointer) this);
  gtk_widget_show_all(window);
}


void Scope_Window::Update()
{
  if (!enabled) {
    return;
  }

  if (!bIsBuilt) {
    Build();
  }

  if (m_bFrozen) {
    return;
  }

  // Compute the grid points
  guint64 start, stop;
  gridPoints(&start, &stop);
  // Horizontal scroll
  // the scroll bar maps pixel positions into time.
  // the span of the scroll bar is the span of time that is currently
  //  cached (or soon to be cached) in the waveform pixmaps.
  // the thumb position is the current view into the cache.
  // the page-size property is 20% of the span
  double dspan = stop - start;
  dspan = (dspan < m_PixmapWidth) ? m_PixmapWidth : dspan;
  g_object_set(G_OBJECT(m_hAdj),
               "page-size", m_PixmapWidth * 200.0 / dspan,
               nullptr);
  gtk_widget_queue_draw(signalDrawingArea);
  gtk_widget_queue_draw(waveDrawingArea);
  m_TimeAxis->Update(start, stop);
  std::vector<Waveform *>::iterator i = signals.begin();

  for (; i != signals.end(); ++i) {
    (*i)->Update(start, stop);
  }

  if (m_entry->isSelected(0)) {
    gtk_widget_hide(m_entry->m_entry);
  }
}


/**
 * Adjust the internal representation to account for a change of the
 * window size on screen
 * @bug something more needed - this adjusts the time axis nicely but not
 *      the background or the traces
 *
 * @param   w       New width of the window
 */
void Scope_Window::setWidth ( int w )
{
    m_PixmapWidth = w;
    std::vector<Waveform *>::iterator i;

    m_TimeAxis->setWidth(w);
    for (i = signals.begin(); i != signals.end(); ++i) {
        (*i)->setWidth(w);
    }
    Update();
}


Scope_Window::Scope_Window(GUI_Processor *_gp)
  : GUI_Object("scope"),
    m_pHpaned(nullptr), m_phScrollBar(nullptr), m_PixmapWidth(1024),
    m_MajorTicks(32), m_MinorTicks(256),
    m_entry(nullptr)
{
  gp = _gp;
  menu = "/menu/Windows/Scope";
  m_Markers[eStart] = new TimeMarker(this, "scope.start", "Scope window start time");
  m_Markers[eStop]  = new TimeMarker(this, "scope.stop",  "Scope window stop time");
  m_Markers[eLeftButton]  = new TimeMarker(this, "scope.left",  "Scope window left marker");
  m_Markers[eRightButton] = new TimeMarker(this, "scope.right", "Scope window right marker");
  m_zoom = new ZoomAttribute(this);
  m_pan  = new PanAttribute(this);
  globalSymbolTable().addSymbol(m_Markers[eStart]);
  globalSymbolTable().addSymbol(m_Markers[eStop]);
  globalSymbolTable().addSymbol(m_Markers[eLeftButton]);
  globalSymbolTable().addSymbol(m_Markers[eRightButton]);
  globalSymbolTable().addSymbol(m_zoom);
  globalSymbolTable().addSymbol(m_pan);
  m_bFrozen = false;
  //
  // Create the signals for the scope window.
  //
  signals.push_back(new Waveform(this, "scope.ch0"));
  signals.push_back(new Waveform(this, "scope.ch1"));
  signals.push_back(new Waveform(this, "scope.ch2"));
  signals.push_back(new Waveform(this, "scope.ch3"));
  signals.push_back(new Waveform(this, "scope.ch4"));
  signals.push_back(new Waveform(this, "scope.ch5"));
  signals.push_back(new Waveform(this, "scope.ch6"));
  signals.push_back(new Waveform(this, "scope.ch7"));
  m_TimeAxis = new TimeAxis(this, "scope.time");

  if (enabled) {
    Build();
  }
}


// zoom(i)
//
const int minSpan = 10;
void Scope_Window::zoom(int i)
{
  m_bFrozen = true;
  gint64 start = (gint64) m_Markers[eStart]->getVal();
  gint64 stop  = (gint64) m_Markers[eStop]->getVal();
  gint64 now  = (gint64) get_cycles().get();

  if (!stop) {
    stop = now;
  }

  gint64 mid  = (start + stop) / 2;
  gint64 span = (stop - start) / 2;

  if (i > 0) {
    span /= i;

  } else {
    span *= -i;
  }

  span = span < minSpan ? minSpan : span;
  start = mid - span;
  stop  = mid + span;

  if (start > stop) {
    start = mid - 1;
    stop = mid + 1;
  }

  start = (start < 0) ? 0 : start;
  stop  = (stop >= now) ? 0 : stop;
  m_Markers[eStart]->set(start);
  m_Markers[eStop]->set(stop);
  m_bFrozen = false;
  Update();
}


void Scope_Window::pan(int i)
{
  gint64 start = i + (gint64) m_Markers[eStart]->getVal();
  gint64 stop  = (gint64) m_Markers[eStop]->getVal();
  gint64 now  = (gint64) get_cycles().get();

  if (start < 0 || !stop || ((stop + i) > now)) {
    return;
  }

  m_Markers[eStart]->set(start);
  m_Markers[eStop]->set(stop + i);
}


gdouble Scope_Window::getSpan()
{
  guint64 start = m_Markers[eStart]->getVal();
  guint64 stop  = m_Markers[eStop]->getVal();
  stop = stop ? stop : get_cycles().get();
  return start > stop ? 0.0 : ((gdouble)(stop - start));
}


/// mapTimeToPixel - convert time to a pixel horizontal offset.
int Scope_Window::mapTimeToPixel(guint64 time)
{
  gdouble span = getSpan();
  guint64 start = m_Markers[eStart]->getVal();
  return (int)((time > start && time <= (start + span)) ? ((time - start) * m_PixmapWidth) / span : 0);
}

#endif //HAVE_GUI
