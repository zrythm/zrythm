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
  z_cairo_get_text_extents_for_widget_full (
    widget, text, width, height, Z_CAIRO_FONT);
}

/**
 * Gets the width of the given text in pixels
 * for the given widget when using the given font
 * settings.
 *
 * @param widget The widget to derive a PangoLayout
 *   from.
 * @param text The text to draw.
 * @param width The width to fill in.
 * @param height The height to fill in.
 */
void
z_cairo_get_text_extents_for_widget_full (
  GtkWidget *  widget,
  const char * text,
  int *        width,
  int *        height,
  const char * font)
{
  PangoLayout *layout;
  PangoFontDescription *desc;

  layout =
    gtk_widget_create_pango_layout (
      widget, text);

  pango_layout_set_markup (layout, text, -1);
  desc =
    pango_font_description_from_string (
      font);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_get_pixel_size (
    layout, width, height);

  g_object_unref (layout);
}

void
z_cairo_draw_text_full (
  cairo_t *    cr,
  const char * text,
  int          start_x,
  int          start_y,
  const char * font)
{

  PangoLayout *layout;
  PangoFontDescription *desc;

  cairo_translate (cr, start_x, start_y);

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  pango_layout_set_markup (layout, text, -1);
  desc =
    pango_font_description_from_string (font);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_cairo_show_layout (cr, layout);

  /* free the layout object */
  g_object_unref (layout);

  cairo_translate (cr, - start_x, - start_y);
}

/**
 * Returns a surface for the icon name.
 */
cairo_surface_t *
z_cairo_get_surface_from_icon_name (
  const char * icon_name,
  int          size,
  int          scale)
{
  GdkPixbuf * pixbuf =
    gtk_icon_theme_load_icon (
      gtk_icon_theme_get_default (),
      icon_name,
      size,
      0,
      NULL);
  g_return_val_if_fail (pixbuf, NULL);

  cairo_surface_t * surface =
    gdk_cairo_surface_create_from_pixbuf (
      pixbuf, 0,
      NULL);

  g_object_unref (pixbuf);

  return surface;
}
