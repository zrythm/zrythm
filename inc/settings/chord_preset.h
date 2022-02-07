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
 * Chord preset.
 */

#ifndef __SETTINGS_CHORD_PRESET_H__
#define __SETTINGS_CHORD_PRESET_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "audio/chord_descriptor.h"

#include "utils/yaml.h"

typedef struct ChordPresetPack ChordPresetPack;

/**
 * @addtogroup settings
 *
 * @{
 */

#define CHORD_PRESET_SCHEMA_VERSION 1

/**
 * A preset of chord descriptors.
 */
typedef struct ChordPreset
{
  int               schema_version;

  /** Preset name. */
  char *            name;

  /** Chord descriptors. */
  ChordDescriptor * descr[12];

  /** Pointer to owner pack. */
  ChordPresetPack * pack;
} ChordPreset;

static const cyaml_schema_field_t
chord_preset_fields_schema[] =
{
  YAML_FIELD_INT (ChordPreset, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    ChordPreset, descr, chord_descriptor_schema, 12),
  YAML_FIELD_STRING_PTR (ChordPreset, name),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
chord_preset_schema =
{
  YAML_VALUE_PTR (
    ChordPreset, chord_preset_fields_schema),
};

ChordPreset *
chord_preset_new (
  const char * name);

NONNULL
ChordPreset *
chord_preset_clone (
  const ChordPreset * src);

/**
 * Gets informational text.
 *
 * Must be free'd by caller.
 */
char *
chord_preset_get_info_text (
  const ChordPreset * self);

const char *
chord_preset_get_name (
  const ChordPreset * self);

void
chord_preset_set_name (
  ChordPreset * self,
  const char *  name);

/**
 * Frees the plugin setting.
 */
NONNULL
void
chord_preset_free (
  ChordPreset * self);

/**
 * @}
 */

#endif
