/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __Z_TOOLKIT_ZTK_KNOB_H__
#define __Z_TOOLKIT_ZTK_KNOB_H__

#include "ztoolkit/ztk_color.h"
#include "ztoolkit/ztk_widget.h"

/**
 * Knob widget.
 */
typedef struct ZtkKnob
{
  /** Base widget. */
  ZtkWidget         base;

  /** Getter. */
  float (*getter)(void*);

  /** Setter. */
  void (*setter)(void*, float);

  /** Object to call get/set with. */
  void *            object;

  /** Zero point 0.0-1.0. */
  float             zero;
  float             min;    ///< min value (eg. 1)
  float             max;    ///< max value (eg. 180)
  double            last_x;    ///< used in gesture drag
  double            last_y;    ///< used in gesture drag

  ZtkColor          start_color;
  ZtkColor          end_color;

} ZtkKnob;

/**
 * Creates a new knob.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 * @param dest Port destination multiplier index, if
 *   type is Port, otherwise ignored.
 */
ZtkKnob *
ztk_knob_new (
  PuglRect * rect,
  float (*get_val)(void *),
  void (*set_val)(void *, float),
  void * object,
  float  min,
  float  max,
  float  zero);

#endif
