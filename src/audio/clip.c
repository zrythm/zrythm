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
#include "audio/encoder.h"
#include "audio/engine.h"
#include "audio/tempo_track.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/file.h"
#include "utils/math.h"
#include "utils/io.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

static void
audio_clip_init_from_file (
  AudioClip * self,
  const char * full_path)
{
  AudioEncoder * enc =
    audio_encoder_new_from_file (full_path);
  audio_encoder_decode (enc, 1);

  size_t arr_size =
    (size_t) enc->num_out_frames *
    (size_t) enc->nfo.channels;
  self->frames =
    calloc (arr_size, sizeof (float));
  self->num_frames = enc->num_out_frames;
  memcpy (
    self->frames, enc->out_frames,
    arr_size * sizeof (float));
  self->name = g_path_get_basename (full_path);
  self->channels = enc->nfo.channels;
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);
  /*g_message (*/
    /*"\n\n num frames %ld \n\n", self->num_frames);*/

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
  self->bpm  = bpm;
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
  self->name = g_strdup (name);
  self->pool_id = -1;
  for (long i = 0; i < nframes * (long) channels;
		  i++)
    {
      self->frames[i] = (sample_t) arr[i];
    }
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);

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
  for (long i = 0; i < nframes * (long) channels;
       i++)
    {
      self->frames[i] = 0.f;
    }

  return self;
}

/**
 * Writes the clip to the pool as a wav file.
 */
void
audio_clip_write_to_pool (
  const AudioClip * self)
{
  /* generate a copy of the given filename in the
   * project dir */
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
  /*char * tmp;*/
  /*int i = 0;*/
  /*while (file_exists (new_path))*/
    /*{*/
      /*g_free (new_path);*/
      /*tmp =*/
        /*g_strdup_printf (*/
          /*"%s(%d)",*/
          /*basename, i++);*/
      /*new_path =*/
        /*g_build_filename (*/
          /*prj_pool_dir,*/
          /*tmp,*/
          /*NULL);*/
      /*g_free (tmp);*/
    /*}*/
  audio_clip_write_to_file (
    self, new_path);
  g_free (without_ext);
  g_free (basename);
  g_free (prj_pool_dir);
}

/**
 * Writes the given audio clip data to a file.
 */
void
audio_clip_write_to_file (
  const AudioClip * self,
  const char *      filepath)
{
  audio_write_raw_file (
    self->frames, self->num_frames,
    AUDIO_ENGINE->sample_rate,
    self->channels, filepath);
}

/**
 * Frees the audio clip.
 */
void
audio_clip_free (
  AudioClip * self)
{
  if (self->frames)
    free (self->frames);
  if (self->name)
    free (self->name);

  free (self);
}
