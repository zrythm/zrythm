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

/**
 * \file
 *
 * Cairo utilities.
 */

#ifndef __UTILS_CAIRO_H__
#define __UTILS_CAIRO_H__

#include <gtk/gtk.h>

typedef struct Dictionary Dictionary;

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Caches for cairo.
 */
typedef struct CairoCaches
{
  /**
   * Icon surface dictionary:
   *   icon name: cairo_surface_t
   */
  Dictionary *  icon_surface_dict;
} CairoCaches;

#define CAIRO_CACHES (ZRYTHM->cairo_caches)

/**
 * Default font for drawing pango text.
 */
#define Z_CAIRO_FONT "Sans Bold 9"

/**
 * Padding to leave from the top/left edges when
 * drawing text.
 */
#define Z_CAIRO_TEXT_PADDING 2

void
z_cairo_draw_selection (
  cairo_t * cr,
  int       start_x,
  int       start_y,
  int       offset_x,
  int       offset_y);

void
z_cairo_draw_horizontal_line (
  cairo_t * cr,
  int       y,
  int       from_x,
  int       to_x,
  double    alpha);

void
z_cairo_draw_vertical_line (
  cairo_t * cr,
  int       x,
  int       from_y,
  int       to_y);

/**
 * @param aspect Aspect ratio.
 * @param corner_radius Corner curvature radius.
 */
void
z_cairo_rounded_rectangle (
  cairo_t * cr,
  int       x,
  int       y,
  int       width,
  int       height,
  double    aspect,
  double    corner_radius);

#define z_cairo_get_text_extents_for_widget( \
  _widget,_layout,_text,_width,_height) \
  _z_cairo_get_text_extents_for_widget ( \
    (GtkWidget *) _widget, _layout, _text, \
    _width, _height)

/**
 * Gets the width of the given text in pixels
 * for the given widget.
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
  int *        height);

/**
 * Draw text with default padding.
 */
#define z_cairo_draw_text(cr,widget,layout,text) \
  z_cairo_draw_text_full ( \
    cr, widget, layout, text, Z_CAIRO_TEXT_PADDING, \
    Z_CAIRO_TEXT_PADDING)

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
  int           start_y);

/**
 * Draws a diamond shape.
 */
void
z_cairo_diamond (
  cairo_t * cr,
  int       x,
  int       y,
  int       width,
  int       height);

/**
 * Returns a surface for the icon name.
 */
cairo_surface_t *
z_cairo_get_surface_from_icon_name (
  const char * icon_name,
  int          size,
  int          scale);

/**
 * Creates a PangoLayout to be cached in widgets
 * based on the given settings.
 */
PangoLayout *
z_cairo_create_pango_layout_from_string (
  GtkWidget *  widget,
  const char * font,
  PangoEllipsizeMode ellipsize_mode,
  int          ellipsize_padding);

/**
 * Creates a PangoLayout to be cached in widgets
 * based on the given settings.
 */
PangoLayout *
z_cairo_create_pango_layout_from_description (
  GtkWidget *  widget,
  PangoFontDescription * descr,
  PangoEllipsizeMode ellipsize_mode,
  int          ellipsize_padding);

/**
 * Creates a PangoLayout with default settings.
 */
PangoLayout *
z_cairo_create_default_pango_layout (
  GtkWidget *  widget);

/**
 * Resets a surface and cairo_t with a new surface
 * and cairo_t based on the given rectangle and
 * cairo_t.
 *
 * To be used inside draw calls of widgets that
 * use caching.
 */
void
z_cairo_reset_caches (
  cairo_t **         cr_cache,
  cairo_surface_t ** surface_cache,
  int                width,
  int                height,
  cairo_t *          new_cr);

/**
 * Wrapper using integers to prevent extra
 * calculations in cairo. According to GTK devs,
 * cairo will attempt to draw details when drawing
 * non-integer coordinates.
 */
static inline void
z_cairo_move_to (
  cairo_t * cr,
  int       x,
  int       y)
{
  cairo_move_to (cr, (double) x, (double) y);
}

static inline void
z_cairo_line_to (
  cairo_t * cr,
  int       x,
  int       y)
{
  cairo_line_to (cr, (double) x, (double) y);
}

static inline void
z_cairo_rel_line_to (
  cairo_t * cr,
  int       x,
  int       y)
{
  cairo_rel_line_to (cr, (double) x, (double) y);
}

static inline void
z_cairo_rectangle (
  cairo_t * cr,
  int       x,
  int       y,
  int       width,
  int       height)
{
  cairo_rectangle (
    cr, (double) x, (double) y,
    (double) width, (double) height);
}

static inline void
z_cairo_arc (
  cairo_t * cr,
  int       xc,
  int       yc,
  int       radius,
  int       angle1,
  int       angle2)
{
  cairo_arc (
    cr, (double) xc, (double) yc,
    (double) radius, (double) angle1,
    (double) angle2);
}

static inline void
z_cairo_translate (
  cairo_t * cr,
  int       x,
  int       y)
{
  cairo_translate (cr, (double) x, (double) y);
}

static inline void
z_cairo_set_source_surface (
  cairo_t *         cr,
  cairo_surface_t * surface,
  int               x,
  int               y)
{
  cairo_set_source_surface (
    cr, surface, (double) x, (double) y);
}

static inline void
z_cairo_set_line_width (
  cairo_t * cr,
  int       width)
{
  cairo_set_line_width (cr, (double) width);
}

CairoCaches *
z_cairo_caches_new (void);

void
z_cairo_caches_free (
  CairoCaches * self);

/**
 * @}
 */

#endif
