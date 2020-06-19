/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/clip.h"
#include "audio/pool.h"
#include "utils/arrays.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <gtk/gtk.h>

/**
 * Inits after loading a project.
 */
void
audio_pool_init_loaded (
  AudioPool * self)
{
  self->clips_size = (size_t) self->num_clips;

  for (int i = 0; i < self->num_clips; i++)
    {
      audio_clip_init_loaded (self->clips[i]);
    }
}

/**
 * Creates a new audio pool.
 */
AudioPool *
audio_pool_new ()
{
  AudioPool * self =
    calloc (1, sizeof (AudioPool));

  self->clips_size = 2;
  self->clips =
    calloc (
      self->clips_size, sizeof (AudioClip *));

  return self;
}

/**
 * Adds an audio clip to the pool.
 *
 * Changes the name of the clip if another clip with
 * the same name already exists.
 */
int
audio_pool_add_clip (
  AudioPool * self,
  AudioClip * clip)
{
  g_return_val_if_fail (clip && clip->name, -1);

  array_double_size_if_full (
    self->clips, self->num_clips, self->clips_size,
    AudioClip *);

  /* check for name collisions */
  AudioClip * other_clip;
  for (int i = 0; i < self->num_clips; i++)
    {
      other_clip = self->clips[i];
      if (string_is_equal (
            clip->name, other_clip->name, 0))
        {
          /* TODO set new name */

          /* reset counter to check again with the
           * new name */
          /*i = 0;*/
        }
    }

  clip->pool_id = self->num_clips;

  array_append (
    self->clips,
    self->num_clips,
    clip);

  return clip->pool_id;
}

void
audio_pool_free (
  AudioPool * self)
{
  for (int i = 0; i < self->num_clips; i++)
    {
      object_free_w_func_and_null (
        audio_clip_free, self->clips[i]);
    }
  object_zero_and_free (self->clips);

  object_zero_and_free (self);
}
