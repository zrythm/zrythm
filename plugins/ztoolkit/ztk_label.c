/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include "ztoolkit/ztk_label.h"

static void
ztk_label_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr)
{
  ZtkLabel * self = (ZtkLabel *) widget;

	// Draw label
	cairo_text_extents_t extents;
	cairo_set_font_size(cr, self->font_size);
	cairo_text_extents(cr, self->label, &extents);
	cairo_move_to(
    cr, widget->rect.x, widget->rect.y);
  ztk_color_set_for_cairo (
    &self->color, cr);
	cairo_show_text (cr, self->label);
}

static void
ztk_label_update_cb (
  ZtkWidget * widget)
{
}

/**
 * Creates a new label.
 */
ZtkLabel *
ztk_label_new (
  double       x,
  double       y,
  double       font_size,
  ZtkColor *   color,
  const char * lbl)
{
  ZtkLabel * self = calloc (1, sizeof (ZtkLabel));
  PuglRect rect = {
    x, y, 0, 0,
  };
  ztk_widget_init (
    (ZtkWidget *) self, &rect,
    ztk_label_update_cb, ztk_label_draw_cb,
    ztk_label_free);

  self->label = strdup (lbl);
  self->font_size = font_size;
  self->color = *color;

  return self;
}

void
ztk_label_free (
  ZtkWidget * widget)
{
  ZtkLabel * self = (ZtkLabel *) widget;
  if (self->label)
    free (self->label);
  free (self);
}
