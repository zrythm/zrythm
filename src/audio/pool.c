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

#include "audio/clip.h"
#include "audio/pool.h"
#include "audio/track.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/mem.h"
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
  AudioPool * self = object_new (AudioPool);

  self->clips_size = 2;
  self->clips =
    object_new_n (self->clips_size, AudioClip *);

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
  char * orig_name_without_ext =
    io_file_strip_ext (clip->name);
  char * orig_path_in_pool =
    audio_clip_get_path_in_pool (clip);
  char * new_name =
    g_strdup (orig_name_without_ext);

  bool changed = false;
  while (name_exists (self, new_name))
    {
      g_free (new_name);
      new_name =
        g_strdup_printf (
          "%s (%d)", orig_name_without_ext, i++);
      changed = true;
    }

  char * new_path_in_pool =
    audio_clip_get_path_in_pool_from_name (
      new_name);
  if (changed)
    {
      g_return_if_fail (
        !string_is_equal (
          new_path_in_pool, orig_path_in_pool));
    }

  g_free (clip->name);
  g_free (orig_path_in_pool);
  g_free (new_path_in_pool);
  clip->name = new_name;
}

/**
 * Returns the next available ID.
 */
static int
get_next_id (
  AudioPool * self)
{
  int next_id = -1;
  for (int i = 0; i < self->num_clips; i++)
    {
      next_id =
        MAX (self->clips[i]->pool_id, next_id);
    }

  return next_id + 1;
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

  int next_id = get_next_id (self);
  clip->pool_id = next_id;

  array_append (
    self->clips, self->num_clips, clip);

  return clip->pool_id;
}

/**
 * Returns the clip for the given ID.
 */
AudioClip *
audio_pool_get_clip (
  AudioPool * self,
  int         clip_id)
{
  g_return_val_if_fail (
    self && clip_id >= 0 &&
    clip_id < self->num_clips, NULL);

  for (int i = 0; i < self->num_clips; i++)
    {
      if (self->clips[i]->pool_id == clip_id)
        {
          return self->clips[i];
        }
    }

  g_return_val_if_reached (NULL);
}

/**
 * Duplicates the clip with the given ID and returns
 * the duplicate.
 *
 * @param write_file Whether to also write the file.
 *
 * @return The ID in the pool.
 */
int
audio_pool_duplicate_clip (
  AudioPool * self,
  int         clip_id,
  bool        write_file)
{
  AudioClip * clip =
    audio_pool_get_clip (self, clip_id);
  g_return_val_if_fail (clip, -1);

  AudioClip * new_clip =
    audio_clip_new_from_float_array (
      clip->frames, clip->num_frames, clip->channels,
      clip->name);
  audio_pool_add_clip (self, new_clip);

  g_message (
    "duplicating clip %s to %s...",
    clip->name, new_clip->name);

  /* assert clip names are not the same */
  g_return_val_if_fail (
    !string_is_equal (clip->name, new_clip->name),
    -1);

  if (write_file)
    {
      audio_clip_write_to_pool (new_clip, false);
    }

  return new_clip->pool_id;
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

/**
 * Removes the clip with the given ID from the pool
 * and optionally frees it (and removes the file).
 */
void
audio_pool_remove_clip (
  AudioPool * self,
  int         clip_id,
  bool        free_and_remove_file)
{
  g_message ("removing clip with ID %d", clip_id);

  AudioClip * clip =
    audio_pool_get_clip (self, clip_id);
  g_return_if_fail (clip);

  audio_clip_remove_and_free (clip);

  for (int i = clip_id; i < self->num_clips - 1;
       i++)
    {
      self->clips[i] = self->clips[i + 1];
    }
  self->num_clips--;
}

/**
 * Loads the frame buffers of clips currently in
 * use in the project from their files and frees the
 * buffers of clips not currently in use.
 *
 * This should be called whenever there is a relevant
 * change in the project (eg, object added/removed).
 */
void
audio_pool_reload_clip_frame_bufs (
  AudioPool * self)
{
  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];
      bool in_use = audio_clip_is_in_use (clip);

      if (in_use && clip->num_frames == 0)
        {
          /* load from the file */
          audio_clip_init_loaded (clip);
        }
      else if (!in_use && clip->num_frames > 0)
        {
          /* unload frames */
          clip->num_frames = 0;
          free (clip->frames);
          clip->frames = NULL;
        }
    }
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
