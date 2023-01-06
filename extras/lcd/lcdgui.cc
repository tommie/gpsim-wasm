/* lcd.c
   Copyright (C) 2000 Scott Dattalo

This is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with lcd.c; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>

#include "lcd.h"
#include "lcdfont.h"
#include "hd44780.h"

cairo_surface_t *LcdFont::create_image(LcdDisplay *lcdP, _5X8 *ch)
{
  int rows = 6 + lcdP->dots.y * lcdP->pixels.y;
  int cols = 1 + lcdP->dots.x * lcdP->pixels.x;
  cairo_surface_t *image = gdk_window_create_similar_surface(mywindow,
                           CAIRO_CONTENT_COLOR_ALPHA, cols, rows);
  cairo_t *cr = cairo_create(image);
  cairo_set_line_width(cr, 0.5);

  for (int j = 0; j < lcdP->dots.y; j++) {
    for (int i = 0; i < lcdP->dots.x; i++) {
      if (ch[0][j][i] == '.') {
        double y = 5 + lcdP->pixels.y * j;
        double x = i * lcdP->pixels.x;
        cairo_set_source_rgb(cr, double(0x11) / 255, double(0x33) / 255, double(0x11) / 255);
        cairo_rectangle(cr, x, y, lcdP->pixels.x, lcdP->pixels.y);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, double(0x66) / 255, double(0x88) / 255, double(0x66) / 255);
        cairo_stroke(cr);
      }
    }
  }

  cairo_destroy(cr);
  return image;
}


/***************************************************************
 * CreateFont
 *   Here we read the lcdfont.h file and build LCD character
 * pixmaps.
 *
 */

LcdFont::LcdFont(gint characters, GtkWidget *parent_window, LcdDisplay *lcdP)
{
  pixmaps.reserve(characters);
  mywindow = gtk_widget_get_window(parent_window);

  for (gint i = 0; i < characters; i++) {
    if (strlen(test[i][0]) < 5) {
      pixmaps.push_back(nullptr);

    } else {
      pixmaps.push_back(create_image(lcdP, &test[i]));
    }
  }
}


LcdFont::~LcdFont()
{
  for (size_t i = 0; i < pixmaps.size(); ++i) {
    if (pixmaps[i]) {
      cairo_surface_destroy(pixmaps[i]);
    }
  }
}


void LcdFont::update_pixmap(int pos, _5X8 *tempchar, LcdDisplay *lcdP)
{
  if (pixmaps[pos]) {
    cairo_surface_destroy(pixmaps[pos]);
    pixmaps[pos] = nullptr;
  }

  pixmaps[pos] = create_image(lcdP, tempchar);
}


cairo_surface_t *LcdFont::getPixMap(unsigned int index)
{
  return ((index < pixmaps.size()) && pixmaps[index]) ? pixmaps[index] : pixmaps[0];
}


void LcdDisplay::update_cgram_pixmaps()
{
  int i, j, k;
  _5X8 tempchar;

  if (fontP == nullptr) {
    return;
  }

  for (i = 0; i < CGRAM_SIZE / 8; i++) {
    for (j = 0; j < 8; j++) {
      for (k = 0; k < 5; k++) {
        if (m_hd44780->getCGRam(8 * i + j) & (1 << (4 - k))) {
          tempchar[j][k] = '.';

        } else {
          tempchar[j][k] = ' ';
        }
      }

      tempchar[j][5] = 0;
    }

    fontP->update_pixmap(i, &tempchar, this);
    fontP->update_pixmap(i + 8, &tempchar, this);
  }

  m_hd44780->setCGRamupdate(false);
}


cairo_surface_t *LcdDisplay::get_pixmap(gint row, gint col)
{
  if (m_hd44780->CGRamupdate()) {
    update_cgram_pixmaps();
  }

  return fontP ? fontP->getPixMap(m_hd44780->getDDRam(row, col)) : nullptr;
}


static gboolean lcd_expose_event(GtkWidget *widget, GdkEvent * , gpointer user_data)
{
  LcdDisplay *lcdP = static_cast<LcdDisplay *>(user_data);
  // If there is no font, then go create it.

  if (!lcdP->fontP) {
    lcdP->fontP = new LcdFont(FONT_LEN, widget, lcdP);
  }

  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  lcdP->w_width = allocation.width;
  lcdP->w_height = allocation.height;
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  lcdP->update(cr);
  cairo_destroy(cr);
  return FALSE;
}


void LcdDisplay::update(cairo_t *cr)
{
  guint i, j;
  cairo_set_source_rgb(cr,
                       double(0x78) / 255, double(0xa8) / 255, double(0x78) / 255);
  cairo_rectangle(cr, 0.0, 0.0, w_width, w_height);
  cairo_fill(cr);
  gint cw = get_char_width();
  gint ch = get_char_height();
  gint border = get_border();

  if (!(disp_type & TWO_ROWS_IN_ONE)) {
    for (j = 0; j < rows; j++)
      for (i = 0; i < cols; i++) {
        cairo_set_source_surface(cr, get_pixmap(j, i), border + i * cw, border + j * (ch));
        cairo_paint(cr);
      }

  } else {
    guint pos;

    for (pos = 0, j = 0; j < rows; j++)
      for (i = 0; i < cols; pos++, i++) {
        cairo_set_source_surface(cr, get_pixmap(j, i), border + pos * cw, border);
        cairo_paint(cr);
      }
  }
}


static gint cursor_event(GtkWidget * , GdkEvent *event, gpointer  * )
{
  if ((event->type == GDK_BUTTON_PRESS) &&
      ((event->button.button == 1) ||
       (event->button.button == 3))) {
    return TRUE;
  }

  return FALSE;
}


/**********************************************************
 *
 *
 */

void LcdDisplay::CreateGraphics()
{
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  if (window) {
    char title[128];
    g_snprintf(title, sizeof(title), "%d X %d", rows, cols);

    if (disp_type & TWO_ROWS_IN_ONE) {
      g_strlcat(title, " (in one row)", sizeof(title));
    }

    gtk_widget_realize(window);
    gtk_window_set_title(GTK_WINDOW(window), "LCD");
    GtkWidget *main_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);
    GtkWidget *vbox =
      gtk_widget_new(gtk_vbox_get_type(),
                     "GtkBox::homogeneous", FALSE,
                     //"GtkBox::spacing", 5,
                     //"GtkContainer::border_width", 10,
                     "GtkWidget::parent", main_vbox,
                     "GtkWidget::visible", TRUE,
                     nullptr);
    GtkWidget *frame =
      gtk_widget_new(gtk_frame_get_type(),
                     "GtkFrame::shadow", GTK_SHADOW_ETCHED_IN,
                     "GtkFrame::label_xalign", 0.5,
                     "GtkFrame::label", title,
                     //"GtkContainer::border_width", 10,
                     "GtkWidget::parent", vbox,
                     "GtkWidget::visible", TRUE,
                     nullptr);
    darea = gtk_drawing_area_new();

    if (!(disp_type & TWO_ROWS_IN_ONE)) {
      gtk_widget_set_size_request(darea,
                                  cols * get_char_width() + 2 * get_border(),
                                  rows * (get_char_height() + get_border()) + get_border());

    } else {
      gtk_widget_set_size_request(darea,
                                  rows * cols * get_char_width() + 2 * get_border(),
                                  get_char_height() + 2 * get_border());
    }

    gtk_container_add(GTK_CONTAINER(frame), darea);
    g_signal_connect(darea,
                     "expose_event",
                     G_CALLBACK(lcd_expose_event),
                     this);
    gtk_widget_add_events(darea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
    g_signal_connect(darea,
                     "button_press_event",
                     G_CALLBACK(cursor_event),
                     nullptr);
    gtk_widget_show_all(window);
  }
}

#endif //HAVE_GUI
