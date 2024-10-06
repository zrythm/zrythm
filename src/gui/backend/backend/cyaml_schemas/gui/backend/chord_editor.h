// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Chord editor schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_CHORD_EDITOR_H__
#define __SCHEMAS_GUI_BACKEND_CHORD_EDITOR_H__

#include "common/utils/yaml.h"
#include "gui/backend/backend/cyaml_schemas/dsp/chord_descriptor.h"
#include "gui/backend/backend/cyaml_schemas/gui/backend/editor_settings.h"

typedef struct ChordEditor_v1
{
  int                  schema_version;
  ChordDescriptor_v2 * chords[128];
  int                  num_chords;
  EditorSettings_v1    editor_settings;
} ChordEditor_v1;

static const cyaml_schema_field_t chord_editor_fields_schema_v1[] = {
  YAML_FIELD_INT (ChordEditor_v1, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    ChordEditor_v1,
    chords,
    chord_descriptor_schema_v2),
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
