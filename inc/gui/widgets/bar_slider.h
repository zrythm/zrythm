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
 * Custom slider widget.
 */

#ifndef __GUI_WIDGETS_BAR_SLIDER_H__
#define __GUI_WIDGETS_BAR_SLIDER_H__

#include <gtk/gtk.h>

#define BAR_SLIDER_WIDGET_TYPE \
  (bar_slider_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BarSliderWidget,
  bar_slider_widget,
  Z, BAR_SLIDER_WIDGET,
  GtkDrawingArea)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Defines how drag_update will work.
 */
typedef enum BarSliderUpdateMode
{
  /** Update the value relative from where the drag
   * was started. */
  BAR_SLIDER_UPDATE_MODE_RELATIVE,
  /** Update the value to wherever the cursor is. */
  BAR_SLIDER_UPDATE_MODE_CURSOR,
} BarSliderUpdateMode;

/**
 * Draggable slider to adjust an amount (such as a
 * percentage).
 *
 * It displays the value in the background as a
 * progress bar.
 */
typedef struct _BarSliderWidget
{
  GtkDrawingArea     parent_instance;

  /** Number of decimal points to show. */
  int                decimals;

  /** The suffix to show after the value (eg "%" for
   * percentages). */
  char *             suffix;

  /** Maximum value. */
  float              max;

  /** Minimum value. */
  float              min;

  /** Zero point. */
  float              zero;

  /** Float getter. */
  float (*getter)(void*);

  /** Float setter. */
  void (*setter)(void*, float);

  /** Widget width. */
  int             width;

  /** Widget height. */
  int             height;

  /** Object to call get/set with. */
  void *          object;

  /** Used when dragging. */
  GtkGestureDrag  *drag;

  /** Used in gesture drag. */
  double          last_x;

  /** Used in gesture drag. */
  double          start_x;

  /** Update mode. */
  BarSliderUpdateMode mode;

  /** Whether hovering or not. */
  int             hover;

} BarSliderWidget;

/**
 * Creates a bar slider widget for floats.
 */
BarSliderWidget *
_bar_slider_widget_new (
  float (*get_val)(void *),
  void (*set_val)(void *, float),
  void * object,
  float  min,
  float  max,
  int    w,
  int    h,
  float  zero,
  int    decimals,
  BarSliderUpdateMode mode,
  const char * suffix);

/**
 * Helper to create a bar slider widget.
 */
#define bar_slider_widget_new( \
  getter,setter,obj,min,max,w,h,zero,dec,mode,suffix) \
  _bar_slider_widget_new ( \
    (float (*) (void *)) getter, \
    (void (*) (void *, float)) setter, \
    (void *) obj, \
    min, max, w, h, zero, dec, mode, suffix)

/**
 * @}
 */

#endif
