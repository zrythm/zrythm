// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Marker schema.
 */

#ifndef __SCHEMAS_AUDIO_MARKER_H__
#define __SCHEMAS_AUDIO_MARKER_H__

#include <cstdint>

#include "schemas/gui/backend/arranger_object.h"
#include "utils/yaml.h"

typedef enum MarkerType_v1
{
  MARKER_TYPE_START_v1,
  MARKER_TYPE_END_v1,
  MARKER_TYPE_CUSTOM_v1,
} MarkerType_v1;

static const cyaml_strval_t marker_type_strings_v1[] = {
  { "start",  MARKER_TYPE_START_v1  },
  { "end",    MARKER_TYPE_END_v1    },
  { "custom", MARKER_TYPE_CUSTOM_v1 },
};

typedef struct Marker_v1
{
  ArrangerObject_v1 base;
  char *            name;
  unsigned int      track_name_hash;
  int               index;
  MarkerType_v1     type;
} Marker_v1;

static const cyaml_schema_field_t marker_fields_schema_v1[] = {
  YAML_FIELD_MAPPING_EMBEDDED (Marker_v1, base, arranger_object_fields_schema_v1),
  YAML_FIELD_STRING_PTR (Marker_v1, name),
  YAML_FIELD_UINT (Marker_v1, track_name_hash),
  YAML_FIELD_INT (Marker_v1, index),
  YAML_FIELD_ENUM (Marker_v1, type, marker_type_strings_v1),
  YAML_FIELD_IGNORE_OPT ("schema_version"),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t marker_schema_v1 = {
  YAML_VALUE_PTR (Marker_v1, marker_fields_schema_v1),
};

#endif
