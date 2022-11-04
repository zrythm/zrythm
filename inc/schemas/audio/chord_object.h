// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

ChordObject *
chord_object_upgrade_from_v1 (ChordObject_v1 * old);

#endif
