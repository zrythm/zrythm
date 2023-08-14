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
