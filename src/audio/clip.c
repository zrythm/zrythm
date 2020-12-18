/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "project.h"
#include "utils/audio.h"
#include "utils/dsp.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/io.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Updates the channel caches.
 *
 * See @ref AudioClip.ch_frames.
 */
void
audio_clip_update_channel_caches (
  AudioClip * self)
{
  g_return_if_fail (
    self->channels > 0 && self->num_frames > 0);

  /* copy the frames to the channel caches */
  for (unsigned int i = 0; i < self->channels; i++)
    {
      self->ch_frames[i] =
        realloc (
          self->ch_frames[i],
          sizeof (float) *
            (size_t) self->num_frames);
      for (size_t j = 0;
           j < (size_t) self->num_frames; j++)
        {
          self->ch_frames[i][j] =
            self->frames[j * self->channels + i];
        }
    }
}

static void
audio_clip_init_from_file (
  AudioClip * self,
  const char * full_path)
{
  self->samplerate =
    (int) AUDIO_ENGINE->sample_rate;
  g_return_if_fail (self->samplerate > 0);

  AudioEncoder * enc =
    audio_encoder_new_from_file (full_path);
  audio_encoder_decode (
    enc, self->samplerate, F_SHOW_PROGRESS);


  size_t arr_size =
    (size_t) enc->num_out_frames *
    (size_t) enc->nfo.channels;
  self->frames =
    realloc (
      self->frames, arr_size * sizeof (float));
  self->num_frames = enc->num_out_frames;
  dsp_copy (
    self->frames, enc->out_frames, arr_size);
  if (self->name)
    {
      g_free (self->name);
    }
  self->name = g_path_get_basename (full_path);
  self->channels = enc->nfo.channels;
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
  /*g_message (*/
    /*"\n\n num frames %ld \n\n", self->num_frames);*/
  audio_clip_update_channel_caches (self);

  audio_encoder_free (enc);
}

/**
 * Inits after loading a Project.
 */
void
audio_clip_init_loaded (
  AudioClip * self)
{
  g_debug (
    "%s: %p", __func__, self);

  char * pool_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_POOL, false);
  char * noext =
    io_file_strip_ext (self->name);
  char * tmp =
    g_build_filename (
      pool_dir, noext, NULL);
  char * filepath =
    g_strdup_printf ("%s.wav", tmp);

  bpm_t bpm = self->bpm;
  audio_clip_init_from_file (self, filepath);
  self->bpm = bpm;
}

/**
 * Creates an audio clip from a file.
 *
 * The name used is the basename of the file.
 */
AudioClip *
audio_clip_new_from_file (
  const char * full_path)
{
  AudioClip * self =
    calloc (1, sizeof (AudioClip));

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
  const float *    arr,
  const long       nframes,
  const channels_t channels,
  const char *     name)
{
  AudioClip * self =
    calloc (1, sizeof (AudioClip));

  self->frames =
    calloc (
      (size_t) (nframes * (long) channels),
      sizeof (sample_t));
  self->num_frames = nframes;
  self->channels = channels;
  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  g_return_val_if_fail (self->samplerate > 0, NULL);
  self->name = g_strdup (name);
  self->pool_id = -1;
  dsp_copy (
    self->frames, arr,
    (size_t) nframes * (size_t) channels);
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
  audio_clip_update_channel_caches (self);

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
  const channels_t channels,
  const long       nframes,
  const char *     name)
{
  AudioClip * self =
    calloc (1, sizeof (AudioClip));

  self->channels = channels;
  self->frames =
    calloc (
      (size_t) (nframes * (long) self->channels),
      sizeof (sample_t));
  self->num_frames = nframes;
  self->name = g_strdup (name);
  self->pool_id = -1;
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
  self->samplerate = (int) AUDIO_ENGINE->sample_rate;
  g_return_val_if_fail (self->samplerate > 0, NULL);
  dsp_fill (
    self->frames, DENORMAL_PREVENTION_VAL,
    (size_t) nframes * (size_t) channels);
  audio_clip_update_channel_caches (self);

  return self;
}

static char *
get_path_in_pool (
  AudioClip * self)
{
  char * prj_pool_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_POOL, false);
  g_warn_if_fail (
    file_exists (prj_pool_dir));
  char * without_ext =
    io_file_strip_ext (self->name);
  char * basename =
    g_strdup_printf (
      "%s.wav", without_ext);
  char * new_path =
    g_build_filename (
      prj_pool_dir,
      basename,
      NULL);
  g_free (without_ext);
  g_free (basename);
  g_free (prj_pool_dir);

  return new_path;
}

/**
 * Writes the clip to the pool as a wav file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 */
void
audio_clip_write_to_pool (
  AudioClip * self,
  bool        parts)
{
  g_warn_if_fail (
    self ==
      audio_pool_get_clip (
        AUDIO_POOL, self->pool_id));

  /* generate a copy of the given filename in the
   * project dir */
  char * new_path = get_path_in_pool (self);
  audio_clip_write_to_file (
    self, new_path, parts);
  g_free (new_path);
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
  int ret =
    audio_write_raw_file (
      self->frames, parts ? self->frames_written : 0,
      self->num_frames,
      (uint32_t) self->samplerate,
      self->channels, filepath);
  audio_clip_update_channel_caches (self);

  if (parts && ret == 0)
    {
      self->frames_written = self->num_frames;
      self->last_write = g_get_monotonic_time ();
    }

  /* TODO error handling */

  return ret;
}

/**
 * Returns whether the clip is used inside the
 * project (in actual project regions only, not
 * in undo stack).
 */
bool
audio_clip_is_in_use (
  AudioClip * self)
{
  /* TODO */
  return true;
}

/**
 * To be called by audio_pool_remove_clip().
 *
 * Removes the file associated with the clip and
 * frees the instance.
 */
void
audio_clip_remove_and_free (
  AudioClip * self)
{
  char * path = get_path_in_pool (self);
  g_debug ("removing clip at %s", path);
  io_remove (path);

  audio_clip_free (self);
}

/**
 * Frees the audio clip.
 */
void
audio_clip_free (
  AudioClip * self)
{
  object_zero_and_free (self->frames);
  for (unsigned int i = 0; i < self->channels; i++)
    {
      object_zero_and_free (self->ch_frames[i]);
    }
  g_free_and_null (self->name);

  object_zero_and_free (self);
}
