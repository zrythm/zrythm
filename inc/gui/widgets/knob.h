/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Basic knob widget, taken from Ardour.
 */

#ifndef __GUI_WIDGETS_KNOB_H__
#define __GUI_WIDGETS_KNOB_H__

#include <stdbool.h>

#include "utils/types.h"

#include <gtk/gtk.h>

#define KNOB_WIDGET_TYPE \
  (knob_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  KnobWidget, knob_widget, Z, KNOB_WIDGET,
  GtkDrawingArea)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Type of knob.
 */
typedef enum KnobType
{
  KNOB_TYPE_NORMAL,
  KNOB_TYPE_PORT_MULTIPLIER,
} KnobType;

typedef struct Port Port;

typedef struct _KnobWidget
{
  GtkDrawingArea      parent_instance;

  KnobType            type;

  /** Getter for the actual value. */
  GenericFloatGetter  getter;

  /** Getter for a snapped value (used if
   * passed). */
  GenericFloatGetter  snapped_getter;

  /** Setter. */
  GenericFloatSetter  setter;

  /** Float setter for drag begin. */
  GenericFloatSetter  init_setter;

  /** Float setter for drag end. */
  GenericFloatSetter  end_setter;

  /** Object to call get/set with. */
  void *              object;

  /** Size in px. */
  int                 size;
  /** Whether hovering or not. */
  bool                hover;
  /** Zero point 0.0-1.0. */
  float               zero;
  /** Draw arc around the knob or not. */
  bool                arc;
  /** Bevel. */
  int                 bevel;
  /** Flat or 3D. */
  bool                flat;
  /** Min value (eg, 1). */
  float               min;
  /** Max value (eg, 180). */
  float               max;

  /** Color away from zero point. */
  GdkRGBA             start_color;
  /** Color closer to zero point. */
  GdkRGBA             end_color;

  /** Used in drag gesture. */
  GtkGestureDrag *    drag;
  /** Used in gesture drag. */
  double              last_x;
  /** Used in gesture drag. */
  double              last_y;

  /* ----- FOR PORTS ONLY ------- */
  /** Destination index for the destination
   * multipliers of the port. */
  int                 dest_index;

  /** Source index on the destination port. */
  int                  src_index;

  /** Last drawn real val. */
  float                last_real_val;
} KnobWidget;

/**
 * Creates a knob widget with the given options and
 * binds it to the given value.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 * @param dest Port destination multiplier index, if
 *   type is Port, otherwise ignored.
 */
KnobWidget *
_knob_widget_new (
  GenericFloatGetter get_val,
  GenericFloatSetter set_val,
  void *             object,
  KnobType           type,
  Port *             dest,
  float              min,
  float              max,
  int                size,
  float              zero);

#define knob_widget_new_simple(getter,setter,obj,min,max,size,zero) \
  _knob_widget_new ( \
    (GenericFloatGetter) getter, \
    (GenericFloatSetter) setter, \
    (void *) obj, \
    KNOB_TYPE_NORMAL, NULL, min, max, size, zero)

#define knob_widget_new_port(_port,_dest,size) \
  _knob_widget_new ( \
    NULL, NULL, (void *) _port, \
    KNOB_TYPE_PORT_MULTIPLIER, \
    _dest, 0.f, 1.f, size, 0.f)

/**
 * @}
 */

#endif
