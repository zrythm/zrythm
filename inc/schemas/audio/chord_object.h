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
 * Chord object schema.
 */

#ifndef __SCHEMAS_AUDIO_CHORD_OBJECT_H__
#define __SCHEMAS_AUDIO_CHORD_OBJECT_H__

#include <stdint.h>

#include "utils/yaml.h"

#include "schemas/gui/backend/arranger_object.h"

typedef struct ChordObject_v1
{
  ArrangerObject_v1 base;
  int               schema_version;
  int               index;
  int               chord_index;
  int               magic;
  void *            layout;
} ChordObject_v1;

static const cyaml_schema_field_t chord_object_fields_schema_v1[] = {
  YAML_FIELD_MAPPING_EMBEDDED (
    ChordObject_v1,
    base,
    arranger_object_fields_schema_v1),
  YAML_FIELD_INT (ChordObject_v1, schema_version),
  YAML_FIELD_INT (ChordObject_v1, index),
  YAML_FIELD_INT (ChordObject_v1, chord_index),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t chord_object_schema_v1 = {
  YAML_VALUE_PTR (ChordObject_v1, chord_object_fields_schema_v1),
};

#endif
