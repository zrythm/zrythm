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
 * Checks if the given value is toggled.
 */
#define control_port_is_val_toggled(val) \
  (val > 0.001f)

/**
 * Returns if the control port is toggled.
 */
#define control_port_is_toggled(self) \
  (control_port_is_val_toggled (self->control))

/**
 * Gets the control value for an integer port.
 */
int
control_port_get_int (
  Port * self);

/**
 * Gets the control value for an integer port.
 */
int
control_port_get_int_from_val (
  float val);

/**
 * Returns the snapped value (eg, if toggle,
 * returns 0.f or 1.f).
 */
float
control_port_get_snapped_val (
  Port * self);

/**
 * Returns the snapped value (eg, if toggle,
 * returns 0.f or 1.f).
 */
float
control_port_get_snapped_val_from_val (
  Port * self,
  float  val);

/**
 * Get the current real value of the control.
 *
 * TODO "normalize" parameter.
 */
float
control_port_get_val (
  Port * self);

/**
 * Get the current real unsnapped value of the
 * control.
 */
float
control_port_get_unsnapped_val (
  Port * self);

/**
 * Get the default real value of the control.
 */
float
control_port_get_default_val (
  Port * self);

/**
 * Get the default real value of the control.
 */
void
control_port_set_real_val (
  Port * self,
  float  val);

/**
 * Get the default real value of the control and
 * sends UI events.
 */
void
control_port_set_real_val_w_events (
  Port * self,
  float  val);

/**
 * Wrapper over port_set_control_value() for toggles.
 */
void
control_port_set_toggled (
  Port * self,
  bool   toggled,
  bool   forward_events);

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
HOT
void
control_port_set_val_from_normalized (
  Port * self,
  float  val,
  bool   automating);

/**
 * @}
 */

#endif
