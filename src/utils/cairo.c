/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils/cairo.h"
#include "utils/dictionary.h"
#include "zrythm.h"

void
z_cairo_draw_selection (cairo_t * cr,
                        double    start_x,
                        double    start_y,
                        double    offset_x,
                        double    offset_y)
{
  cairo_set_source_rgba (cr, 0.9, 0.9, 0.9, 1.0);
  cairo_rectangle (
    cr, start_x, start_y, offset_x, offset_y);
  cairo_stroke (cr);
  cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 0.3);
  cairo_rectangle (
    cr, start_x, start_y, offset_x, offset_y);
  cairo_fill (cr);
}

void
z_cairo_draw_horizontal_line (
  cairo_t * cr,
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
z_cairo_draw_vertical_line (
  cairo_t * cr,
  double    x,
  double    from_y,
  double    to_y)
{
  cairo_move_to (cr, x, from_y);
  cairo_line_to (cr, x, to_y);
  cairo_stroke (cr);
}

/**
 * Creates a PangoLayout with default settings.
 */
PangoLayout *
z_cairo_create_default_pango_layout (
  GtkWidget *  widget)
{
  PangoLayout * layout =
    gtk_widget_create_pango_layout (
      widget, NULL);

  PangoFontDescription * desc =
    pango_font_description_from_string (
      Z_CAIRO_FONT);
  pango_layout_set_font_description (
    layout, desc);
  pango_font_description_free (desc);

  return layout;
}

/**
 * Creates a PangoLayout to be cached in widgets
 * based on the given settings.
 */
PangoLayout *
z_cairo_create_pango_layout (
  GtkWidget *  widget,
  const char * font,
  PangoEllipsizeMode ellipsize_mode,
  int          ellipsize_padding)
{
  PangoLayout * layout =
    gtk_widget_create_pango_layout (
      widget, NULL);

  if (ellipsize_mode > PANGO_ELLIPSIZE_NONE)
    {
      pango_layout_set_width (
        layout,
        pango_units_from_double (
          MIN (
            gtk_widget_get_allocated_width (
              widget) -
            ellipsize_padding * 2, 1)));
      pango_layout_set_ellipsize (
        layout, ellipsize_mode);
    }
  PangoFontDescription * desc =
    pango_font_description_from_string (
      font);
  pango_layout_set_font_description (
    layout, desc);
  pango_font_description_free (desc);

  return layout;
}

/**
 * Gets the width of the given text in pixels
 * for the given widget.
 *
 * Assumes that the layout is already set on
 * the widget.
 *
 * @param widget The widget to derive a PangoLayout
 *   from.
 * @param text The text to draw.
 * @param width The width to fill in.
 * @param height The height to fill in.
 */
void
_z_cairo_get_text_extents_for_widget (
  GtkWidget *  widget,
  PangoLayout * layout,
  const char * text,
  int *        width,
  int *        height)
{
  pango_layout_set_markup (layout, text, -1);
  pango_layout_get_pixel_size (
    layout, width, height);
}

/**
 * Draws the given text using the given font
 * starting at the given position.
 */
void
z_cairo_draw_text_full (
  cairo_t *     cr,
  GtkWidget *   widget,
  PangoLayout * layout,
  const char *  text,
  int           start_x,
  int           start_y)
{
  cairo_translate (cr, start_x, start_y);

  pango_layout_set_markup (layout, text, -1);
  pango_cairo_show_layout (cr, layout);

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
  cairo_surface_t * surface =
    dictionary_find_simple (
      CAIRO_CACHES->icon_surface_dict,
      icon_name, cairo_surface_t);
  if (!surface)
    {
      GdkPixbuf * pixbuf =
        gtk_icon_theme_load_icon (
          gtk_icon_theme_get_default (),
          icon_name,
          size,
          0,
          NULL);
      g_return_val_if_fail (pixbuf, NULL);

      surface =
        gdk_cairo_surface_create_from_pixbuf (
          pixbuf, 0,
          NULL);
      g_object_unref (pixbuf);

      dictionary_add (
        CAIRO_CACHES->icon_surface_dict,
        icon_name, surface);
    }

  return surface;
}

CairoCaches *
z_cairo_caches_new (void)
{
  CairoCaches * self =
    calloc (1, sizeof (CairoCaches));
  self->icon_surface_dict =
    dictionary_new ();

  return self;
}
