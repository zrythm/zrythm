/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdbool.h>
#include <stdlib.h>

#include "gui/widgets/custom_button.h"
#include "utils/cairo.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

static void
init (CustomButtonWidget * self)
{
  gdk_rgba_parse (
    &self->def_color, UI_COLOR_BUTTON_NORMAL);
  gdk_rgba_parse (
    &self->hovered_color, UI_COLOR_BUTTON_HOVER);
  self->toggled_color = UI_COLORS->bright_orange;
  self->held_color = UI_COLORS->bright_orange;
  self->aspect = 1.0;
  self->corner_radius = 1.6;
}

/**
 * Updates the drawing caches.
 */
/*static void*/
/*update_caches (*/
  /*CustomButtonWidget * self)*/
/*{*/
/*}*/

/**
 * Creates a new track widget from the given track.
 */
CustomButtonWidget *
custom_button_widget_new (
  const char * icon_name,
  int          size)
{
  CustomButtonWidget * self =
    calloc (1, sizeof (CustomButtonWidget));

  strcpy (self->icon_name, icon_name);
  self->size = size;
  init (self);

  self->icon_surface =
    z_cairo_get_surface_from_icon_name (
      self->icon_name, self->size - 2, 0);
  g_return_val_if_fail (self->icon_surface, NULL);

  return self;
}

static void
get_color_for_state (
  CustomButtonWidget *    self,
  CustomButtonWidgetState state,
  GdkRGBA *               c)
{
  (void) c;
  switch (state)
    {
    case CUSTOM_BUTTON_WIDGET_STATE_NORMAL:
      *c = self->def_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_HOVERED:
      *c = self->hovered_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_ACTIVE:
      *c = self->held_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_TOGGLED:
      *c = self->toggled_color;
      break;
    default:
      g_warn_if_reached ();
    }
}

static void
draw_bg (
  CustomButtonWidget * self,
  cairo_t *            cr,
  double               x,
  double               y,
  double               width,
  int                  draw_frame,
  CustomButtonWidgetState    state)
{
  if (draw_frame)
    {
      cairo_set_source_rgba (
        cr, 1, 1, 1, 0.4);
      cairo_set_line_width (cr, 0.5);
      z_cairo_rounded_rectangle (
        cr, x, y, self->size, self->size,
        self->aspect, self->corner_radius);
      cairo_stroke (cr);
    }

  /* draw bg with fade from last state */
  GdkRGBA c;
  get_color_for_state (self, state, &c);
  if (self->last_state != state)
    {
      self->transition_frames =
        CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES;
    }

  /* draw transition if transition frames exist */
  if (self->transition_frames)
    {
      GdkRGBA mid_c;
      ui_get_mid_color (
        &mid_c, &c, &self->last_color,
        1.0 -
        (double) self->transition_frames /
          (double) CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES);
      c = mid_c;
      self->transition_frames--;
    }
  gdk_cairo_set_source_rgba (cr, &c);
  self->last_color = c;

  z_cairo_rounded_rectangle (
    cr, x, y, width, self->size, self->aspect,
    self->corner_radius);
  cairo_fill (cr);
}

static void
draw_icon_with_shadow (
  CustomButtonWidget * self,
  cairo_t *            cr,
  double               x,
  double               y,
  CustomButtonWidgetState    state)
{
  /* show icon with shadow */
  cairo_set_source_rgba (
    cr, 0, 0, 0, 0.4);
  cairo_mask_surface (
    cr, self->icon_surface, x + 1, y + 1);
  cairo_fill (cr);

  /* add main icon */
  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
  cairo_set_source_surface (
    cr, self->icon_surface, x + 1, y + 1);
  cairo_paint (cr);
}

void
custom_button_widget_draw (
  CustomButtonWidget * self,
  cairo_t *            cr,
  double               x,
  double               y,
  CustomButtonWidgetState    state)
{
  draw_bg (self, cr, x, y, self->size, false, state);

  draw_icon_with_shadow (self, cr, x, y, state);

  self->last_state = state;
}

/**
 * @param width Max width for the button to use.
 */
void
custom_button_widget_draw_with_text (
  CustomButtonWidget * self,
  cairo_t *            cr,
  double               x,
  double               y,
  double               width,
  CustomButtonWidgetState    state)
{
  draw_bg (self, cr, x, y, width, 0, state);

  draw_icon_with_shadow (self, cr, x, y, state);

  /* draw text */
  cairo_set_source_rgba (
    cr, 1, 1, 1, 1);
  cairo_move_to (
    cr, x + self->size + 2,
    (y + self->size / 2) - self->text_height / 2);
  PangoLayout * layout = self->layout;
  pango_layout_set_text (
    layout, self->text, -1);
  pango_cairo_show_layout (cr, layout);

  self->width = (int) width;
  self->last_state = state;
}

/**
 * Sets the text and layout to draw the text width.
 *
 * @param font_descr Font description to set the
 *   pango layout font to.
 */
void
custom_button_widget_set_text (
  CustomButtonWidget * self,
  PangoLayout *        layout,
  char *               text,
  const char *         font_descr)
{
  g_return_if_fail (text && layout);

  self->text = g_strdup (text);
  self->layout = pango_layout_copy (layout);
  PangoFontDescription * desc =
    pango_font_description_from_string (font_descr);
  pango_layout_set_font_description (
    self->layout, desc);
  pango_font_description_free (desc);
  pango_layout_get_pixel_size (
    self->layout, NULL, &self->text_height);
}

void
custom_button_widget_free (
  CustomButtonWidget * self)
{
  if (self->text)
    g_free (self->text);
  if (self->layout)
    g_object_unref (self->layout);

  free (self);
}
