/*
   Copyright (C) 2005
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


#include "../config.h"
#ifdef HAVE_GUI

#include <glib.h>
#include "preferences.h"

//========================================================================
// Colors (probably should be in a separate file)
GuiColors gColors;
GuiColors::GuiColors()
  : m_bInitialized(false)
{
}

void GuiColors::initialize()
{
  GdkColormap *colormap = gdk_colormap_get_system();

  gdk_color_parse("light cyan", &normal_bg_color);
  gdk_color_parse("black", &normal_fg_color);
  gdk_color_parse("blue", &item_has_changed_color);
  gdk_color_parse("red1", &breakpoint_color);
  gdk_color_parse("light gray", &alias_color);
  gdk_color_parse("black", &invalid_color);
  gdk_color_parse("cyan", &sfr_bg_color);

  gdk_colormap_alloc_color(colormap, &normal_bg_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &normal_fg_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &item_has_changed_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &breakpoint_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &alias_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &invalid_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &sfr_bg_color, FALSE, TRUE);
}

GdkColor *GuiColors::breakpoint()
{
  if (!m_bInitialized)
    initialize();
  return &breakpoint_color;
}

GdkColor *GuiColors::item_has_changed()
{
  if (!m_bInitialized)
    initialize();
  return &item_has_changed_color;
}

GdkColor *GuiColors::normal_fg()
{
  if (!m_bInitialized)
    initialize();
  return &normal_fg_color;
}

GdkColor *GuiColors::normal_bg()
{
  if (!m_bInitialized)
    initialize();
  return &normal_bg_color;
}

GdkColor *GuiColors::sfr_bg()
{
  if (!m_bInitialized)
    initialize();
  return &sfr_bg_color;
}

GdkColor *GuiColors::alias()
{
  if (!m_bInitialized)
    initialize();
  return &alias_color;
}

GdkColor *GuiColors::invalid()
{
  if (!m_bInitialized)
    initialize();
  return &invalid_color;
}

#endif //HAVE_GUI
