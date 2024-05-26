/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <cstdlib>

#include "dsp/channel.h"
#include "dsp/track.h"
#include "gui/backend/audio_clip_editor.h"
#include "project.h"
#include "utils/objects.h"

void
audio_clip_editor_init (AudioClipEditor * self)
{
  editor_settings_init (&self->editor_settings);
}

AudioClipEditor *
audio_clip_editor_clone (AudioClipEditor * src)
{
  AudioClipEditor * self = object_new (AudioClipEditor);

  self->editor_settings = src->editor_settings;

  return self;
}

AudioClipEditor *
audio_clip_editor_new (void)
{
  AudioClipEditor * self = object_new (AudioClipEditor);

  return self;
}

void
audio_clip_editor_free (AudioClipEditor * self)
{
  object_zero_and_free (self);
}
