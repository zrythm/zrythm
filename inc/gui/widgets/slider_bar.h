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
 * A slider widget that looks like a progress bar.
 */

#ifndef __GUI_WIDGETS_SLIDER_BAR_H__
#define __GUI_WIDGETS_SLIDER_BAR_H__

#include <gtk/gtk.h>

#define SLIDER_BAR_WIDGET_TYPE \
  (slider_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SliderBarWidget,
  slider_bar_widget,
  Z,
  SLIDER_BAR_WIDGET,
  GtkDrawingArea)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_SLIDER_BAR \
  MW_RIGHT_DOCK_EDGE->control_room

typedef struct _SliderBarWidget
{
  GtkDrawingArea parent_instance;

  /** Getter. */
  float (*getter) (void *);

  /** Setter. */
  void (*setter) (void *, float);

  /** The object to call get/set with. */
  void * object;

  float min;
  float max;
  float zero;
  int   width;
  int   height;

  /** Size in px. */
  int size;

  /** Drag gesture. */
  GtkGestureDrag * drag;

  /** Used for dragging. */
  double last_y;

  /** Text to be displayed in the center. */
  char * text;

  /** Pointer to backend. */
} SliderBarWidget;

/**
 * Creates a new SliderBarWidget.
 *
 * @param width -1 if not fixed.
 * @param height -1 if not fixed.
 */
SliderBarWidget *
_slider_bar_widget_new (
  float (*get_val) (void *),
  void (*set_val) (void *, float),
  void *       object,
  float        min,
  float        max,
  int          width,
  int          height,
  float        zero,
  const char * text);

#define slider_bar_widget_new_simple( \
  getter, setter, obj, min, max, w, h, zero, text) \
  _slider_bar_widget_new ( \
    (float (*) (void *)) getter, \
    (void (*) (void *, float)) setter, \
    (void *) obj, min, max, w, h, zero, text)

/**
 * @}
 */

#endif
