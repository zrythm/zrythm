// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/clip.h"
#include "utils/objects.h"

#include "schemas/dsp/clip.h"

AudioClip *
audio_clip_upgrade_from_v1 (AudioClip_v1 * old)
{
  if (!old)
    return NULL;

  AudioClip * self = object_new (AudioClip);

  self->schema_version = AUDIO_CLIP_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  UPDATE (name);
  UPDATE (file_hash);
  UPDATE (bpm);
  UPDATE (bit_depth);
  UPDATE (use_flac);
  UPDATE (samplerate);
  UPDATE (pool_id);

  return self;
}
