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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Mixer selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_MIXER_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_MIXER_SELECTIONS_H__

#include "utils/yaml.h"

#include "schemas/plugins/plugin.h"

typedef struct MixerSelections_v1
{
  int               schema_version;
  PluginSlotType_v1 type;
  int         slots[MIXER_SELECTIONS_MAX_SLOTS];
  Plugin_v1 * plugins[MIXER_SELECTIONS_MAX_SLOTS];
  int         num_slots;
  int         track_pos;
  int         has_any;
} MixerSelections_v1;

static const cyaml_schema_field_t
  mixer_selections_fields_schema_v1[] = {
    YAML_FIELD_INT (
      MixerSelections_v1,
      schema_version),
    YAML_FIELD_ENUM (
      MixerSelections_v1,
      type,
      plugin_slot_type_strings_v1),
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
      MixerSelections_v1,
      slots,
      int_schema),
    CYAML_FIELD_SEQUENCE_COUNT (
      "plugins",
      CYAML_FLAG_DEFAULT,
      MixerSelections_v1,
      plugins,
      num_slots,
      &plugin_schema_v1,
      0,
      CYAML_UNLIMITED),
    YAML_FIELD_INT (MixerSelections_v1, track_pos),
    YAML_FIELD_INT (MixerSelections_v1, has_any),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  mixer_selections_schema_v1 = {
    YAML_VALUE_PTR (
      MixerSelections_v1,
      mixer_selections_fields_schema_v1),
  };

#endif
