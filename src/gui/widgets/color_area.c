/*
 * gui/widgets/color_area.c - A channel color widget
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

#include "gui/widgets/color_area.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ColorAreaWidget,
               color_area_widget,
               GTK_TYPE_DRAWING_AREA)

/**
 * Draws the color picker.
 */
static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  ColorAreaWidget * color_area = COLOR_AREA_WIDGET (widget);
  GdkRGBA * color = color_area->color;
  guint width, height;
  GtkStyleContext *context;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  /* FIXME curved doesn't work right on vertical */
  /* a custom shape that could be wrapped in a function */
  /*double x         = 0,        [> parameters like cairo_rectangle <]*/
         /*y         = 0,*/
         /*aspect        = 1.0,     [> aspect ratio <]*/
         /*corner_radius = height / 8.0;   [> and corner curvature radius <]*/

  /*double radius = corner_radius / aspect;*/
  /*double degrees = G_PI / 180.0;*/

  /*cairo_new_sub_path (cr);*/
  /*cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);*/
  /*cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);*/
  /*cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);*/
  /*cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);*/
  /*cairo_close_path (cr);*/

  /*gdk_cairo_set_source_rgba (cr, color);*/
  /*cairo_fill_preserve (cr);*/

  cairo_rectangle (cr, 0, 0, width, height);
  gdk_cairo_set_source_rgba (cr, color);
  cairo_fill (cr);

  return FALSE;
}

void
color_area_widget_set_color (ColorAreaWidget * widget,
                             GdkRGBA * color)
{
  widget->color = color;

  gtk_widget_queue_draw (GTK_WIDGET (widget));
}

static void
color_area_widget_init (ColorAreaWidget * self)
{
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
}

static void
color_area_widget_class_init (ColorAreaWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "color-area");
}
