/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Marker schema.
 */

#ifndef __SCHEMAS_AUDIO_MARKER_H__
#define __SCHEMAS_AUDIO_MARKER_H__

#include <stdint.h>

#include "utils/yaml.h"

#include "schemas/gui/backend/arranger_object.h"

typedef enum MarkerType_v1
{
  MARKER_TYPE_START_v1,
  MARKER_TYPE_END_v1,
  MARKER_TYPE_CUSTOM_v1,
} MarkerType_v1;

static const cyaml_strval_t marker_type_strings_v1[] = {
  {"start",   MARKER_TYPE_START_v1 },
  { "end",    MARKER_TYPE_END_v1   },
  { "custom", MARKER_TYPE_CUSTOM_v1},
};

typedef struct Marker_v1
{
  ArrangerObject_v1 base;
  int               schema_version;
  MarkerType_v1     type;
  char *            name;
  int               track_pos;
  int               index;
  void *            layout;
} Marker;

static const cyaml_schema_field_t
  marker_fields_schema_v1[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      Marker_v1,
      base,
      arranger_object_fields_schema_v1),
    YAML_FIELD_MAPPING_STRING_PTR (Marker_v1, name),
    YAML_FIELD_INT (Marker_v1, track_pos),
    YAML_FIELD_INT (Marker_v1, index),
    YAML_FIELD_ENUM (
      Marker_v1,
      type,
      marker_type_strings_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t marker_schema_v1 = {
  YAML_VALUE_PTR (Marker_v1, marker_fields_schema_v1),
};

#endif
