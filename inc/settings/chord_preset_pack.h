/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * @file
 *
 * Chord preset pack.
 */

#ifndef __SETTINGS_CHORD_PRESET_PACK_H__
#define __SETTINGS_CHORD_PRESET_PACK_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "settings/chord_preset.h"
#include "utils/yaml.h"

/**
 * @addtogroup settings
 *
 * @{
 */

#define CHORD_PRESET_PACK_SCHEMA_VERSION 1

/**
 * Chord preset pack.
 */
typedef struct ChordPresetPack
{
  int schema_version;

  /** Pack name. */
  char * name;

  /** Presets. */
  ChordPreset ** presets;
  int            num_presets;
  size_t         presets_size;

  /** Whether this is a standard preset pack (not
   * user-defined). */
  bool is_standard;
} ChordPresetPack;

static const cyaml_schema_field_t chord_preset_pack_fields_schema[] = {
  YAML_FIELD_INT (ChordPresetPack, schema_version),
  YAML_FIELD_STRING_PTR (ChordPresetPack, name),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    ChordPresetPack,
    presets,
    chord_preset_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t chord_preset_pack_schema = {
  YAML_VALUE_PTR (ChordPresetPack, chord_preset_pack_fields_schema),
};

ChordPresetPack *
chord_preset_pack_new (const char * name, bool is_standard);

bool
chord_preset_pack_contains_name (const ChordPresetPack * self, const char * name);

bool
chord_preset_pack_contains_preset (
  const ChordPresetPack * self,
  const ChordPreset *     pset);

/**
 * @note The given preset is cloned so the caller is
 *   still responsible for @ref pset.
 */
void
chord_preset_pack_add_preset (ChordPresetPack * self, const ChordPreset * pset);

void
chord_preset_pack_delete_preset (ChordPresetPack * self, ChordPreset * pset);

const char *
chord_preset_pack_get_name (const ChordPresetPack * self);

void
chord_preset_pack_set_name (ChordPresetPack * self, const char * name);

ChordPresetPack *
chord_preset_pack_clone (const ChordPresetPack * src);

GMenuModel *
chord_preset_pack_generate_context_menu (const ChordPresetPack * self);

void
chord_preset_pack_free (ChordPresetPack * self);

void
chord_preset_pack_destroy_cb (void * self);

/**
 * @}
 */

#endif
