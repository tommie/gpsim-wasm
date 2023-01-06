/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
   T. Scott Dattalo and Ralf Forsberg

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

#ifndef GUI_GUI_SCOPE_H_
#define GUI_GUI_SCOPE_H_

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <valarray>
#include <vector>
#include "gui_object.h"

class GUI_Processor;
class TimeMarker;
class ZoomAttribute;
class PanAttribute;
class Waveform;
class SignalNameEntry;
class TimeAxis;

class GridPointMapping {
public:
  explicit GridPointMapping(int nPointsToMap);
  ~GridPointMapping();

  int     pixel(int index)
  {
    return (index < m_nPoints) ? m_pixel[index] : 0;
  }
  guint64 cycle(int index)
  {
    return (index < m_nPoints) ? m_cycle[index] : 0;
  }
  int sze()
  {
    return m_nPoints;
  }

  int      m_nPoints;
  std::valarray<int> m_pixel;
  std::valarray<guint64> m_cycle;
};


//
// The Scope Window
//

class Scope_Window : public GUI_Object {
public:
  explicit Scope_Window(GUI_Processor *gp);
  Scope_Window(const Scope_Window &) = delete;
  Scope_Window& operator =(const Scope_Window &) = delete;

  void Build() override;
  void Update() override;

  /// zoom - positive values mean zoom in, negative values mean zoom out
  void zoom(int);

  /// pan - positive values mean pan right, negative values mean pan left.
  void pan(int);

  /// getSpan - returns time span currently cached (essentially tStop-tStart).
  gdouble getSpan();

  /// mapTimeToPixel - convert time to a pixel horizontal offset.
  int mapTimeToPixel(guint64 time);

  GridPointMapping &MajorTicks()
  {
    return m_MajorTicks;
  }
  GridPointMapping &MinorTicks()
  {
    return m_MinorTicks;
  }

private:
  /// Signals for the scope window
  static gint signalButtonPress(GtkWidget *, GdkEventButton *, Scope_Window *sw);
  static gint signalEntryKeyPress(GtkEntry *, GdkEventKey *, Scope_Window *sw);
  static gboolean signal_name_expose(GtkWidget *widget,
                                     GdkEventExpose *event, Scope_Window *sw);
  static gboolean signal_expose(GtkWidget *widget,
                                GdkEventExpose *event, Scope_Window *sw);
  static gboolean key_press(GtkWidget *widget, GdkEventKey *key,
                            Scope_Window *sw);
  static gboolean on_configure(GtkWidget *widget, GdkEvent *event, Scope_Window *sw);

  /// selectSignalName - begin the signal name editing mode
  /// returns true if selection state has changed.
  bool selectSignalName(int y);

  /// endSignalNameSelection - end the signal name editing mode.
  bool endSignalNameSelection(bool bAccept);

  ///
  int waveXoffset();

  /// gridPoints - Fill the major and minor axis grid point arrays
  ///   These arrays map time values into pixel coordinates.
  void gridPoints(guint64 *start, guint64 *stop);

  /// Adjust the internal representation to account for a change of the
  /// window size on screen
  void setWidth ( int w );

  enum {
    eStart = 0,
    eStop,
    eLeftButton,
    eRightButton,
    eNumMarkers
  } MarkerLabels;

  TimeMarker *m_Markers[eNumMarkers];
  ZoomAttribute *m_zoom;
  PanAttribute  *m_pan;

  GtkWidget *m_pHpaned;     /* The signal names and waves are rendered
			       in a horizontal pane */
  GtkWidget *m_phScrollBar; // scroll bar for the waves

  int m_PixmapWidth; // Width of waveform pixmaps.

  GridPointMapping m_MajorTicks;
  GridPointMapping m_MinorTicks;

  bool m_bFrozen;

  GtkObject *m_hAdj;
  SignalNameEntry *m_entry;
  TimeAxis *m_TimeAxis;
  std::vector<Waveform *> signals;
};


#endif // GUI_GUI_SCOPE_H_
