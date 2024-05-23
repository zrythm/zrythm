// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "utils/cairo.h"
#include "utils/dictionary.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/pango.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#if 0
void
z_cairo_draw_selection_with_color (
  cairo_t * cr,
  GdkRGBA * color,
  double    start_x,
  double    start_y,
  double    offset_x,
  double    offset_y)
{
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (
    cr, color->red, color->green, color->blue,
    color->alpha);
  cairo_rectangle (
    cr, start_x, start_y, offset_x, offset_y);
  cairo_stroke (cr);
  cairo_set_source_rgba (
    cr, color->red / 3.0, color->green / 3.0,
    color->blue / 3.0, color->alpha / 3.0);
  cairo_rectangle (
    cr, start_x, start_y, offset_x, offset_y);
  cairo_fill (cr);
}

void
z_cairo_draw_selection (
  cairo_t * cr,
  double    start_x,
  double    start_y,
  double    offset_x,
  double    offset_y)
{
  GdkRGBA color =
    Z_GDK_RGBA_INIT (0.9, 0.9, 0.9, 1.0);
  z_cairo_draw_selection_with_color (
    cr, &color, start_x, start_y, offset_x,
    offset_y);
}
#endif

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
  PangoLayout * layout = gtk_widget_create_pango_layout (widget, NULL);

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
PangoLayout *
z_cairo_create_pango_layout_from_description (
  GtkWidget *            widget,
  PangoFontDescription * descr,
  PangoEllipsizeMode     ellipsize_mode,
  int                    ellipsize_padding)
{
  PangoLayout * layout = z_pango_create_layout_from_description (widget, descr);

  if (ellipsize_mode > PANGO_ELLIPSIZE_NONE)
    {
      pango_layout_set_width (
        layout,
        pango_units_from_double (
          MAX (gtk_widget_get_width (widget) - ellipsize_padding * 2, 1)));
      pango_layout_set_ellipsize (layout, ellipsize_mode);
    }

  return layout;
}

/**
 * Creates a PangoLayout to be cached in widgets
 * based on the given settings.
 */
PangoLayout *
z_cairo_create_pango_layout_from_string (
  GtkWidget *        widget,
  const char *       font,
  PangoEllipsizeMode ellipsize_mode,
  int                ellipsize_padding)
{
  PangoFontDescription * desc = pango_font_description_from_string (font);
  PangoLayout *          layout = z_cairo_create_pango_layout_from_description (
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
  g_return_if_fail (layout && widget && text);
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
  g_return_if_fail (cr && layout && widget && text);
  cairo_translate (cr, start_x, start_y);

  pango_layout_set_markup (layout, text, -1);
  pango_cairo_show_layout (cr, layout);

  cairo_translate (cr, -start_x, -start_y);
}

/**
 * Returns a surface for the icon name.
 */
cairo_surface_t *
z_cairo_get_surface_from_icon_name (const char * icon_name, int size, int scale)
{
  g_return_val_if_fail (icon_name, NULL);
  cairo_surface_t * surface = dictionary_find_simple (
    CAIRO_CACHES->icon_surface_dict, icon_name, cairo_surface_t);
  if (!surface)
    {
      GdkTexture * texture =
        z_gdk_texture_new_from_icon_name (icon_name, size, size, scale);
      if (!texture)
        {
          g_message ("failed to load texture from icon %s", icon_name);

          /* return a warning icon if not found */
          if (string_is_equal (icon_name, "data-warning"))
            {
              return NULL;
            }
          else
            {
              return z_cairo_get_surface_from_icon_name (
                "data-warning", size, scale);
            }
        }

      surface = cairo_image_surface_create (
        CAIRO_FORMAT_ARGB32, gdk_texture_get_width (texture),
        gdk_texture_get_height (texture));
      gdk_texture_download (
        texture, cairo_image_surface_get_data (surface),
        (gsize) cairo_image_surface_get_stride (surface));
      cairo_surface_mark_dirty (surface);

      g_object_unref (texture);

      dictionary_add (CAIRO_CACHES->icon_surface_dict, icon_name, surface);
    }

  return surface;
}

/**
 * Resets a surface and cairo_t with a new surface
 * and cairo_t based on the given rectangle and
 * cairo_t.
 *
 * To be used inside draw calls of widgets that
 * use caching.
 *
 * @param width New surface width.
 * @param height New surface height.
 */
void
z_cairo_reset_caches (
  cairo_t **         cr_cache,
  cairo_surface_t ** surface_cache,
  int                width,
  int                height,
  cairo_t *          new_cr)
{
  g_return_if_fail (width < 8000 && height < 8000);

  if (*surface_cache)
    {
      cairo_surface_destroy (*surface_cache);
    }
  if (*cr_cache)
    {
      cairo_destroy (*cr_cache);
    }

  *surface_cache = cairo_surface_create_similar (
    cairo_get_target (new_cr), CAIRO_CONTENT_COLOR_ALPHA, width, height);
  *cr_cache = cairo_create (*surface_cache);
}

CairoCaches *
z_cairo_caches_new (void)
{
  CairoCaches * self = object_new (CairoCaches);

  self->icon_surface_dict = dictionary_new ();

  return self;
}

void
z_cairo_caches_free (CairoCaches * self)
{
  object_free_w_func_and_null (dictionary_free, self->icon_surface_dict);

  object_zero_and_free (self);
}
