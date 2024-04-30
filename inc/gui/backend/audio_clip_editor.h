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
  EditorSettings editor_settings;
} AudioClipEditor;

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
