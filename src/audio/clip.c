/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/encoder.h"
#include "audio/engine.h"
#include "audio/tempo_track.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/hash.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static AudioClip *
_create (void)
{
  AudioClip * self = object_new (AudioClip);
  self->schema_version = AUDIO_CLIP_SCHEMA_VERSION;

  return self;
}

/**
 * Updates the channel caches.
 *
 * See @ref AudioClip.ch_frames.
 *
 * @param start_from Frames to start from (per
 *   channel. The previous frames will be kept.
 */
void
audio_clip_update_channel_caches (
  AudioClip * self,
  size_t      start_from)
{
  z_return_if_fail_cmp (self->channels, >, 0);
  z_return_if_fail_cmp (self->num_frames, >, 0);

  /* copy the frames to the channel caches */
  for (unsigned int i = 0; i < self->channels; i++)
    {
      self->ch_frames[i] = g_realloc (
        self->ch_frames[i],
        sizeof (float) * (size_t) self->num_frames);
      for (size_t j = start_from;
           j < (size_t) self->num_frames; j++)
        {
          self->ch_frames[i][j] =
            self->frames[j * self->channels + i];
        }
    }
}

static void
audio_clip_init_from_file (
  AudioClip *  self,
  const char * full_path)
{
  g_return_if_fail (self);

  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  g_return_if_fail (self->samplerate > 0);

  AudioEncoder * enc =
    audio_encoder_new_from_file (full_path);
  audio_encoder_decode (
    enc, self->samplerate, F_SHOW_PROGRESS);

  size_t arr_size =
    (size_t) enc->num_out_frames
    * (size_t) enc->nfo.channels;
  self->frames = g_realloc (
    self->frames, arr_size * sizeof (float));
  self->num_frames = enc->num_out_frames;
  dsp_copy (self->frames, enc->out_frames, arr_size);
  g_free_and_null (self->name);
  char * basename = g_path_get_basename (full_path);
  self->name = io_file_strip_ext (basename);
  g_free (basename);
  self->channels = enc->nfo.channels;
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
  switch (enc->nfo.bit_depth)
    {
    case 16:
      self->bit_depth = BIT_DEPTH_16;
      self->use_flac = true;
      break;
    case 24:
      self->bit_depth = BIT_DEPTH_24;
      self->use_flac = true;
      break;
    case 32:
      self->bit_depth = BIT_DEPTH_32;
      self->use_flac = false;
      break;
    default:
      g_debug (
        "unknown bit depth: %d", enc->nfo.bit_depth);
      self->bit_depth = BIT_DEPTH_32;
      self->use_flac = false;
    }
  /*g_message (*/
  /*"\n\n num frames %ld \n\n", self->num_frames);*/
  audio_clip_update_channel_caches (self, 0);

  audio_encoder_free (enc);
}

/**
 * Inits after loading a Project.
 */
void
audio_clip_init_loaded (AudioClip * self)
{
  g_debug ("%s: %p", __func__, self);

  char * filepath =
    audio_clip_get_path_in_pool_from_name (
      self->name, self->use_flac, F_NOT_BACKUP);

  bpm_t bpm = self->bpm;
  audio_clip_init_from_file (self, filepath);
  self->bpm = bpm;

  g_free (filepath);
}

/**
 * Creates an audio clip from a file.
 *
 * The name used is the basename of the file.
 */
AudioClip *
audio_clip_new_from_file (const char * full_path)
{
  AudioClip * self = _create ();

  audio_clip_init_from_file (self, full_path);

  self->pool_id = -1;
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);

  return self;
}

/**
 * Creates an audio clip by copying the given float
 * array.
 *
 * @param name A name for this clip.
 */
AudioClip *
audio_clip_new_from_float_array (
  const float *          arr,
  const unsigned_frame_t nframes,
  const channels_t       channels,
  BitDepth               bit_depth,
  const char *           name)
{
  AudioClip * self = _create ();

  self->frames = object_new_n (
    (size_t) (nframes * channels), sample_t);
  self->num_frames = nframes;
  self->channels = channels;
  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  g_return_val_if_fail (self->samplerate > 0, NULL);
  self->name = g_strdup (name);
  self->bit_depth = bit_depth;
  self->use_flac = bit_depth < BIT_DEPTH_32;
  self->pool_id = -1;
  dsp_copy (
    self->frames, arr,
    (size_t) nframes * (size_t) channels);
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
  audio_clip_update_channel_caches (self, 0);

  return self;
}

/**
 * Create an audio clip while recording.
 *
 * The frames will keep getting reallocated until
 * the recording is finished.
 *
 * @param nframes Number of frames to allocate. This
 *   should be the current cycle's frames when
 *   called during recording.
 */
AudioClip *
audio_clip_new_recording (
  const channels_t       channels,
  const unsigned_frame_t nframes,
  const char *           name)
{
  AudioClip * self = _create ();

  self->channels = channels;
  self->frames = object_new_n (
    (size_t) (nframes * self->channels), sample_t);
  self->num_frames = nframes;
  self->name = g_strdup (name);
  self->pool_id = -1;
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  self->bit_depth = BIT_DEPTH_32;
  self->use_flac = false;
  g_return_val_if_fail (self->samplerate > 0, NULL);
  dsp_fill (
    self->frames, DENORMAL_PREVENTION_VAL,
    (size_t) nframes * (size_t) channels);
  audio_clip_update_channel_caches (self, 0);

  return self;
}

/**
 * Gets the path of a clip matching \ref name from
 * the pool.
 *
 * @param use_flac Whether to look for a FLAC file
 *   instead of a wav file.
 * @param is_backup Whether writing to a backup
 *   project.
 */
char *
audio_clip_get_path_in_pool_from_name (
  const char * name,
  bool         use_flac,
  bool         is_backup)
{
  char * prj_pool_dir = project_get_path (
    PROJECT, PROJECT_PATH_POOL, is_backup);
  if (!file_exists (prj_pool_dir))
    {
      g_critical ("%s does not exist", prj_pool_dir);
      return NULL;
    }
  char * without_ext = io_file_strip_ext (name);
  char * basename = g_strdup_printf (
    "%s.%s", without_ext, use_flac ? "FLAC" : "wav");
  char * new_path =
    g_build_filename (prj_pool_dir, basename, NULL);
  g_free (without_ext);
  g_free (basename);
  g_free (prj_pool_dir);

  return new_path;
}

/**
 * Gets the path of the given clip from the pool.
 *
 * @param is_backup Whether writing to a backup
 *   project.
 */
char *
audio_clip_get_path_in_pool (
  AudioClip * self,
  bool        is_backup)
{
  return audio_clip_get_path_in_pool_from_name (
    self->name, self->use_flac, is_backup);
}

/**
 * Writes the clip to the pool as a wav file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 * @param is_backup Whether writing to a backup
 *   project.
 */
void
audio_clip_write_to_pool (
  AudioClip * self,
  bool        parts,
  bool        is_backup)
{
  AudioClip * pool_clip =
    audio_pool_get_clip (AUDIO_POOL, self->pool_id);
  g_return_if_fail (pool_clip);
  g_return_if_fail (pool_clip == self);

  audio_pool_print (AUDIO_POOL);
  g_message (
    "attempting to write clip %s (%d) to pool...",
    self->name, self->pool_id);

  /* generate a copy of the given filename in the
   * project dir */
  char * path_in_main_project =
    audio_clip_get_path_in_pool (self, F_NOT_BACKUP);
  char * new_path =
    audio_clip_get_path_in_pool (self, is_backup);
  g_return_if_fail (path_in_main_project);
  g_return_if_fail (new_path);

  /* whether a new write is needed */
  bool need_new_write = true;

  /* skip if file with same hash already exists */
  if (file_exists (new_path) && !parts)
    {
      char * existing_file_hash = hash_get_from_file (
        new_path, HASH_ALGORITHM_XXH3_64);
      bool same_hash =
        self->file_hash
        && string_is_equal (
          self->file_hash, existing_file_hash);
      g_free (existing_file_hash);

      if (same_hash)
        {
          g_debug (
            "skipping writing to existing clip %s "
            "in pool",
            new_path);
          need_new_write = false;
        }
#if 0
      else
        {
          g_critical (
            "attempted to overwrite %s with a "
            "different clip",
            new_path);
          return;
        }
#endif
    }

  /* if writing to backup and same file exists in
   * main project dir, copy (first try reflink) */
  if (need_new_write && self->file_hash && is_backup)
    {
      bool exists_in_main_project = false;
      if (file_exists (path_in_main_project))
        {
          char * existing_file_hash =
            hash_get_from_file (
              path_in_main_project,
              HASH_ALGORITHM_XXH3_64);
          exists_in_main_project = string_is_equal (
            self->file_hash, existing_file_hash);
          g_free (existing_file_hash);
        }

      if (exists_in_main_project)
        {
          /* try reflink */
          g_debug (
            "reflinking clip from main project "
            "('%s' to '%s')",
            path_in_main_project, new_path);

          if (
            file_reflink (
              path_in_main_project, new_path)
            != 0)
            {
              g_message (
                "failed to reflink, copying "
                "instead");

              /* copy */
              GFile * src_file = g_file_new_for_path (
                path_in_main_project);
              GFile * dest_file =
                g_file_new_for_path (new_path);
              GError * err = NULL;
              g_debug (
                "copying clip from main project "
                "('%s' to '%s')",
                path_in_main_project, new_path);
              if (g_file_copy (
                    src_file, dest_file, 0, NULL,
                    NULL, NULL, &err))
                {
                  need_new_write = false;
                }
              else /* else if failed */
                {
                  g_warning (
                    "Failed to copy '%s' to '%s': %s",
                    path_in_main_project, new_path,
                    err->message);
                }
            } /* endif reflink fail */
        }
    }

  if (need_new_write)
    {
      g_debug (
        "writing clip %s to pool "
        "(parts %d, is backup  %d): '%s'",
        self->name, parts, is_backup, new_path);
      audio_clip_write_to_file (
        self, new_path, parts);

      if (!parts)
        {
          /* store file hash */
          g_free_and_null (self->file_hash);
          self->file_hash = hash_get_from_file (
            new_path, HASH_ALGORITHM_XXH3_64);
        }
    }

  g_free (path_in_main_project);
  g_free (new_path);

  audio_pool_print (AUDIO_POOL);
}

/**
 * Writes the given audio clip data to a file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 *
 * @return Non-zero if fail.
 */
int
audio_clip_write_to_file (
  AudioClip *  self,
  const char * filepath,
  bool         parts)
{
  g_return_val_if_fail (self->samplerate > 0, -1);
  size_t before_frames =
    (size_t) self->frames_written;
  unsigned_frame_t ch_offset =
    parts ? self->frames_written : 0;
  unsigned_frame_t offset =
    ch_offset * self->channels;
  int ret = audio_write_raw_file (
    &self->frames[offset], ch_offset,
    parts
      ? (self->num_frames - self->frames_written)
      : self->num_frames,
    (uint32_t) self->samplerate, self->use_flac,
    self->bit_depth, self->channels, filepath);
  audio_clip_update_channel_caches (
    self, before_frames);

  if (parts && ret == 0)
    {
      self->frames_written = self->num_frames;
      self->last_write = g_get_monotonic_time ();
    }

  /* TODO move this to a unit test for this
   * function */
  if (ZRYTHM_TESTING)
    {
      AudioClip * new_clip =
        audio_clip_new_from_file (filepath);
      if (self->num_frames != new_clip->num_frames)
        {
          g_warning (
            "%zu != %zu", self->num_frames,
            new_clip->num_frames);
        }
      float epsilon = 0.0001f;
      g_warn_if_fail (audio_frames_equal (
        self->ch_frames[0], new_clip->ch_frames[0],
        (size_t) new_clip->num_frames, epsilon));
      g_warn_if_fail (audio_frames_equal (
        self->frames, new_clip->frames,
        (size_t) new_clip->num_frames
          * new_clip->channels,
        epsilon));
      audio_clip_free (new_clip);
    }

  return ret;
}

/**
 * Returns whether the clip is used inside the
 * project.
 *
 * @param check_undo_stack If true, this checks both
 *   project regions and the undo stack. If false,
 *   this only checks actual project regions only.
 */
bool
audio_clip_is_in_use (
  AudioClip * self,
  bool        check_undo_stack)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->type != TRACK_TYPE_AUDIO)
        continue;

      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              ZRegion * r = lane->regions[k];
              if (r->id.type != REGION_TYPE_AUDIO)
                continue;

              if (r->pool_id == self->pool_id)
                return true;
            }
        }
    }

  if (check_undo_stack)
    {
      if (undo_manager_contains_clip (
            UNDO_MANAGER, self))
        {
          return true;
        }
    }

  return false;
}

typedef struct AppLaunchData
{
  GFile *         file;
  GtkAppChooser * app_chooser;
} AppLaunchData;

static void
app_launch_data_free (
  AppLaunchData * data,
  GClosure *      closure)
{
  free (data);
}

static void
on_launch_clicked (
  GtkButton *     btn,
  AppLaunchData * data)
{
  GError * err = NULL;
  GList *  file_list = NULL;
  file_list = g_list_append (file_list, data->file);
  GAppInfo * app_nfo =
    gtk_app_chooser_get_app_info (data->app_chooser);
  bool success = g_app_info_launch (
    app_nfo, file_list, NULL, &err);
  g_list_free (file_list);
  if (!success)
    {
      g_message ("app launch unsuccessful");
    }
}

/**
 * Shows a dialog with info on how to edit a file,
 * with an option to open an app launcher.
 *
 * When the user closes the dialog, the clip is
 * assumed to have been edited.
 *
 * The given audio clip will be free'd.
 *
 * @note This must not be used on pool clips.
 *
 * @return A new instance of AudioClip if successful,
 *   NULL, if not.
 */
AudioClip *
audio_clip_edit_in_ext_program (AudioClip * self)
{
  GError * err = NULL;
  char *   tmp_dir = g_dir_make_tmp (
      "zrythm-audio-clip-tmp-XXXXXX", &err);
  g_return_val_if_fail (tmp_dir, NULL);
  char * abs_path =
    g_build_filename (tmp_dir, "tmp.wav", NULL);
  audio_clip_write_to_file (self, abs_path, false);
  audio_clip_free (self);

  GFile * file = g_file_new_for_path (abs_path);
  err = NULL;
  GFileInfo * file_info = g_file_query_info (
    file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
    G_FILE_QUERY_INFO_NONE, NULL, &err);
  g_return_val_if_fail (file_info, false);
  const char * content_type =
    g_file_info_get_content_type (file_info);

  GtkWidget * dialog = gtk_dialog_new_with_buttons (
    _ ("Edit in external app"),
    GTK_WINDOW (MAIN_WINDOW),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    _ ("_OK"), GTK_RESPONSE_ACCEPT, _ ("_Cancel"),
    GTK_RESPONSE_REJECT, NULL);

  /* populate content area */
  GtkWidget * content_area =
    gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget * main_box =
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start (main_box, 4);
  gtk_widget_set_margin_end (main_box, 4);
  gtk_widget_set_margin_top (main_box, 4);
  gtk_widget_set_margin_bottom (main_box, 4);
  gtk_box_append (GTK_BOX (content_area), main_box);

  GtkWidget * lbl = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (lbl), true);
  char * markup = g_markup_printf_escaped (
    _ ("Edit the file at <u>%s</u>, then "
       "press OK"),
    abs_path);
  gtk_label_set_markup (GTK_LABEL (lbl), markup);
  gtk_box_append (GTK_BOX (main_box), lbl);

  GtkWidget * launch_box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_halign (
    launch_box, GTK_ALIGN_CENTER);
  GtkWidget * app_chooser_button =
    gtk_app_chooser_button_new (content_type);
  gtk_box_append (
    GTK_BOX (launch_box), app_chooser_button);
  GtkWidget * btn =
    gtk_button_new_with_label (_ ("Launch"));
  AppLaunchData * data = object_new (AppLaunchData);
  data->file = file;
  data->app_chooser =
    GTK_APP_CHOOSER (app_chooser_button);
  g_signal_connect_data (
    G_OBJECT (btn), "clicked",
    G_CALLBACK (on_launch_clicked), data,
    (GClosureNotify) app_launch_data_free, 0);
  gtk_box_append (GTK_BOX (launch_box), btn);
  gtk_box_append (GTK_BOX (main_box), launch_box);

  int ret =
    z_gtk_dialog_run (GTK_DIALOG (dialog), true);
  if (ret != GTK_RESPONSE_ACCEPT)
    {
      g_debug ("cancelled");
      return NULL;
    }

  /* ok - reload from file */
  self = audio_clip_new_from_file (abs_path);
  g_return_val_if_fail (self, false);

  return self;
}

/**
 * To be called by audio_pool_remove_clip().
 *
 * Removes the file associated with the clip and
 * frees the instance.
 *
 * @param backup Whether to remove from backup
 *   directory.
 */
void
audio_clip_remove_and_free (
  AudioClip * self,
  bool        backup)
{
  char * path = audio_clip_get_path_in_pool (
    self, F_NOT_BACKUP);
  g_message ("removing clip at %s", path);
  g_return_if_fail (path);
  io_remove (path);

  audio_clip_free (self);
}

AudioClip *
audio_clip_clone (AudioClip * src)
{
  AudioClip * self = object_new (AudioClip);
  self->schema_version = AUDIO_CLIP_SCHEMA_VERSION;

  self->name = g_strdup (src->name);
  self->file_hash = g_strdup (src->file_hash);
  self->bpm = src->bpm;
  self->bit_depth = src->bit_depth;
  self->use_flac = src->use_flac;
  self->samplerate = src->samplerate;
  self->pool_id = src->pool_id;

  return self;
}

/**
 * Frees the audio clip.
 */
void
audio_clip_free (AudioClip * self)
{
  object_zero_and_free (self->frames);
  for (unsigned int i = 0; i < self->channels; i++)
    {
      object_zero_and_free_if_nonnull (
        self->ch_frames[i]);
    }
  g_free_and_null (self->name);
  g_free_and_null (self->file_hash);

  object_zero_and_free (self);
}
