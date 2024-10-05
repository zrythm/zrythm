// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Velocity schema.
 */

#ifndef __SCHEMAS_AUDIO_VELOCITY_H__
#define __SCHEMAS_AUDIO_VELOCITY_H__

#include "schemas/gui/backend/arranger_object.h"

typedef struct Velocity_v1
{
  ArrangerObject_v1 base;
  int               schema_version;
  uint8_t           vel;
} Velocity_v1;

static const cyaml_schema_field_t velocity_fields_schema_v1[] = {
  YAML_FIELD_MAPPING_EMBEDDED (Velocity_v1, base, arranger_object_fields_schema_v1),
  YAML_FIELD_INT (Velocity_v1, schema_version),
  YAML_FIELD_UINT (Velocity_v1, vel), CYAML_FIELD_END
};

static const cyaml_schema_value_t velocity_schema_v1 = {
  YAML_VALUE_PTR (Velocity_v1, velocity_fields_schema_v1),
};

#endif
