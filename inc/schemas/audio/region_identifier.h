/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Region identifier schema.
 */

#ifndef __SCHEMAS_AUDIO_REGION_IDENTIFIER_H__
#define __SCHEMAS_AUDIO_REGION_IDENTIFIER_H__

#include "utils/general.h"
#include "utils/yaml.h"

typedef enum RegionType_v1
{
  REGION_TYPE_MIDI_v1 = 1 << 0,
  REGION_TYPE_AUDIO_v1 = 1 << 1,
  REGION_TYPE_AUTOMATION_v1 = 1 << 2,
  REGION_TYPE_CHORD_v1 = 1 << 3,
} RegionType_v1;

static const cyaml_bitdef_t region_type_bitvals_v1[] = {
  {.name = "midi",        .offset = 0, .bits = 1},
  { .name = "audio",      .offset = 1, .bits = 1},
  { .name = "automation", .offset = 2, .bits = 1},
  { .name = "chord",      .offset = 3, .bits = 1},
};

typedef struct RegionIdentifier_v1
{
  int           schema_version;
  RegionType_v1 type;
  int           link_group;
  int           track_pos;
  int           lane_pos;
  int           at_idx;
  int           idx;
} RegionIdentifier_v1;

static const cyaml_schema_field_t
  region_identifier_fields_schema_v1[] = {
    YAML_FIELD_INT (RegionIdentifier_v1, schema_version),
    YAML_FIELD_BITFIELD (
      RegionIdentifier_v1,
      type,
      region_type_bitvals_v1),
    YAML_FIELD_INT (RegionIdentifier_v1, link_group),
    YAML_FIELD_INT (RegionIdentifier_v1, track_pos),
    YAML_FIELD_INT (RegionIdentifier_v1, lane_pos),
    YAML_FIELD_INT (RegionIdentifier_v1, at_idx),
    YAML_FIELD_INT (RegionIdentifier_v1, idx),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t region_identifier_schema_v1 = {
  YAML_VALUE_PTR (
    RegionIdentifier_v1,
    region_identifier_fields_schema_v1),
};

static const cyaml_schema_value_t
  region_identifier_schema_default_v1 = {
    YAML_VALUE_DEFAULT (
      RegionIdentifier_v1,
      region_identifier_fields_schema_v1),
  };

#endif
