/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Transport schema.
 */

#ifndef __SCHEMAS_AUDIO_TRANSPORT_H__
#define __SCHEMAS_AUDIO_TRANSPORT_H__

#include <stdint.h>

#include "utils/types.h"

#include "schemas/audio/port.h"
#include "zix/sem.h"

typedef enum DummyEnum
{
  DUMMY_ENUM1,
} DummyEnum;

typedef struct TimeSignature_v1
{
  int schema_version;
  int beats_per_bar;
  int beat_unit;
} TimeSignature_v1;

static const cyaml_schema_field_t
  time_signature_fields_schema_v1[] = {
    YAML_FIELD_INT (TimeSignature_v1, schema_version),
    YAML_FIELD_INT (TimeSignature_v1, beats_per_bar),
    YAML_FIELD_INT (TimeSignature_v1, beat_unit),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t time_signature_schema_v1 = {
  YAML_VALUE_PTR (
    TimeSignature_v1,
    time_signature_fields_schema_v1),
};

typedef struct Transport_v1
{
  int              schema_version;
  int              total_bars;
  Position_v1      playhead_pos;
  Position_v1      cue_pos;
  Position_v1      loop_start_pos;
  Position_v1      loop_end_pos;
  Position_v1      punch_in_pos;
  Position_v1      punch_out_pos;
  Position_v1      range_1;
  Position_v1      range_2;
  int              has_range;
  TimeSignature_v1 time_sig;
  double           ticks_per_beat;
  double           ticks_per_bar;
  int              sixteenths_per_beat;
  int              sixteenths_per_bar;
  nframes_t        position;
  int              rolling;
  bool             loop;
  bool             punch_mode;
  bool             recording;
  bool             metronome_enabled;
  bool             start_playback_on_midi_input;
  DummyEnum        recording_mode;
  ZixSem           paused;
  Position_v1      playhead_before_pause;
  Port_v1 *        roll;
  Port_v1 *        stop;
  Port_v1 *        backward;
  Port_v1 *        forward;
  Port_v1 *        loop_toggle;
  Port_v1 *        rec_toggle;
  DummyEnum        play_state;
} Transport;

static const cyaml_strval_t beat_unit_strings_v1[] = {
  {"2",   BEAT_UNIT_2_v1 },
  { "4",  BEAT_UNIT_4_v1 },
  { "8",  BEAT_UNIT_8_v1 },
  { "16", BEAT_UNIT_16_v1},
};

static const cyaml_schema_field_t transport_fields_schema_v1[] = {
  YAML_FIELD_INT (Transport_v1, schema_version),
  YAML_FIELD_INT (Transport_v1, total_bars),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    playhead_pos,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    cue_pos,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    loop_start_pos,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    loop_end_pos,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    punch_in_pos,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    punch_out_pos,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    range_1,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    range_2,
    position_fields_schema_v1),
  YAML_FIELD_INT (Transport_v1, has_range),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport_v1,
    time_sig,
    time_signature_fields_schema_v1),
  YAML_FIELD_INT (Transport_v1, position),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport_v1,
    roll,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport_v1,
    stop,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport_v1,
    backward,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport_v1,
    forward,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport_v1,
    loop_toggle,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport_v1,
    rec_toggle,
    port_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t transport_schema_v1 = {
  YAML_VALUE_PTR (Transport_v1, transport_fields_schema_v1),
};

#endif
