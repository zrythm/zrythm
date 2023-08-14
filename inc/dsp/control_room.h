// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * The control room backend.
 */
#ifndef __AUDIO_CONTROL_ROOM_H__
#define __AUDIO_CONTROL_ROOM_H__

#include "dsp/fader.h"

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define CONTROL_ROOM_SCHEMA_VERSION 2

#define CONTROL_ROOM (AUDIO_ENGINE->control_room)

#define control_room_is_in_active_project(self) \
  (self->audio_engine \
   && engine_is_in_active_project (self->audio_engine))

/**
 * The control room allows to specify how Listen will
 * work on each Channel and to set overall volume
 * after the Master Channel so you can change the
 * volume without touching the Master Fader.
 */
typedef struct ControlRoom
{
  int schema_version;

  /**
   * The volume to set muted channels to when
   * soloing/muting.
   */
  Fader * mute_fader;

  /**
   * The volume to set listened channels to when
   * Listen is enabled on a Channel.
   */
  Fader * listen_fader;

  /**
   * The volume to set other channels to when Listen
   * is enabled on a Channel, or the monitor when
   * dim is enabled.
   */
  Fader * dim_fader;

  /** Dim the output volume. */
  bool dim_output;

  /**
   * Monitor fader.
   *
   * The Master stereo out should connect to this.
   *
   * @note This needs to be serialized because some
   *   ports connect to it.
   */
  Fader * monitor_fader;

  char * hw_out_l_id;
  char * hw_out_r_id;

  /* caches */
  ExtPort * hw_out_l;
  ExtPort * hw_out_r;

  /** Pointer to owner audio engine, if any. */
  AudioEngine * audio_engine;
} ControlRoom;

static const cyaml_schema_field_t control_room_fields_schema[] = {
  YAML_FIELD_INT (ControlRoom, schema_version),
  YAML_FIELD_MAPPING_PTR (
    ControlRoom,
    monitor_fader,
    fader_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t control_room_schema = {
  YAML_VALUE_PTR (ControlRoom, control_room_fields_schema),
};

/**
 * Inits the control room from a project.
 */
COLD NONNULL_ARGS (1) void control_room_init_loaded (
  ControlRoom * self,
  AudioEngine * engine);

/**
 * Creates a new control room.
 */
COLD WARN_UNUSED_RESULT ControlRoom *
control_room_new (AudioEngine * engine);

/**
 * Sets dim_output to on/off and notifies interested
 * parties.
 */
void
control_room_set_dim_output (
  ControlRoom * self,
  int           dim_output);

/**
 * Used during serialization.
 */
NONNULL ControlRoom *
control_room_clone (const ControlRoom * src);

void
control_room_free (ControlRoom * self);

/**
 * @}
 */

#endif
