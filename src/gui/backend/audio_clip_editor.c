/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <stdlib.h>

#include "dsp/channel.h"
#include "dsp/track.h"
#include "gui/backend/audio_clip_editor.h"
#include "project.h"
#include "utils/objects.h"

void
audio_clip_editor_init (AudioClipEditor * self)
{
  self->schema_version = AUDIO_CLIP_EDITOR_SCHEMA_VERSION;

  editor_settings_init (&self->editor_settings);
}

AudioClipEditor *
audio_clip_editor_clone (AudioClipEditor * src)
{
  AudioClipEditor * self = object_new (AudioClipEditor);
  self->schema_version = AUDIO_CLIP_EDITOR_SCHEMA_VERSION;

  self->editor_settings = src->editor_settings;

  return self;
}

AudioClipEditor *
audio_clip_editor_new (void)
{
  AudioClipEditor * self = object_new (AudioClipEditor);
  self->schema_version = AUDIO_CLIP_EDITOR_SCHEMA_VERSION;

  return self;
}

void
audio_clip_editor_free (AudioClipEditor * self)
{
  object_zero_and_free (self);
}
