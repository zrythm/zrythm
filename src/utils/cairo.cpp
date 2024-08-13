// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/pango.h"

void
z_cairo_draw_horizontal_line (
  cairo_t * cr,
  double    y,
  double    from_x,
  double    to_x,
  double    line_width,
  double    alpha)
{
  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, alpha);
  cairo_rectangle (cr, from_x, y, to_x - from_x, line_width);
  cairo_fill (cr);
}

void
z_cairo_draw_vertical_line (
  cairo_t * cr,
  double    x,
  double    from_y,
  double    to_y,
  double    line_width)
{
  cairo_rectangle (cr, x, from_y, line_width, to_y - from_y);
  cairo_fill (cr);
}

/**
 * Creates a PangoLayout with default settings.
 */
PangoLayout *
z_cairo_create_default_pango_layout (GtkWidget * widget)
{
  PangoLayout * layout = gtk_widget_create_pango_layout (widget, nullptr);

  PangoFontDescription * desc =
    pango_font_description_from_string (Z_CAIRO_FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  return layout;
}

/**
 * Creates a PangoLayout to be cached in widgets
 * based on the given settings.
 */
PangoLayoutUniquePtr
z_cairo_create_pango_layout_from_description (
  GtkWidget *            widget,
  PangoFontDescription * descr,
  PangoEllipsizeMode     ellipsize_mode,
  int                    ellipsize_padding)
{
  auto layout = z_pango_create_layout_from_description (widget, descr);

  if (ellipsize_mode > PANGO_ELLIPSIZE_NONE)
    {
      pango_layout_set_width (
        layout.get (),
        pango_units_from_double (
          MAX (gtk_widget_get_width (widget) - ellipsize_padding * 2, 1)));
      pango_layout_set_ellipsize (layout.get (), ellipsize_mode);
    }

  return layout;
}

/**
 * Creates a PangoLayout to be cached in widgets
 * based on the given settings.
 */
PangoLayoutUniquePtr
z_cairo_create_pango_layout_from_string (
  GtkWidget *        widget,
  const char *       font,
  PangoEllipsizeMode ellipsize_mode,
  int                ellipsize_padding)
{
  PangoFontDescription * desc = pango_font_description_from_string (font);
  auto                   layout = z_cairo_create_pango_layout_from_description (
    widget, desc, ellipsize_mode, ellipsize_padding);
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
  GtkWidget *   widget,
  PangoLayout * layout,
  const char *  text,
  int *         width,
  int *         height)
{
  z_return_if_fail (layout && widget && text);
  pango_layout_set_markup (layout, text, -1);
  pango_layout_get_pixel_size (layout, width, height);
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
  z_return_if_fail (cr && layout && widget && text);
  cairo_translate (cr, start_x, start_y);

  pango_layout_set_markup (layout, text, -1);
  pango_cairo_show_layout (cr, layout);

  cairo_translate (cr, -start_x, -start_y);
}

void
z_cairo_set_source_color (cairo_t * cr, Color color)
{
  auto color_rgba = color.to_gdk_rgba ();
  gdk_cairo_set_source_rgba (cr, &color_rgba);
}