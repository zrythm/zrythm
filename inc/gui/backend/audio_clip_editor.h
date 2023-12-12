// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio clip editor backend.
 */

#ifndef __AUDIO_AUDIO_CLIP_EDITOR_H__
#define __AUDIO_AUDIO_CLIP_EDITOR_H__

#include "gui/backend/editor_settings.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUDIO_CLIP_EDITOR_SCHEMA_VERSION 1

#define AUDIO_CLIP_EDITOR (CLIP_EDITOR->audio_clip_editor)

/**
 * Audio clip editor serializable backend.
 *
 * The actual widgets should reflect the
 * information here.
 */
typedef struct AudioClipEditor
{
  int            schema_version;
  EditorSettings editor_settings;
} AudioClipEditor;

static const cyaml_schema_field_t audio_clip_editor_fields_schema[] = {
  YAML_FIELD_INT (AudioClipEditor, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioClipEditor,
    editor_settings,
    editor_settings_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_clip_editor_schema = {
  YAML_VALUE_PTR (AudioClipEditor, audio_clip_editor_fields_schema),
};

void
audio_clip_editor_init (AudioClipEditor * self);

AudioClipEditor *
audio_clip_editor_clone (AudioClipEditor * src);

AudioClipEditor *
audio_clip_editor_new (void);

void
audio_clip_editor_free (AudioClipEditor * self);

/**
 * @}
 */

#endif
