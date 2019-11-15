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

#include "gui/widgets/custom_button.h"
#include "utils/cairo.h"
#include "utils/ui.h"

static void
init (CustomButtonWidget * self)
{
  gdk_rgba_parse (
    &self->def_color, UI_COLOR_BUTTON_NORMAL);
  gdk_rgba_parse (
    &self->hovered_color, UI_COLOR_BUTTON_HOVER);
  gdk_rgba_parse (
    &self->toggled_color, UI_COLOR_BUTTON_TOGGLED);
  gdk_rgba_parse (
    &self->held_color, UI_COLOR_BUTTON_ACTIVE);
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

void
custom_button_widget_draw (
  CustomButtonWidget * self,
  cairo_t *            cr,
  double               x,
  double               y,
  CustomButtonWidgetState    state)
{
  /* draw border */
  /*cairo_set_source_rgba (*/
    /*cr, 1, 1, 1, 0.4);*/
  /*cairo_set_line_width (cr, 0.5);*/
  /*z_cairo_rounded_rectangle (*/
    /*cr, x, y, self->size, self->size, self->aspect,*/
    /*self->corner_radius);*/
  /*cairo_stroke (cr);*/

  /* draw bg with fade from last state */
  GdkRGBA c;
  get_color_for_state (self, state, &c);
  if (self->last_state != state)
    {
      self->transition_frames = CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES;
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
      gdk_cairo_set_source_rgba (
        cr, &c);
      self->transition_frames--;
    }
  gdk_cairo_set_source_rgba (
    cr, &c);
  self->last_color = c;

  z_cairo_rounded_rectangle (
    cr, x, y, self->size, self->size, self->aspect,
    self->corner_radius);
  cairo_fill (cr);

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

  self->last_state = state;
}

void
custom_button_widget_free (
  CustomButtonWidget * self)
{
}
