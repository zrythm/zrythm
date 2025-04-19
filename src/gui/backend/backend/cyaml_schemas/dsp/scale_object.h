// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Scale object schema.
 */

#ifndef __SCHEMAS_AUDIO_SCALE_OBJECT_H__
#define __SCHEMAS_AUDIO_SCALE_OBJECT_H__

#include <cstdint>

#include "gui/backend/backend/cyaml_schemas/dsp/scale.h"
#include "gui/backend/backend/cyaml_schemas/gui/backend/arranger_object.h"
#include "utils/yaml.h"

typedef struct ScaleObject_v1
{
  ArrangerObject_v1 base;
  int               index;
  MusicalScale_v2 * scale;
} ScaleObject_v1;

static const cyaml_schema_field_t scale_object_fields_schema_v1[] = {
  YAML_FIELD_MAPPING_EMBEDDED (
    ScaleObject_v1,
    base,
    arranger_object_fields_schema_v1),
  YAML_FIELD_INT (ScaleObject_v1, index),
  YAML_FIELD_MAPPING_PTR (ScaleObject_v1, scale, musical_scale_fields_schema_v2),
  YAML_FIELD_IGNORE_OPT ("schema_version"),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t scale_object_schema_v1 = {
  YAML_VALUE_PTR (ScaleObject_v1, scale_object_fields_schema_v1),
};

#endif
