// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Mixer selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_MIXER_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_MIXER_SELECTIONS_H__

#include "common/utils/yaml.h"
#include "gui/cpp/backend/cyaml_schemas/plugins/plugin.h"

typedef struct MixerSelections_v1
{
  int                schema_version;
  ZPluginSlotType_v1 type;
  int                slots[60];
  Plugin_v1 *        plugins[60];
  int                num_slots;
  unsigned int       track_name_hash;
  int                has_any;
} MixerSelections_v1;

static const cyaml_schema_field_t mixer_selections_fields_schema_v1[] = {
  YAML_FIELD_INT (MixerSelections_v1, schema_version),
  YAML_FIELD_ENUM (MixerSelections_v1, type, plugin_slot_type_strings_v1),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (MixerSelections_v1, slots, int_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "plugins",
    CYAML_FLAG_DEFAULT,
    MixerSelections_v1,
    plugins,
    num_slots,
    &plugin_schema_v1,
    0,
    CYAML_UNLIMITED),
  YAML_FIELD_UINT (MixerSelections_v1, track_name_hash),
  YAML_FIELD_INT (MixerSelections_v1, has_any),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t mixer_selections_schema_v1 = {
  YAML_VALUE_PTR (MixerSelections_v1, mixer_selections_fields_schema_v1),
};

#endif
