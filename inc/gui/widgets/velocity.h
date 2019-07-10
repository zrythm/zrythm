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
 * Velocity widget.
 */

#ifndef __GUI_WIDGETS_VELOCITY_H__
#define __GUI_WIDGETS_VELOCITY_H__

#include "audio/velocity.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define VELOCITY_WIDGET_TYPE \
  (velocity_widget_get_type ())
G_DECLARE_FINAL_TYPE (VelocityWidget,
                      velocity_widget,
                      Z,
                      VELOCITY_WIDGET,
                      GtkBox)

typedef struct _VelocityWidget
{
  GtkBox           parent_instance;

  /** The drawing area. */
  GtkDrawingArea * drawing_area;

  /** The Velocity associated with this. */
  Velocity *       velocity;

  GtkWindow *      tooltip_win;
  GtkLabel *       tooltip_label;

  /** If cursor is at resizing position. */
  int              resize;
} VelocityWidget;

/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity);

void
velocity_widget_select (
  VelocityWidget * self,
  int              select);

/**
 * Returns if the current position is for resizing.
 */
int
velocity_widget_is_resize (
  VelocityWidget * self,
  int              y);

/**
 * Updates the tooltips and shows the tooltip
 * window (when dragging) or not.
 */
void
velocity_widget_update_tooltip (
  VelocityWidget * self,
  int              show);

#endif
