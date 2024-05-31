// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "dsp/clip.h"
#include "dsp/engine.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "gui/widgets/main_window.h"
#include "io/audio_file.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/hash.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/resampler.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

typedef enum
{
  Z_AUDIO_CLIP_ERROR_SAVING_FAILED,
  Z_AUDIO_CLIP_ERROR_CANCELLED,
} ZAudioClipError;

#define Z_AUDIO_CLIP_ERROR z_audio_clip_error_quark ()
GQuark
z_audio_clip_error_quark (void);
G_DEFINE_QUARK (
  z - audio - clip - error - quark,
  z_audio_clip_error)

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
audio_clip_update_channel_caches (AudioClip * self, size_t start_from)
{
  z_return_if_fail_cmp (self->channels, >, 0);
  z_return_if_fail_cmp (self->num_frames, >, 0);

  /* copy the frames to the channel caches */
  for (unsigned int i = 0; i < self->channels; i++)
    {
      self->ch_frames[i] = (float *) g_realloc (
        self->ch_frames[i], sizeof (float) * (size_t) self->num_frames);
      for (size_t j = start_from; j < (size_t) self->num_frames; j++)
        {
          self->ch_frames[i][j] = self->frames[j * self->channels + i];
        }
    }
}

static bool
audio_clip_init_from_file (
  AudioClip *  self,
  const char * full_path,
  GError **    error)
{
  g_return_val_if_fail (self, false);

  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  g_return_val_if_fail (self->samplerate > 0, false);

  GError * err = NULL;

  /* read metadata */
  AudioFile * af = audio_file_new (full_path);
  bool        success = audio_file_read_metadata (af, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Error reading metadata from %s", full_path);
      return false;
    }
  self->num_frames = (unsigned_frame_t) af->metadata.num_frames;
  self->channels = (channels_t) af->metadata.channels;
  switch (af->metadata.bit_depth)
    {
    case 16:
      self->bit_depth = BitDepth::BIT_DEPTH_16;
      break;
    case 24:
      self->bit_depth = BitDepth::BIT_DEPTH_24;
      break;
    case 32:
      self->bit_depth = BitDepth::BIT_DEPTH_32;
      break;
    default:
      g_debug ("unknown bit depth: %d", af->metadata.bit_depth);
      self->bit_depth = BitDepth::BIT_DEPTH_32;
    }

  /* read frames in file's sample rate */
  size_t arr_size = self->num_frames * self->channels;
  self->frames = (float *) g_realloc (self->frames, arr_size * sizeof (float));
  success = audio_file_read_samples (
    af, false, self->frames, 0, self->num_frames, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Error reading frames from %s", full_path);
      return false;
    }

  /* resample to project's sample rate */
  Resampler * r = resampler_new (
    self->frames, self->num_frames, af->metadata.samplerate, self->samplerate,
    self->channels, RESAMPLER_QUALITY_VERY_HIGH, &err);
  if (!r)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, "Failed to create resampler");
      return false;
    }
  while (!resampler_is_done (r))
    {
      success = resampler_process (r, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, "Resampler failed to process");
          return false;
        }
    }
  self->num_frames = r->num_out_frames;
  arr_size = self->num_frames * self->channels;
  self->frames = (float *) g_realloc (self->frames, arr_size * sizeof (float));
  dsp_copy (self->frames, r->out_frames, arr_size);
  object_free_w_func_and_null (resampler_free, r);

  success = audio_file_finish (af, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, "Failed to finish audio file usage");
      return false;
    }
  object_free_w_func_and_null (audio_file_free, af);

  g_free_and_null (self->name);
  char * basename = g_path_get_basename (full_path);
  self->name = io_file_strip_ext (basename);
  g_free (basename);
  self->bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);
  self->use_flac = audio_clip_use_flac (self->bit_depth);
  /*g_message (*/
  /*"\n\n num frames %ld \n\n", self->num_frames);*/
  audio_clip_update_channel_caches (self, 0);

  return true;
}

/**
 * Inits after loading a Project.
 */
bool
audio_clip_init_loaded (AudioClip * self, GError ** error)
{
  g_debug ("%s: %p", __func__, self);

  char * filepath = audio_clip_get_path_in_pool_from_name (
    self->name, self->use_flac, F_NOT_BACKUP);

  bpm_t    bpm = self->bpm;
  GError * err = NULL;
  bool     success = audio_clip_init_from_file (self, filepath, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, _ ("Failed to initialize audio file"));
      return false;
    }
  self->bpm = bpm;

  g_free (filepath);

  return true;
}

/**
 * Creates an audio clip from a file.
 *
 * The name used is the basename of the file.
 */
AudioClip *
audio_clip_new_from_file (const char * full_path, GError ** error)
{
  AudioClip * self = _create ();

  GError * err = NULL;
  bool     success = audio_clip_init_from_file (self, full_path, &err);
  if (!success)
    {
      audio_clip_free (self);
      PROPAGATE_PREFIXED_ERROR (
        error, err, _ ("Failed to initialize audio file at %s"), full_path);
      return NULL;
    }

  self->pool_id = -1;
  self->bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);

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

  self->frames = object_new_n ((size_t) (nframes * channels), sample_t);
  self->num_frames = nframes;
  self->channels = channels;
  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  g_return_val_if_fail (self->samplerate > 0, NULL);
  self->name = g_strdup (name);
  self->bit_depth = bit_depth;
  self->use_flac = audio_clip_use_flac (bit_depth);
  self->pool_id = -1;
  dsp_copy (self->frames, arr, (size_t) nframes * (size_t) channels);
  self->bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);
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
  self->frames = object_new_n ((size_t) (nframes * self->channels), sample_t);
  self->num_frames = nframes;
  self->name = g_strdup (name);
  self->pool_id = -1;
  self->bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);
  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  self->bit_depth = BitDepth::BIT_DEPTH_32;
  self->use_flac = false;
  g_return_val_if_fail (self->samplerate > 0, NULL);
  dsp_fill (
    self->frames, DENORMAL_PREVENTION_VAL (AUDIO_ENGINE),
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
  char * prj_pool_dir =
    project_get_path (PROJECT, ProjectPath::PROJECT_PATH_POOL, is_backup);
  if (!file_exists (prj_pool_dir))
    {
      g_critical ("%s does not exist", prj_pool_dir);
      return NULL;
    }
  char * without_ext = io_file_strip_ext (name);
  char * basename =
    g_strdup_printf ("%s.%s", without_ext, use_flac ? "FLAC" : "wav");
  char * new_path = g_build_filename (prj_pool_dir, basename, NULL);
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
audio_clip_get_path_in_pool (AudioClip * self, bool is_backup)
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
 *
 * @return Whether successful.
 */
bool
audio_clip_write_to_pool (
  AudioClip * self,
  bool        parts,
  bool        is_backup,
  GError **   error)
{
  AudioClip * pool_clip = audio_pool_get_clip (AUDIO_POOL, self->pool_id);
  g_return_val_if_fail (pool_clip, false);
  g_return_val_if_fail (pool_clip == self, false);

  audio_pool_print (AUDIO_POOL);
  g_message (
    "attempting to write clip %s (%d) to pool...", self->name, self->pool_id);

  /* generate a copy of the given filename in the
   * project dir */
  char * path_in_main_project = audio_clip_get_path_in_pool (self, F_NOT_BACKUP);
  char * new_path = audio_clip_get_path_in_pool (self, is_backup);
  g_return_val_if_fail (path_in_main_project, false);
  g_return_val_if_fail (new_path, false);

  /* whether a new write is needed */
  bool need_new_write = true;

  /* skip if file with same hash already exists */
  if (file_exists (new_path) && !parts)
    {
      char * existing_file_hash =
        hash_get_from_file (new_path, HASH_ALGORITHM_XXH3_64);
      bool same_hash =
        self->file_hash && string_is_equal (self->file_hash, existing_file_hash);
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
            hash_get_from_file (path_in_main_project, HASH_ALGORITHM_XXH3_64);
          exists_in_main_project =
            string_is_equal (self->file_hash, existing_file_hash);
          g_free (existing_file_hash);
        }

      if (exists_in_main_project)
        {
          /* try reflink */
          g_debug (
            "reflinking clip from main project "
            "('%s' to '%s')",
            path_in_main_project, new_path);

          if (file_reflink (path_in_main_project, new_path) != 0)
            {
              g_message (
                "failed to reflink, copying "
                "instead");

              /* copy */
              GFile *  src_file = g_file_new_for_path (path_in_main_project);
              GFile *  dest_file = g_file_new_for_path (new_path);
              GError * err = NULL;
              g_debug (
                "copying clip from main project "
                "('%s' to '%s')",
                path_in_main_project, new_path);
              if (
                g_file_copy (
                  src_file, dest_file, (GFileCopyFlags) 0, NULL, NULL, NULL,
                  &err))
                {
                  need_new_write = false;
                }
              else /* else if failed */
                {
                  g_warning (
                    "Failed to copy '%s' to '%s': %s", path_in_main_project,
                    new_path, err->message);
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
      GError * err = NULL;
      bool     success = audio_clip_write_to_file (self, new_path, parts, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, _ ("Failed to write audio clip to file: %s"), new_path);
          return false;
        }

      if (!parts)
        {
          /* store file hash */
          g_free_and_null (self->file_hash);
          self->file_hash =
            hash_get_from_file (new_path, HASH_ALGORITHM_XXH3_64);
        }
    }

  g_free (path_in_main_project);
  g_free (new_path);

  audio_pool_print (AUDIO_POOL);

  return true;
}

/**
 * Writes the given audio clip data to a file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 *
 * @return Whether successful.
 */
bool
audio_clip_write_to_file (
  AudioClip *  self,
  const char * filepath,
  bool         parts,
  GError **    error)
{
  g_return_val_if_fail (self->samplerate > 0, false);
  g_return_val_if_fail (self->frames_written < SIZE_MAX, false);
  size_t           before_frames = (size_t) self->frames_written;
  unsigned_frame_t ch_offset = parts ? self->frames_written : 0;
  unsigned_frame_t offset = ch_offset * self->channels;

  size_t nframes;
  if (parts)
    {
      z_return_val_if_fail_cmp (
        self->num_frames, >=, self->frames_written, false);
      unsigned_frame_t _nframes = self->num_frames - self->frames_written;
      z_return_val_if_fail_cmp (_nframes, <, SIZE_MAX, false);
      nframes = _nframes;
    }
  else
    {
      z_return_val_if_fail_cmp (self->num_frames, <, SIZE_MAX, false);
      nframes = self->num_frames;
    }
  GError * err = NULL;
  bool     success = audio_write_raw_file (
    &self->frames[offset], ch_offset, nframes, (uint32_t) self->samplerate,
    self->use_flac, self->bit_depth, self->channels, filepath, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, _ ("Failed to write audio file"));
      return false;
    }
  audio_clip_update_channel_caches (self, before_frames);

  if (parts && success)
    {
      self->frames_written = self->num_frames;
      self->last_write = g_get_monotonic_time ();
    }

  /* TODO move this to a unit test for this
   * function */
  if (ZRYTHM_TESTING)
    {
      err = NULL;
      AudioClip * new_clip = audio_clip_new_from_file (filepath, &err);
      if (!new_clip)
        {
          PROPAGATE_PREFIXED_ERROR_LITERAL (
            error, err, _ ("Failed to create audio clip from file"));
          return false;
        }

      if (self->num_frames != new_clip->num_frames)
        {
          g_warning ("%zu != %zu", self->num_frames, new_clip->num_frames);
        }
      float epsilon = 0.0001f;
      g_warn_if_fail (audio_frames_equal (
        self->ch_frames[0], new_clip->ch_frames[0],
        (size_t) new_clip->num_frames, epsilon));
      g_warn_if_fail (audio_frames_equal (
        self->frames, new_clip->frames,
        (size_t) new_clip->num_frames * new_clip->channels, epsilon));
      audio_clip_free (new_clip);
    }

  return true;
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
audio_clip_is_in_use (AudioClip * self, bool check_undo_stack)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->type != TrackType::TRACK_TYPE_AUDIO)
        continue;

      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              Region * r = lane->regions[k];
              if (r->id.type != RegionType::REGION_TYPE_AUDIO)
                continue;

              if (r->pool_id == self->pool_id)
                return true;
            }
        }
    }

  if (check_undo_stack)
    {
      if (undo_manager_contains_clip (UNDO_MANAGER, self))
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
app_launch_data_free (AppLaunchData * data, GClosure * closure)
{
  free (data);
}

static void
on_launch_clicked (GtkButton * btn, AppLaunchData * data)
{
  GError * err = NULL;
  GList *  file_list = NULL;
  file_list = g_list_append (file_list, data->file);
  GAppInfo * app_nfo = gtk_app_chooser_get_app_info (data->app_chooser);
  bool       success = g_app_info_launch (app_nfo, file_list, NULL, &err);
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
audio_clip_edit_in_ext_program (AudioClip * self, GError ** error)
{
  GError * err = NULL;
  char *   tmp_dir = g_dir_make_tmp ("zrythm-audio-clip-tmp-XXXXXX", &err);
  if (!tmp_dir)
    {
      PROPAGATE_PREFIXED_ERROR (error, err, "%s", "Failed to create tmp dir");
      return NULL;
    }
  char * abs_path = g_build_filename (tmp_dir, "tmp.wav", NULL);
  bool   success = audio_clip_write_to_file (self, abs_path, false, &err);
  audio_clip_free (self);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", "Failed to write clip to file");
      return NULL;
    }

  GFile *     file = g_file_new_for_path (abs_path);
  GFileInfo * file_info = g_file_query_info (
    file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL,
    &err);
  if (!file_info)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Failed to query file info for %s", abs_path);
      return NULL;
    }
  const char * content_type = g_file_info_get_content_type (file_info);

  GtkWidget * dialog = gtk_dialog_new_with_buttons (
    _ ("Edit in external app"), GTK_WINDOW (MAIN_WINDOW),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, _ ("_OK"),
    GTK_RESPONSE_ACCEPT, _ ("_Cancel"), GTK_RESPONSE_REJECT, NULL);

  /* populate content area */
  GtkWidget * content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget * main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
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

  GtkWidget * launch_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_halign (launch_box, GTK_ALIGN_CENTER);
  GtkWidget * app_chooser_button = gtk_app_chooser_button_new (content_type);
  gtk_box_append (GTK_BOX (launch_box), app_chooser_button);
  GtkWidget *     btn = gtk_button_new_with_label (_ ("Launch"));
  AppLaunchData * data = object_new (AppLaunchData);
  data->file = file;
  data->app_chooser = GTK_APP_CHOOSER (app_chooser_button);
  g_signal_connect_data (
    G_OBJECT (btn), "clicked", G_CALLBACK (on_launch_clicked), data,
    (GClosureNotify) app_launch_data_free, (GConnectFlags) 0);
  gtk_box_append (GTK_BOX (launch_box), btn);
  gtk_box_append (GTK_BOX (main_box), launch_box);

  int ret = z_gtk_dialog_run (GTK_DIALOG (dialog), true);
  if (ret != GTK_RESPONSE_ACCEPT)
    {
      g_set_error_literal (
        error, Z_AUDIO_CLIP_ERROR, Z_AUDIO_CLIP_ERROR_CANCELLED,
        "Operation cancelled");
      return NULL;
    }

  /* ok - reload from file */
  err = NULL;
  self = audio_clip_new_from_file (abs_path, &err);
  if (!self)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, _ ("Failed to create audio clip from file at %s"), abs_path);
      return NULL;
    }

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
audio_clip_remove_and_free (AudioClip * self, bool backup)
{
  char * path = audio_clip_get_path_in_pool (self, backup);
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
      object_zero_and_free_if_nonnull (self->ch_frames[i]);
    }
  g_free_and_null (self->name);
  g_free_and_null (self->file_hash);

  object_zero_and_free (self);
}
