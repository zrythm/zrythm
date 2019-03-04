/*
 * gui/widgets/velocity.h - velocity for MIDI notes
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file */

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
  GtkBox                   parent_instance;
  Velocity *               velocity;   ///< the velocity associated with this
  GtkDrawingArea *         drawing_area; ///< the drwaing area
  UiCursorState            cursor_state;
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

#endif
