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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Chord editor schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_CHORD_EDITOR_H__
#define __SCHEMAS_GUI_BACKEND_CHORD_EDITOR_H__

#include "audio/chord_descriptor.h"
#include "gui/backend/editor_settings.h"
#include "utils/yaml.h"

typedef struct ChordEditor_v1
{
  int                  schema_version;
  ChordDescriptor_v1 * chords[128];
  int                  num_chords;
  EditorSettings_v1    editor_settings;
} ChordEditor_v1;

static const cyaml_schema_field_t chord_editor_fields_schema_v1[] = {
  YAML_FIELD_INT (ChordEditor_v1, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    ChordEditor_v1,
    chords,
    chord_descriptor_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    ChordEditor_v1,
    editor_settings,
    editor_settings_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t chord_editor_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordEditor_v1,
    chord_editor_fields_schema_v1),
};

#endif
