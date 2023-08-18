// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/pool.h"
#include "utils/objects.h"

#include "schemas/dsp/pool.h"

AudioPool *
audio_pool_upgrade_from_v1 (AudioPool_v1 * old)
{
  if (!old)
    return NULL;

  AudioPool * self = object_new (AudioPool);

  self->schema_version = AUDIO_POOL_SCHEMA_VERSION;

  self->num_clips = old->num_clips;
  self->clips = g_malloc_n (
    (size_t) old->num_clips, sizeof (AudioClip *));
  for (int i = 0; i < self->num_clips; i++)
    {
      self->clips[i] =
        audio_clip_upgrade_from_v1 (old->clips[i]);
    }

  return self;
}
