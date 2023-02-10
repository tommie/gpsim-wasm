/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004,2005
   T. Scott Dattalo

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

#ifndef GUI_PREFERENCES_H_
#define GUI_PREFERENCES_H_

#include <config.h>

#ifdef HAVE_GUI

#include <gdk/gdk.h>

//========================================================================
class GuiColor
{
public:
  GuiColor(const char *_name, const char *_description);

private:
  const char *name;
  const char *description;
  GdkColor   *color;
};

class GuiColors
{
public:
  GuiColors();

  GdkColor *breakpoint();
  GdkColor *item_has_changed();
  GdkColor *normal_fg();
  GdkColor *normal_bg();
  GdkColor *sfr_bg();
  GdkColor *alias();
  GdkColor *invalid();
  void initialize();

private:
  bool m_bInitialized;

  GdkColor breakpoint_color;
  GdkColor item_has_changed_color;
  GdkColor normal_fg_color;
  GdkColor normal_bg_color;
  GdkColor sfr_bg_color;
  GdkColor alias_color;
  GdkColor invalid_color;
};

extern GuiColors gColors;

#endif // HAVE_GUI

#endif // GUI_PREFERENCES_H_
