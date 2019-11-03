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

#include "ztoolkit/ztk_widget.h"

/**
 * Inits a new ZWidget.
 *
 * To be called from inheriting structs.
 */
void
ztk_widget_init (
  ZtkWidget *       self,
  PuglRect *        rect,
  void (*update_cb) (ZtkWidget *),
  void (*draw_cb) (ZtkWidget *, cairo_t *),
  void (*free_cb) (ZtkWidget *))
{
  self->rect = *rect;
  self->update_cb = update_cb;
  self->draw_cb = draw_cb;
  self->free_cb = free_cb;
}

/**
 * Returns if the widget is hit by the given
 * coordinates.
 */
int
ztk_widget_is_hit (
  ZtkWidget * self,
  double      x,
  double      y)
{
  return
    x >= self->rect.x &&
    x <= self->rect.x + self->rect.width &&
    y >= self->rect.y &&
    y <= self->rect.y + self->rect.height;
}

/**
 * Draws the widget.
 */
void
ztk_widget_draw (
  ZtkWidget * self,
  cairo_t *   cr)
{
  self->draw_cb (self, cr);
}

/**
 * Frees the widget.
 */
void
ztk_widget_free (
  ZtkWidget * self);
