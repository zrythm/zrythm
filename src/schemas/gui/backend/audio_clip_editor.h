// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio clip editor schema.
 */

#ifndef __SCHEMAS_AUDIO_AUDIO_CLIP_EDITOR_H__
#define __SCHEMAS_AUDIO_AUDIO_CLIP_EDITOR_H__

#include "schemas/gui/backend/editor_settings.h"
#include "utils/yaml.h"

typedef struct AudioClipEditor_v1
{
  int               schema_version;
  EditorSettings_v1 editor_settings;
} AudioClipEditor_v1;

static const cyaml_schema_field_t audio_clip_editor_fields_schema_v1[] = {
  YAML_FIELD_INT (AudioClipEditor_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioClipEditor_v1,
    editor_settings,
    editor_settings_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_clip_editor_schema_v1 = {
  YAML_VALUE_PTR (AudioClipEditor_v1, audio_clip_editor_fields_schema_v1),
};

#endif
