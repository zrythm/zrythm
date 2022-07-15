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
 * Chord descriptor schema.
 */

#ifndef __SCHEMAS_AUDIO_CHORD_DESCRIPTOR_H__
#define __SCHEMAS_AUDIO_CHORD_DESCRIPTOR_H__

#include <stdint.h>

#include "audio/position.h"
#include "utils/yaml.h"

typedef enum MusicalNote_v1
{
  NOTE_C_v1,
  NOTE_CS_v1,
  NOTE_D_v1,
  NOTE_DS_v1,
  NOTE_E_v1,
  NOTE_F_v1,
  NOTE_FS_v1,
  NOTE_G_v1,
  NOTE_GS_v1,
  NOTE_A_v1,
  NOTE_AS_v1,
  NOTE_B_v1,
} MusicalNote_v1;

static const cyaml_strval_t musical_note_strings_v1[] = {
  {"C",   NOTE_C_v1 },
  { "C#", NOTE_CS_v1},
  { "D",  NOTE_D_v1 },
  { "D#", NOTE_DS_v1},
  { "E",  NOTE_E_v1 },
  { "F",  NOTE_F_v1 },
  { "F#", NOTE_FS_v1},
  { "G",  NOTE_G_v1 },
  { "G#", NOTE_GS_v1},
  { "A",  NOTE_A_v1 },
  { "A#", NOTE_AS_v1},
  { "B",  NOTE_B_v1 },
};

typedef enum ChordType_v1
{
  CHORD_TYPE_MAJ_v1,
  CHORD_TYPE_MIN_v1,
  CHORD_TYPE_DIM_v1,
  CHORD_TYPE_SUS4_v1,
  CHORD_TYPE_SUS2_v1,
  CHORD_TYPE_AUG_v1,
  NUM_CHORD_TYPES_v1,
} ChordType_v1;

typedef enum ChordAccent_v1
{
  CHORD_ACC_NONE_v1,
  CHORD_ACC_7_v1,
  CHORD_ACC_j7_v1,
  CHORD_ACC_b9_v1,
  CHORD_ACC_9_v1,
  CHORD_ACC_S9_v1,
  CHORD_ACC_11_v1,
  CHORD_ACC_b5_S11_v1,
  CHORD_ACC_S5_b13_v1,
  CHORD_ACC_6_13_v1,
  NUM_CHORD_ACCENTS_v1,
} ChordAccent_v1;

typedef struct ChordDescriptor_v1
{
  int            schema_version;
  bool           has_bass;
  bool           is_custom;
  MusicalNote_v1 root_note;
  MusicalNote_v1 bass_note;
  ChordType_v1   type;
  ChordAccent_v1 accent;
  bool           notes[CHORD_DESCRIPTOR_MAX_NOTES];
  int            inversion;
} ChordDescriptor_v1;

static const cyaml_schema_field_t
  chord_descriptor_fields_schema_v1[] = {
    YAML_FIELD_INT (ChordDescriptor_v1, schema_version),
    YAML_FIELD_INT (ChordDescriptor_v1, has_bass),
    YAML_FIELD_ENUM (
      ChordDescriptor,
      root_note,
      musical_note_strings_v1),
    YAML_FIELD_ENUM (
      ChordDescriptor_v1,
      bass_note,
      musical_note_strings_v1),
    CYAML_FIELD_SEQUENCE_FIXED (
      "notes",
      CYAML_FLAG_OPTIONAL,
      ChordDescriptor_v1,
      notes,
      &int_schema,
      36),
    YAML_FIELD_INT (ChordDescriptor_v1, inversion),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t chord_descriptor_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordDescriptor_v1,
    chord_descriptor_fields_schema_v1),
};

#endif
