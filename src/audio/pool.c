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
#include "audio/track.h"
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

static bool
name_exists (
  AudioPool *  self,
  const char * name)
{
  AudioClip * clip;
  for (int i = 0; i < self->num_clips; i++)
    {
      clip = self->clips[i];
      if (string_is_equal (clip->name, name))
        {
          return true;
        }
    }
  return false;
}

/**
 * Ensures that the name of the clip is unique.
 *
 * The clip must not be part of the pool yet.
 *
 * If the clip name is not unique, it will be
 * replaced by a unique name.
 */
void
audio_pool_ensure_unique_clip_name (
  AudioPool * self,
  AudioClip * clip)
{
  int i = 1;
  char * orig_name = clip->name;
  char * new_name = g_strdup (orig_name);

  while (name_exists (self, new_name))
    {
      g_free (new_name);
      new_name =
        g_strdup_printf (
          "%s (%d)", orig_name, i++);
    }

  g_free (clip->name);
  clip->name = new_name;
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

  audio_pool_ensure_unique_clip_name (self, clip);

  clip->pool_id = self->num_clips;

  array_append (
    self->clips,
    self->num_clips,
    clip);

  return clip->pool_id;
}

/**
 * Generates a name for a recording clip.
 */
char *
audio_pool_gen_name_for_recording_clip (
  AudioPool * pool,
  Track *     track,
  int         lane)
{
  return
    g_strdup_printf (
      "%s - lane %d - recording",
      track->name,
      /* add 1 to get human friendly index */
      lane + 1);
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
