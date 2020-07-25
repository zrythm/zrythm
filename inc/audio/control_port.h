/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Functions for control ports.
 */

#ifndef __AUDIO_CONTROL_PORT_H__
#define __AUDIO_CONTROL_PORT_H__

#include <stdbool.h>

typedef struct Port Port;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Converts normalized value (0.0 to 1.0) to
 * real value (eg. -10.0 to 100.0).
 *
 * @note This behaves differently from
 *   \ref port_set_control_value() and
 *   \ref port_get_control_value() and should be
 *   used in widgets.
 */
float
control_port_normalized_val_to_real (
  Port * self,
  float  normalized_val);

/**
 * Converts real value (eg. -10.0 to 100.0) to
 * normalized value (0.0 to 1.0).
 *
 * @note This behaves differently from
 *   \ref port_set_control_value() and
 *   \ref port_get_control_value() and should be
 *   used in widgets.
 */
float
control_port_real_val_to_normalized (
  Port * self,
  float  real_val);

/**
 * Returns if the control port is toggled.
 */
int
control_port_is_toggled (
  Port * self);

/**
 * Get the current real value of the control.
 *
 * TODO "normalize" parameter.
 */
float
control_port_get_val (
  Port * self);

/**
 * Updates the actual value.
 *
 * The given value is always a normalized 0.0-1.0
 * value and must be translated to the actual value
 * before setting it.
 *
 * @param automating Whether this is from an
 *   automation event. This will set Lv2Port's
 *   automating field to true, which will cause the
 *   plugin to receive a UI event for this change.
 */
void
control_port_set_val_from_normalized (
  Port * self,
  float  val,
  bool   automating);

/**
 * @}
 */

#endif
