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
  self->aspect = 10.0;
  self->corner_radius = 4.0;
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
      self->icon_name, self->size, 0);
  g_return_val_if_fail (self->icon_surface, NULL);

  return self;
}

void
custom_button_widget_draw (
  CustomButtonWidget * self,
  cairo_t *            cr,
  double               x,
  double               y,
  CustomButtonWidgetState    state)
{

}

void
custom_button_widget_free (
  CustomButtonWidget * self)
{
}
