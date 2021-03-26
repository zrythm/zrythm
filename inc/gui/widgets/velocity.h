/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
#include "gui/widgets/arranger_object.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define VELOCITY_WIDTH 12

/**
 * Draws the Velocity in the given cairo context in
 * relative coordinates.
 *
 * @param cr The arranger cairo context.
 * @param rect Arranger rectangle.
 */
void
velocity_draw (
  Velocity *     self,
  cairo_t *      cr,
  GdkRectangle * rect);

#if 0
typedef struct _VelocityWidget
{
  ArrangerObjectWidget parent_instance;

  /** The Velocity associated with this. */
  Velocity *       velocity;
} VelocityWidget;

/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (
  Velocity * velocity);

#endif

/**
 * @}
 */

#endif
