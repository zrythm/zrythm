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
 * Region schema.
 */
#ifndef __SCHEMAS_AUDIO_REGION_H__
#define __SCHEMAS_AUDIO_REGION_H__

#include "audio/automation_point.h"
#include "audio/chord_object.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/position.h"
#include "audio/region_identifier.h"
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

typedef enum RegionMusicalMode_v1
{
  REGION_MUSICAL_MODE_INHERIT_v1,
  REGION_MUSICAL_MODE_OFF_v1,
  REGION_MUSICAL_MODE_ON_v1,
} RegionMusicalMode;

static const cyaml_strval_t
  region_musical_mode_strings_v1[] = {
    {__ ("Inherit"),
     REGION_MUSICAL_MODE_INHERIT_v1            },
    { __ ("Off"),    REGION_MUSICAL_MODE_OFF_v1},
    { __ ("On"),     REGION_MUSICAL_MODE_ON_v1 },
};

typedef struct ZRegion_v1
{
  ArrangerObject_v1     base;
  int                   schema_version;
  RegionIdentifier_v1   id;
  char *                name;
  GdkRGBA               color;
  MidiNote_v1 **        midi_notes;
  int                   num_midi_notes;
  size_t                midi_notes_size;
  MidiNote_v1 *         unended_notes[12000];
  int                   num_unended_notes;
  int                   pool_id;
  bool                  stretching;
  double                before_length;
  double                stretch_ratio;
  RegionMusicalMode_v1  musical_mode;
  Position_v1 *         split_points;
  int                   num_split_points;
  size_t                split_points_size;
  AutomationPoint_v1 ** aps;
  int                   num_aps;
  size_t                aps_size;
  AutomationPoint_v1 *  last_recorded_ap;
  ChordObject_v1 **     chord_objects;
  int                   num_chord_objects;
  size_t                chord_objects_size;
  int                   bounce;
  void *                layout;
  void *                chords_layout;
  GdkRectangle          last_main_full_rect;
  GdkRectangle          last_main_draw_rect;
  GdkRectangle          last_lane_full_rect;
  GdkRectangle          last_lane_draw_rect;
  gint64                last_clip_change;
  gint64                last_cache_time;
  ArrangerObject_v1     last_positions_obj;
  int                   magic;
} ZRegion_v1;

static const cyaml_schema_field_t region_fields_schema_v1[] = {
  YAML_FIELD_INT (ZRegion_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    ZRegion_v1,
    base,
    arranger_object_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ZRegion_v1,
    id,
    region_identifier_fields_schema),
  YAML_FIELD_STRING_PTR (ZRegion_v1, name),
  YAML_FIELD_INT (ZRegion_v1, pool_id),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion_v1,
    midi_notes,
    num_midi_notes,
    &midi_note_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aps",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion_v1,
    aps,
    num_aps,
    &automation_point_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_objects",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion_v1,
    chord_objects,
    num_chord_objects,
    &chord_object_schema,
    0,
    CYAML_UNLIMITED),
  YAML_FIELD_ENUM (
    ZRegion_v1,
    musical_mode,
    region_musical_mode_strings),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t region_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER_NULL_STR,
    ZRegion_v1,
    region_fields_schema_v1),
};

#endif
