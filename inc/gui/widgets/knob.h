/*
 * Copyright (C) 2019 Alexandros Theodotou
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
 * Basic knob widget, taken from Ardour.
 */

#ifndef __GUI_WIDGETS_KNOB_H__
#define __GUI_WIDGETS_KNOB_H__

#include <gtk/gtk.h>

#define KNOB_WIDGET_TYPE \
  (knob_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  KnobWidget,
  knob_widget,
  Z, KNOB_WIDGET,
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
  GtkDrawingArea        parent_instance;

  KnobType              type;

  /** Getter. */
  float (*getter)(void*);

  /** Setter. */
  void (*setter)(void*, float);

  /** Object to call get/set with. */
  void *                object;

  int                   size;  ///< size in px
  int                   hover;   ///< used to detect if hovering or not
  float                 zero;   ///<   zero point 0.0-1.0 */
  int                   arc;    ///< draw arc around the knob or not
  int                   bevel;  ///< bevel
  int                   flat;    ///< flat or 3D
  float                 min;    ///< min value (eg. 1)
  float                 max;    ///< max value (eg. 180)
  GdkRGBA              start_color;    ///< color away from zero point
  GdkRGBA              end_color;     ///< color close to zero point
  GtkGestureDrag        *drag;     ///< used for drag gesture
  double                last_x;    ///< used in gesture drag
  double                last_y;    ///< used in gesture drag

  /* ----- FOR PORTS ONLY ------- */
  /** Destination index for the destination
   * multipliers of the port. */
  int                   dest_index;

  /** Source index on the destination port. */
  int             src_index;
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
  float (*get_val)(void *),
  void (*set_val)(void *, float),
  void * object,
  KnobType type,
  Port * dest,
  float  min,
  float  max,
  int    size,
  float  zero);

#define knob_widget_new_simple(getter,setter,obj,min,max,size,zero) \
  _knob_widget_new ( \
    (float (*) (void *)) getter, \
    (void (*) (void *, float)) setter, \
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
