// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Region schema.
 */
#ifndef __SCHEMAS_AUDIO_REGION_H__
#define __SCHEMAS_AUDIO_REGION_H__

#include "schemas/dsp/automation_point.h"
#include "schemas/dsp/chord_object.h"
#include "schemas/dsp/midi_note.h"
#include "schemas/dsp/position.h"
#include "schemas/dsp/region_identifier.h"
#include "schemas/gui/backend/arranger_object.h"
#include "utils/yaml.h"

typedef enum RegionMusicalMode_v1
{
  REGION_MUSICAL_MODE_INHERIT_v1,
  REGION_MUSICAL_MODE_OFF_v1,
  REGION_MUSICAL_MODE_ON_v1,
} RegionMusicalMode_v1;

static const cyaml_strval_t region_musical_mode_strings_v1[] = {
  { "Inherit", REGION_MUSICAL_MODE_INHERIT_v1 },
  { "Off",     REGION_MUSICAL_MODE_OFF_v1     },
  { "On",      REGION_MUSICAL_MODE_ON_v1      },
};

typedef struct ZRegion_v1
{
  ArrangerObject_v1     base;
  int                   schema_version;
  RegionIdentifier_v1   id;
  char *                name;
  MidiNote_v1 **        midi_notes;
  int                   num_midi_notes;
  int                   pool_id;
  float                 gain;
  RegionMusicalMode_v1  musical_mode;
  AutomationPoint_v1 ** aps;
  int                   num_aps;
  ChordObject_v1 **     chord_objects;
  int                   num_chord_objects;
} ZRegion_v1;

static const cyaml_schema_field_t region_fields_schema_v1[] = {
  YAML_FIELD_INT (ZRegion_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (ZRegion_v1, base, arranger_object_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (ZRegion_v1, id, region_identifier_fields_schema_v1),
  YAML_FIELD_STRING_PTR (ZRegion_v1, name),
  YAML_FIELD_INT (ZRegion_v1, pool_id),
  YAML_FIELD_FLOAT (ZRegion_v1, gain),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion_v1,
    midi_notes,
    num_midi_notes,
    &midi_note_schema_v1,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aps",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion_v1,
    aps,
    num_aps,
    &automation_point_schema_v1,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_objects",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion_v1,
    chord_objects,
    num_chord_objects,
    &chord_object_schema_v1,
    0,
    CYAML_UNLIMITED),
  YAML_FIELD_IGNORE_OPT ("color"),
  YAML_FIELD_IGNORE_OPT ("use_color"),
  YAML_FIELD_ENUM (ZRegion_v1, musical_mode, region_musical_mode_strings_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t region_schema_v1 = {
  YAML_VALUE_PTR_NULLABLE (ZRegion_v1, region_fields_schema_v1),
};

#endif
