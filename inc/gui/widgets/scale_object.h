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
 * Widget for ScaleObject.
 */

#ifndef __GUI_WIDGETS_SCALE_OBJECT_H__
#define __GUI_WIDGETS_SCALE_OBJECT_H__

#include <gtk/gtk.h>

#define SCALE_OBJECT_WIDGET_TYPE \
  (scale_object_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ScaleObjectWidget,
  scale_object_widget,
  Z, SCALE_OBJECT_WIDGET,
  GtkBox);

typedef struct ScaleObjectObject ScaleObjectObject;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define SCALE_OBJECT_WIDGET_TRIANGLE_W 10

/**
 * Widget for scales inside the ScaleObjectTrack.
 */
typedef struct _ScaleObjectWidget
{
  GtkBox           parent_instance;
  GtkDrawingArea * drawing_area;
  ScaleObject *    scale_object;

  /** For double click. */
  GtkGestureMultiPress * mp;

  /** Cache pango layout TODO. */
  PangoLayout *    layout;

  /** Cache text H extents and W extents for
   * the widget. */
  int              textw;
  int              texth;
} ScaleObjectWidget;

/**
 * Creates a scale widget.
 */
ScaleObjectWidget *
scale_object_widget_new (
  ScaleObject * scale);

/**
 * @}
 */

#endif
