// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "actions/undo_manager.h"
#include "audio/clip.h"
#include "audio/pool.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * Inits after loading a project.
 */
void
audio_pool_init_loaded (AudioPool * self)
{
  self->clips_size = (size_t) self->num_clips;

  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];
      if (clip)
        audio_clip_init_loaded (clip);
    }
}

/**
 * Creates a new audio pool.
 */
AudioPool *
audio_pool_new ()
{
  AudioPool * self = object_new (AudioPool);

  self->schema_version = AUDIO_POOL_SCHEMA_VERSION;

  self->clips_size = 2;
  self->clips = object_new_n (self->clips_size, AudioClip *);

  return self;
}

static bool
name_exists (AudioPool * self, const char * name)
{
  AudioClip * clip;
  for (int i = 0; i < self->num_clips; i++)
    {
      clip = self->clips[i];
      if (clip && string_is_equal (clip->name, name))
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
  bool   is_backup = false;
  char * orig_name_without_ext =
    io_file_strip_ext (clip->name);
  char * orig_path_in_pool =
    audio_clip_get_path_in_pool (clip, is_backup);
  char * new_name = g_strdup (orig_name_without_ext);

  bool changed = false;
  while (name_exists (self, new_name))
    {
      char *       prev_new_name = new_name;
      const char * regex = "^.*\\((\\d+)\\)$";
      char *       cur_val_str =
        string_get_regex_group (new_name, regex, 1);
      int cur_val =
        string_get_regex_group_as_int (new_name, regex, 1, 0);
      if (cur_val == 0)
        {
          new_name = g_strdup_printf ("%s (1)", new_name);
        }
      else
        {
          size_t len =
            strlen (new_name) -
            /* + 2 for the parens */
            (strlen (cur_val_str) + 2);
          /* + 1 for the terminating NULL */
          size_t tmp_len = len + 1;
          char   tmp[tmp_len];
          memset (tmp, 0, tmp_len * sizeof (char));
          memcpy (tmp, new_name, len - 1);
          new_name =
            g_strdup_printf ("%s (%d)", tmp, cur_val + 1);
        }
      g_free (cur_val_str);
      g_free (prev_new_name);
      changed = true;
    }

  char * new_path_in_pool =
    audio_clip_get_path_in_pool_from_name (
      new_name, clip->use_flac, is_backup);
  if (changed)
    {
      g_return_if_fail (!string_is_equal (
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
get_next_id (AudioPool * self)
{
  int next_id = -1;
  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];
      if (clip)
        {
          next_id = MAX (clip->pool_id, next_id);
        }
      else
        {
          return i;
        }
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
audio_pool_add_clip (AudioPool * self, AudioClip * clip)
{
  g_return_val_if_fail (clip && clip->name, -1);

  g_message ("adding clip <%s> to pool...", clip->name);

  array_double_size_if_full (
    self->clips, self->num_clips, self->clips_size,
    AudioClip *);

  audio_pool_ensure_unique_clip_name (self, clip);

  int next_id = get_next_id (self);
  g_return_val_if_fail (self->clips[next_id] == NULL, -1);

  clip->pool_id = next_id;
  self->clips[next_id] = clip;
  if (next_id == self->num_clips)
    self->num_clips++;

  g_message ("added clip <%s> to pool", clip->name);

  audio_pool_print (self);

  return clip->pool_id;
}

/**
 * Returns the clip for the given ID.
 */
AudioClip *
audio_pool_get_clip (AudioPool * self, int clip_id)
{
  g_return_val_if_fail (
    self && clip_id >= 0 && clip_id < self->num_clips, NULL);

  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];
      if (clip && clip->pool_id == clip_id)
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
 * @return The ID in the pool, or -1 if an error occurred.
 */
int
audio_pool_duplicate_clip (
  AudioPool * self,
  int         clip_id,
  bool        write_file,
  GError **   error)
{
  AudioClip * clip = audio_pool_get_clip (self, clip_id);
  g_return_val_if_fail (clip, -1);

  AudioClip * new_clip = audio_clip_new_from_float_array (
    clip->frames, clip->num_frames, clip->channels,
    clip->bit_depth, clip->name);
  audio_pool_add_clip (self, new_clip);

  g_message (
    "duplicating clip %s to %s...", clip->name,
    new_clip->name);

  /* assert clip names are not the same */
  g_return_val_if_fail (
    !string_is_equal (clip->name, new_clip->name), -1);

  if (write_file)
    {
      GError * err = NULL;
      bool     success = audio_clip_write_to_pool (
            new_clip, F_NO_PARTS, F_NOT_BACKUP, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s", "Failed to write clip to pool");
          return -1;
        }
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
  return g_strdup_printf (
    "%s - lane %d - recording", track->name,
    /* add 1 to get human friendly index */
    lane + 1);
}

/**
 * Removes the clip with the given ID from the pool
 * and optionally frees it (and removes the file).
 *
 * @param backup Whether to remove from backup
 *   directory.
 */
void
audio_pool_remove_clip (
  AudioPool * self,
  int         clip_id,
  bool        free_and_remove_file,
  bool        backup)
{
  g_message ("removing clip with ID %d", clip_id);

  AudioClip * clip = audio_pool_get_clip (self, clip_id);
  g_return_if_fail (clip);

  if (free_and_remove_file)
    {
      audio_clip_remove_and_free (clip, backup);
    }
  else
    {
      audio_clip_free (clip);
    }

  self->clips[clip_id] = NULL;
}

/**
 * Removes and frees (and removes the files for) all
 * clips not used by the project or undo stacks.
 *
 * @param backup Whether to remove from backup
 *   directory.
 */
void
audio_pool_remove_unused (AudioPool * self, bool backup)
{
  g_message ("--- removing unused files from pool ---");

  /* remove clips from the pool that are not in
   * use */
  int removed_clips = 0;
  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];

      if (clip && !audio_clip_is_in_use (clip, true))
        {
          g_message ("unused clip [%d]: %s", i, clip->name);
          audio_pool_remove_clip (self, i, F_FREE, backup);
          removed_clips++;
        }
    }

  /* remove untracked files from pool directory */
  char * prj_pool_dir =
    project_get_path (PROJECT, PROJECT_PATH_POOL, backup);
  char ** files = io_get_files_in_dir_ending_in (
    prj_pool_dir, 1, NULL, false);
  if (files)
    {
      for (size_t i = 0; files[i] != NULL; i++)
        {
          const char * path = files[i];

          bool found = false;
          for (int j = 0; j < self->num_clips; j++)
            {
              AudioClip * clip = self->clips[j];
              if (!clip)
                continue;

              char * clip_path =
                audio_clip_get_path_in_pool (clip, backup);

              if (string_is_equal (clip_path, path))
                {
                  found = true;
                  break;
                }

              g_free (clip_path);
            }

          /* if file not found in pool clips,
           * delete */
          if (!found)
            {
              io_remove (path);
            }
        }
      g_strfreev (files);
    }
  g_free (prj_pool_dir);

  g_message (
    "%s: done, removed %d clips", __func__, removed_clips);
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
audio_pool_reload_clip_frame_bufs (AudioPool * self)
{
  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];
      if (!clip)
        continue;

      bool in_use = audio_clip_is_in_use (clip, false);

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

typedef struct WriteClipData
{
  AudioClip * clip;
  bool        is_backup;

  /** To be set after writing the file. */
  bool     successful;
  GError * error;
} WriteClipData;

/**
 * Thread for writing an audio clip to disk.
 *
 * To be used as a GThreadFunc.
 */
static void
write_clip_thread (void * data, void * user_data)
{
  WriteClipData * write_clip_data = (WriteClipData *) data;
  write_clip_data->successful = audio_clip_write_to_pool (
    write_clip_data->clip, false, write_clip_data->is_backup,
    &write_clip_data->error);
}

static void
write_clip_data_free (void * data)
{
  WriteClipData * self = (WriteClipData *) data;
  object_zero_and_free (self);
}

/**
 * Writes all the clips to disk.
 *
 * Used when saving a project elsewhere.
 *
 * @param is_backup Whether this is a backup project.
 *
 * @return Whether successful.
 */
bool
audio_pool_write_to_disk (
  AudioPool * self,
  bool        is_backup,
  GError **   error)
{
  /* ensure pool dir exists */
  char * prj_pool_dir =
    project_get_path (PROJECT, PROJECT_PATH_POOL, is_backup);
  if (!file_exists (prj_pool_dir))
    {
      io_mkdir (prj_pool_dir);
    }
  g_free (prj_pool_dir);

  GError *      err = NULL;
  GThreadPool * thread_pool = g_thread_pool_new (
    write_clip_thread, self, (int) g_get_num_processors (),
    F_NOT_EXCLUSIVE, &err);
  if (err)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", "Failed to create thread pool");
      return false;
    }

  GPtrArray * clip_data_arr =
    g_ptr_array_new_with_free_func (write_clip_data_free);
  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];
      if (clip)
        {
          WriteClipData * data = object_new (WriteClipData);
          data->clip = clip;
          data->is_backup = is_backup;
          data->successful = false;
          data->error = NULL;
          g_ptr_array_add (clip_data_arr, data);

          /* start writing in a new thread */
          g_thread_pool_push (thread_pool, data, NULL);
        }
    }

  g_debug ("waiting for thread pool to finish...");
  g_thread_pool_free (thread_pool, false, true);
  g_debug ("done");

  for (size_t i = 0; i < clip_data_arr->len; i++)
    {
      WriteClipData * clip_data = (WriteClipData *)
        g_ptr_array_index (clip_data_arr, i);
      if (!clip_data->successful)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, clip_data->error,
            _ ("Failed to write clip %s"),
            clip_data->clip->name);
          clip_data->error = NULL;
          return false;
        }
    }

  g_ptr_array_unref (clip_data_arr);

  return true;
}

void
audio_pool_print (const AudioPool * const self)
{
  GString * gstr = g_string_new ("[Audio Pool]\n");
  for (int i = 0; i < self->num_clips; i++)
    {
      AudioClip * clip = self->clips[i];
      if (clip)
        {
          char * pool_path =
            audio_clip_get_path_in_pool (clip, F_NOT_BACKUP);
          g_string_append_printf (
            gstr, "[Clip #%d] %s (%s): %s\n", i, clip->name,
            clip->file_hash, pool_path);
          g_free (pool_path);
        }
      else
        {
          g_string_append_printf (
            gstr, "[Clip #%d] <empty>\n", i);
        }
    }
  char * str = g_string_free (gstr, false);
  g_message ("%s", str);
  g_free (str);
}

/**
 * To be used during serialization.
 */
AudioPool *
audio_pool_clone (const AudioPool * src)
{
  AudioPool * self = object_new (AudioPool);
  self->schema_version = AUDIO_POOL_SCHEMA_VERSION;

  self->clips =
    object_new_n ((size_t) src->num_clips, AudioClip *);
  for (int i = 0; i < src->num_clips; i++)
    {
      if (src->clips[i])
        self->clips[i] = audio_clip_clone (src->clips[i]);
    }
  self->num_clips = src->num_clips;

  return self;
}

void
audio_pool_free (AudioPool * self)
{
  for (int i = 0; i < self->num_clips; i++)
    {
      object_free_w_func_and_null (
        audio_clip_free, self->clips[i]);
    }
  object_zero_and_free (self->clips);

  object_zero_and_free (self);
}
