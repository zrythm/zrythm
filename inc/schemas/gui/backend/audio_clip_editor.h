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
 * @file
 *
 * Audio clip editor schema.
 */

#ifndef __SCHEMAS_AUDIO_AUDIO_CLIP_EDITOR_H__
#define __SCHEMAS_AUDIO_AUDIO_CLIP_EDITOR_H__

#include "gui/backend/editor_settings.h"
#include "utils/yaml.h"

typedef struct AudioClipEditor_v1
{
  int               schema_version;
  EditorSettings_v1 editor_settings;
} AudioClipEditor_v1;

static const cyaml_schema_field_t
  audio_clip_editor_fields_schema_v1[] = {
    YAML_FIELD_INT (AudioClipEditor_v1, schema_version),
    YAML_FIELD_MAPPING_EMBEDDED (
      AudioClipEditor_v1,
      editor_settings,
      editor_settings_fields_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  audio_clip_editor_schema_v1 = {
    YAML_VALUE_PTR (
      AudioClipEditor_v1,
      audio_clip_editor_fields_schema_v1),
  };

#endif
