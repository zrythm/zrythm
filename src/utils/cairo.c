/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/cairo.h"

void
z_cairo_draw_selection (cairo_t * cr,
                        double    start_x,
                        double    start_y,
                        double    offset_x,
                        double    offset_y)
{
  cairo_set_source_rgba (cr, 0.9, 0.9, 0.9, 1.0);
  cairo_rectangle (cr,
                   start_x,
                   start_y,
                   offset_x,
                   offset_y);
  cairo_stroke (cr);
  cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 0.3);
  cairo_rectangle (cr,
                   start_x,
                   start_y,
                   offset_x,
                   offset_y);
  cairo_fill (cr);
}

void
z_cairo_draw_horizontal_line (cairo_t * cr,
                              double    y,
                              double    from_x,
                              double    to_x,
                              double    alpha)
{
  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, alpha);
  cairo_set_line_width (cr, 0.5);
  cairo_move_to (cr, from_x, y);
  cairo_line_to (cr, to_x, y);
  cairo_stroke (cr);
}

void
z_cairo_draw_vertical_line (cairo_t * cr,
                            double    x,
                            double    from_y,
                            double    to_y)
{
  cairo_move_to (cr, x, from_y);
  cairo_line_to (cr, x, to_y);
  cairo_stroke (cr);
}

/**
 * Gets the width of the given text in pixels
 * for the given widget when z_cairo_draw_text()
 * is used.
 *
 * @param widget The widget to derive a PangoLayout
 *   from.
 * @param text The text to draw.
 * @param width The width to fill in.
 * @param height The height to fill in.
 */
void
z_cairo_get_text_extents_for_widget (
  GtkWidget *  widget,
  const char * text,
  int *        width,
  int *        height)
{
  PangoLayout *layout;
  PangoFontDescription *desc;

  layout =
    gtk_widget_create_pango_layout (
      widget, text);

  pango_layout_set_markup (layout, text, -1);
  desc =
    pango_font_description_from_string (
      Z_CAIRO_FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_get_pixel_size (
    layout, width, height);

  g_object_unref (layout);
}
