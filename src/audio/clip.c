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
#include "project.h"
#include "utils/audio.h"
#include "utils/math.h"
#include "utils/io.h"

#include <gtk/gtk.h>

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

  AudioEncoder * enc =
    audio_encoder_new_from_file (full_path);
  audio_encoder_decode (enc, 1);

  self->frames =
    calloc (
      (size_t) enc->num_out_frames * enc->channels,
      sizeof (float));
  self->num_frames = enc->num_out_frames;
  for (long i = 0;
       i < self->num_frames * enc->channels; i++)
    {
      self->frames[i] =
        enc->out_frames[i];
    }
  self->channels = enc->channels;
  self->name = io_path_get_basename (full_path);
  self->pool_id = -1;

  audio_encoder_free (enc);

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
      (size_t) (nframes * channels),
      sizeof (sample_t));
  self->num_frames = nframes;
  self->channels = channels;
  self->name = g_strdup (name);
  self->pool_id = -1;
  for (long i = 0; i < nframes * channels; i++)
    {
      self->frames[i] = (sample_t) arr[i];
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
    project_get_pool_dir (
      PROJECT);
  g_warn_if_fail (
    io_file_exists (prj_pool_dir));
  char * basename =
    g_strdup_printf (
      "%s.wav", self->name);
  char * new_path =
    g_build_filename (
      prj_pool_dir,
      basename,
      NULL);
  /*char * tmp;*/
  /*int i = 0;*/
  /*while (io_file_exists (new_path))*/
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
