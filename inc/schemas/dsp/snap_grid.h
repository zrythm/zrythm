// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Snap/grid schema.
 */

#ifndef __SCHEMAS_AUDIO_SNAP_GRID_H__
#define __SCHEMAS_AUDIO_SNAP_GRID_H__

#include <stdbool.h>

#include "schemas/dsp/position.h"

typedef enum NoteLength_v1
{
  NOTE_LENGTH_BAR_v1,
  NOTE_LENGTH_BEAT_v1,
  NOTE_LENGTH_2_1_v1,
  NOTE_LENGTH_1_1_v1,
  NOTE_LENGTH_1_2_v1,
  NOTE_LENGTH_1_4_v1,
  NOTE_LENGTH_1_8_v1,
  NOTE_LENGTH_1_16_v1,
  NOTE_LENGTH_1_32_v1,
  NOTE_LENGTH_1_64_v1,
  NOTE_LENGTH_1_128_v1,
} NoteLength_v1;

static const cyaml_strval_t note_length_strings_v1[] = {
  {"bar",    NOTE_LENGTH_BAR_v1  },
  { "beat",  NOTE_LENGTH_BEAT_v1 },
  { "2/1",   NOTE_LENGTH_2_1_v1  },
  { "1/1",   NOTE_LENGTH_1_1_v1  },
  { "1/2",   NOTE_LENGTH_1_2_v1  },
  { "1/4",   NOTE_LENGTH_1_4_v1  },
  { "1/8",   NOTE_LENGTH_1_8_v1  },
  { "1/16",  NOTE_LENGTH_1_16_v1 },
  { "1/32",  NOTE_LENGTH_1_32_v1 },
  { "1/64",  NOTE_LENGTH_1_64_v1 },
  { "1/128", NOTE_LENGTH_1_128_v1},
};

typedef enum NoteType_v1
{
  NOTE_TYPE_NORMAL_v1,
  NOTE_TYPE_DOTTED_v1,
  NOTE_TYPE_TRIPLET_v1,
} NoteType_v1;

static const cyaml_strval_t note_type_strings_v1[] = {
  {"normal",   NOTE_TYPE_NORMAL_v1 },
  { "dotted",  NOTE_TYPE_DOTTED_v1 },
  { "triplet", NOTE_TYPE_TRIPLET_v1},
};

typedef enum NoteLengthType_v1
{
  NOTE_LENGTH_CUSTOM_v1,
  NOTE_LENGTH_LINK_v1,
  NOTE_LENGTH_LAST_OBJECT_v1,
} NoteLengthType_v1;

static const cyaml_strval_t note_length_type_strings_v1[] = {
  {"custom",       NOTE_LENGTH_CUSTOM_v1     },
  { "link",        NOTE_LENGTH_LINK_v1       },
  { "last object", NOTE_LENGTH_LAST_OBJECT_v1},
};

typedef enum SnapGridType_v1
{
  SNAP_GRID_TYPE_TIMELINE_v1,
  SNAP_GRID_TYPE_EDITOR_v1,
} SnapGridType_v1;

static const cyaml_strval_t snap_grid_type_strings_v1[] = {
  {"timeline", SNAP_GRID_TYPE_TIMELINE_v1},
  { "editor",  SNAP_GRID_TYPE_EDITOR_v1  },
};

typedef struct SnapGrid_v1
{
  int               schema_version;
  SnapGridType_v1   type;
  bool              snap_adaptive;
  NoteLength_v1     snap_note_length;
  NoteType_v1       snap_note_type;
  bool              snap_to_grid;
  bool              snap_to_grid_keep_offset;
  bool              snap_to_events;
  NoteLength_v1     default_note_length;
  NoteType_v1       default_note_type;
  bool              default_adaptive;
  NoteLengthType_v1 length_type;
} SnapGrid_v1;

static const cyaml_schema_field_t snap_grid_fields_schema_v1[] = {
  YAML_FIELD_INT (SnapGrid_v1, schema_version),
  YAML_FIELD_ENUM (SnapGrid_v1, type, snap_grid_type_strings_v1),
  YAML_FIELD_ENUM (
    SnapGrid_v1,
    snap_note_length,
    note_length_strings_v1),
  YAML_FIELD_ENUM (
    SnapGrid_v1,
    snap_note_type,
    note_type_strings_v1),
  YAML_FIELD_INT (SnapGrid_v1, snap_adaptive),
  YAML_FIELD_ENUM (
    SnapGrid_v1,
    default_note_length,
    note_length_strings_v1),
  YAML_FIELD_ENUM (
    SnapGrid_v1,
    default_note_type,
    note_type_strings_v1),
  YAML_FIELD_INT (SnapGrid_v1, default_adaptive),
  YAML_FIELD_ENUM (
    SnapGrid_v1,
    length_type,
    note_length_type_strings_v1),
  YAML_FIELD_INT (SnapGrid_v1, snap_to_grid),
  YAML_FIELD_INT (SnapGrid_v1, snap_to_grid_keep_offset),
  YAML_FIELD_INT (SnapGrid_v1, snap_to_events),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t snap_grid_schema_v1 = {
  YAML_VALUE_PTR (SnapGrid_v1, snap_grid_fields_schema_v1),
};

#endif
