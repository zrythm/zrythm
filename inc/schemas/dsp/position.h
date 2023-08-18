// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Position schema.
 */

#ifndef __SCHEMAS_AUDIO_POSITION_H__
#define __SCHEMAS_AUDIO_POSITION_H__

#include <stdbool.h>

#include "utils/types.h"
#include "utils/yaml.h"

typedef struct Position_v1
{
  int            schema_version;
  double         ticks;
  signed_frame_t frames;
} Position_v1;

static const cyaml_schema_field_t position_fields_schema_v1[] = {
  YAML_FIELD_INT (Position_v1, schema_version),
  YAML_FIELD_FLOAT (Position_v1, ticks),
  YAML_FIELD_INT (Position_v1, frames), CYAML_FIELD_END
};

static const cyaml_schema_value_t position_schema_v1 = {
  YAML_VALUE_PTR (Position_v1, position_fields_schema_v1),
};

Position *
position_upgrade_from_v1 (Position_v1 * old);

#endif
