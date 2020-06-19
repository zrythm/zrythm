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
 * The control room backend.
 */
#ifndef __AUDIO_CONTROL_ROOM_H__
#define __AUDIO_CONTROL_ROOM_H__

#include "audio/fader.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define CONTROL_ROOM (AUDIO_ENGINE->control_room)

/**
 * The control room allows to specify how Listen will
 * work on each Channel and to set overall volume
 * after the Master Channel so you can change the
 * volume without touching the Master Fader.
 */
typedef struct ControlRoom
{
  /** Temporarily dim the output volume. */
  int        dim_output;

  /**
   * Monitor fader.
   *
   * The Master stereo out should connect to this.
   *
   * @note This needs to be serialized because some
   *   ports connect to it.
   */
  Fader *    monitor_fader;

  /**
   * The volume to set other channels to when Listen
   * is enabled on a Channel.
   */
  Fader *    listen_vol_fader;

} ControlRoom;

static const cyaml_schema_field_t
control_room_fields_schema[] =
{
  YAML_FIELD_MAPPING_PTR (
    ControlRoom, monitor_fader, fader_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
control_room_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ControlRoom, control_room_fields_schema),
};

/**
 * Inits the control room from a project.
 */
void
control_room_init_loaded (
  ControlRoom * self);

/**
 * Creates a new control room.
 */
ControlRoom *
control_room_new (void);

void
control_room_free (
  ControlRoom * self);

/**
 * Sets dim_output to on/off and notifies interested
 * parties.
 */
void
control_room_set_dim_output (
  ControlRoom * self,
  int           dim_output);

/**
 * @}
 */

#endif
