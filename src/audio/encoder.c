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

#include <math.h>
#include <stdlib.h>

#include "audio/encoder.h"
#include "audio/engine.h"
#include "ext/audio_decoder/ad.h"
#include "project.h"

#include <gtk/gtk.h>

#include <samplerate.h>

typedef struct adinfo adinfo;

static long
src_cb (
  AudioEncoder * self,
  float **       audio)
{
	*audio = &(self->in_frames[0]);

  return self->num_in_frames;
}

/**
 * Creates a new instance of an AudioEncoder from
 * the given file, that can be used for encoding.
 *
 * It converts the file into a float array in its
 * original sample rate and prepares the instance
 * for decoding it into the project's sample rate.
 */
AudioEncoder *
audio_encoder_new_from_file (
  const char *    filepath)
{
  AudioEncoder * self =
    calloc (1, sizeof (AudioEncoder));

  /* read info */
  adinfo nfo;
  void * handle =
    ad_open (filepath, &nfo);
  self->num_in_frames = nfo.frames;
  self->channels = nfo.channels;
  size_t in_buff_size =
    (size_t) (nfo.frames * nfo.channels);
  self->in_frames =
    malloc (
      in_buff_size * sizeof (float));
  long samples_read =
    ad_read (handle,
             self->in_frames,
             in_buff_size);
  g_warn_if_fail (
    in_buff_size == (size_t) samples_read);

  /* get resample ratio */
  self->resample_ratio =
    (1.0 * AUDIO_ENGINE->sample_rate) /
    nfo.sample_rate;
  if (fabs (self->resample_ratio - 1.0) < 1e-20)
    {
      g_message ("Target samplerate and input "
                 "samplerate are the same.");
    }
  if (src_is_valid_ratio (self->resample_ratio) == 0)
    {
      g_warning ("Sample rate change out of valid "
                 "range.");
    }

  ad_close (handle);

  return self;
}

/**
 * Decodes the information in the AudioEncoder
 * instance and stores the results there.
 *
 * Resamples the input float array to match the
 * project's sample rate.
 *
 * Assumes that the input array is already filled in.
 *
 * @param show_progress Display a progress dialog.
 */
void
audio_encoder_decode (
  AudioEncoder * self,
  const int      show_progress)
{
  g_warn_if_fail (self->num_in_frames > 0);
  g_warn_if_fail (self->channels > 0);

  g_message ("--audio decoding start--");

  self->num_out_frames =
    (long)
    ((double) self->num_in_frames *
       self->resample_ratio);
  size_t out_buf_size =
    (size_t)
    (self->num_out_frames * self->channels);
  self->out_frames =
    malloc (
      out_buf_size *
      sizeof (float));
  /*g_message (*/
    /*"out_buff_size %ld, src ratio %f",*/
    /*out_buf_size,*/
    /*self->resample_ratio);*/

  int err;
  SRC_STATE * state =
    src_callback_new (
      (src_callback_t) src_cb,
      SRC_SINC_BEST_QUALITY,
      (int) self->channels, &err, self);
  if (!state)
    {
      g_warning (
        "Failed to create a src callback: %s",
        src_strerror (err));
      return;
    }

  /* start reading */
  long frames_read = -1;
  long total_read = 0;
  long frames_to_read = 0;
  do
    {
      frames_to_read =
        MIN (
          6000,
          (long) self->num_out_frames - total_read);
      /*g_message (*/
        /*"attempting to read %ld frames",*/
        /*frames_to_read);*/
      frames_read =
        src_callback_read (
          state, self->resample_ratio,
          frames_to_read,
          &self->out_frames[
            total_read * self->channels]);

      /* handle errors */
      int err_ret =
        src_error (state);
      if (err_ret)
        {
          g_warning (
            "An error occurred: %s",
            src_strerror (err_ret));
          break;
        }

      total_read += frames_read;
      /*g_message (*/
        /*"read %ld frames, %ld remaining",*/
        /*frames_read,*/
        /*(long) self->num_out_frames - total_read);*/

      if (frames_read == -1)
        break;

    } while (frames_read > 0);

  src_delete (state);
  g_message ("--audio decoding end--");

  if (total_read != self->num_out_frames)
    {
      g_warning (
        "Total frames read (%ld) and out frames "
        "expected (%ld) do not match",
        total_read, self->num_out_frames);
    }
  g_warn_if_fail (frames_read != -1);
}

/**
 * Free's the AudioEncoder and its members.
 */
void
audio_encoder_free (
  AudioEncoder * self)
{
  if (self->in_frames)
    free (self->in_frames);
  if (self->out_frames)
    free (self->out_frames);
  if (self->file)
    g_free (self->file);

  free (self);
}
