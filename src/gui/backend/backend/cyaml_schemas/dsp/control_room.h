// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Control room schema.
 */
#ifndef __SCHEMAS_AUDIO_CONTROL_ROOM_H__
#define __SCHEMAS_AUDIO_CONTROL_ROOM_H__

#include "gui/backend/backend/cyaml_schemas/dsp/fader.h"

typedef struct ControlRoom_v1
{
  int        schema_version;
  Fader_v1 * monitor_fader;
} ControlRoom_v1;

static const cyaml_schema_field_t control_room_fields_schema_v1[] = {
  YAML_FIELD_INT (ControlRoom_v1, schema_version),
  YAML_FIELD_MAPPING_PTR (ControlRoom_v1, monitor_fader, fader_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t control_room_schema_v1 = {
  YAML_VALUE_PTR (ControlRoom_v1, control_room_fields_schema_v1),
};

typedef struct ControlRoom_v2
{
  int        schema_version;
  Fader_v2 * monitor_fader;
} ControlRoom_v2;

static const cyaml_schema_field_t control_room_fields_schema_v2[] = {
  YAML_FIELD_INT (ControlRoom_v2, schema_version),
  YAML_FIELD_MAPPING_PTR (ControlRoom_v2, monitor_fader, fader_fields_schema_v2),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t control_room_schema_v2 = {
  YAML_VALUE_PTR (ControlRoom_v2, control_room_fields_schema_v2),
};

ControlRoom_v2 *
control_room_upgrade_from_v1 (ControlRoom_v1 * old);

#endif
