/*
 * gui/widgets/ruler.cpp - The ruler on top of the timeline
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>

#define DEFAULT_ZOOM_LEVEL 1.0

typedef struct
{
    GtkDrawingArea * parent_widget;
    gint       zoom_level;
} _RulerPrivate;


static gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  gfloat zoom_level = 1.00f;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);


  for (int i = 0; i < width * 2; i++)
  {
      if (i % 100 == 0)
      {
          cairo_set_source_rgb (cr, 1, 1, 1);
          cairo_set_line_width (cr, 1);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height / 2);
          cairo_stroke (cr);
      }
      else if (i % 25 == 0)
      {
          cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height / 4);
          cairo_stroke (cr);
      }
  }

  /*cairo_arc (cr,*/
             /*width / 2.0, height / 2.0,*/
             /*MIN (width, height) / 2.0,*/
             /*0, 2 * G_PI);*/

  /*gtk_style_context_get_color (context,*/
                               /*gtk_style_context_get_state (context),*/
                               /*&color);*/
  /*gdk_cairo_set_source_rgba (cr, &color);*/

  /*cairo_fill (cr);*/

 return FALSE;
}

/**
 * adds callbacks to the drawing area given
 */
void
set_ruler (GtkWidget * drawing_area)
{
    g_signal_connect (G_OBJECT (drawing_area), "draw",
                      G_CALLBACK (draw_callback), NULL);
}
