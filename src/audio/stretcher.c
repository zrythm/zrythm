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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2018 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdlib.h>

#include "audio/stretcher.h"
#include "utils/math.h"

#include <gtk/gtk.h>

/**
 * Create a new Stretcher using the rubberband
 * backend.
 *
 * @param samplerate The new samplerate.
 * @param time_ratio The ratio to multiply time by (eg
 *   if the BPM is doubled, this will be 0.5).
 * @param pitch_ratio The ratio to pitch by. This will
 *   normally be 1.0 when time-stretching).
 */
Stretcher *
stretcher_new_rubberband (
  unsigned int   samplerate,
  unsigned int   channels,
  double         time_ratio,
  double         pitch_ratio)
{
  Stretcher * self = calloc (1, sizeof (Stretcher));

  self->backend = STRETCHER_BACKEND_RUBBERBAND;
  self->samplerate = samplerate;
  self->channels = channels;
  self->block_size = 6000;
  self->rubberband_state =
    rubberband_new (
      samplerate, channels,
      RubberBandOptionProcessOffline |
        RubberBandOptionStretchElastic |
        RubberBandOptionTransientsCrisp |
        RubberBandOptionDetectorCompound |
        RubberBandOptionPhaseLaminar |
        RubberBandOptionThreadingNever |
        RubberBandOptionWindowStandard |
        RubberBandOptionSmoothingOff |
        RubberBandOptionFormantShifted |
        RubberBandOptionPitchHighQuality |
        RubberBandOptionChannelsApart,
      time_ratio, pitch_ratio);
  rubberband_set_default_debug_level (0);
  rubberband_set_max_process_size (
    self->rubberband_state, self->block_size);

  g_message ("time ratio: %f", time_ratio);

  return self;
}

/**
 * Perform stretching.
 *
 * @param in_samples_l The left samples.
 * @param in_samples_r The right channel samples. If
 *   this is NULL, the audio is assumed to be mono.
 * @param in_samples_size The number of input samples
 *   per channel.
 */
ssize_t
stretcher_stretch (
  Stretcher * self,
  float *     in_samples_l,
  float *     in_samples_r,
  size_t      in_samples_size,
  float *     out_samples_l,
  float *     out_samples_r)
{
  g_return_val_if_fail (in_samples_l, -1);

  /* FIXME this function is wrong */
  g_return_val_if_reached (-1);

  /* create the de-interleaved array */
  unsigned int channels = in_samples_r ? 2 : 1;
  g_return_val_if_fail (
    self->channels != channels, -1);
  const float * in_samples[channels];
  in_samples[0] = in_samples_l;
  if (channels == 2)
    in_samples[1] = in_samples_r;

  /* study and process */
  rubberband_study (
    self->rubberband_state, in_samples,
    in_samples_size, 1);
  rubberband_process (
    self->rubberband_state, in_samples,
    in_samples_size, 1);

  /* get the output data */
  int available_out_samples =
    rubberband_available (self->rubberband_state);
  float * out_samples[channels];
  rubberband_retrieve (
    self->rubberband_state, out_samples,
    (unsigned int) available_out_samples);

  /* store the output data in the given arrays */
  memcpy (
    &out_samples_l[0], out_samples[0],
    (size_t) available_out_samples * sizeof (float));
  if (channels == 2)
    {
      memcpy (
        &out_samples_r[0], out_samples[1],
        (size_t) available_out_samples *
          sizeof (float));
    }

  return available_out_samples;
}

/**
 * Perform stretching.
 *
 * @param in_samples_size The number of input samples
 *   per channel.
 *
 * @return The number of output samples generated per
 *   channel.
 */
ssize_t
stretcher_stretch_interleaved (
  Stretcher * self,
  float *     in_samples,
  size_t      in_samples_size,
  float **    _out_samples)
{
  g_return_val_if_fail (in_samples, -1);

  g_message ("input samples: %zu", in_samples_size);

  /* create the de-interleaved array */
  unsigned int channels = self->channels;
  float in_buffers_l[in_samples_size];
  float in_buffers_r[in_samples_size];
  for (size_t i = 0; i < in_samples_size; i++)
    {
      in_buffers_l[i] = in_samples[i * channels];
      if (channels == 2)
        in_buffers_r[i] =
          in_samples[i * channels + 1];
    }
  const float * in_buffers[2] = {
    in_buffers_l, in_buffers_r };

  /* tell rubberband how many input samples it will
   * receive */
  rubberband_set_expected_input_duration (
    self->rubberband_state, in_samples_size);

  /* study first */
  size_t samples_to_read = in_samples_size;
  while (samples_to_read > 0)
    {
      /* samples to read now */
      unsigned int read_now =
        (unsigned int)
        MIN (
          (size_t) self->block_size,
          samples_to_read);

      /* read */
      rubberband_study (
        self->rubberband_state, in_buffers,
        read_now, read_now == samples_to_read);

      /* remaining samples to read */
      samples_to_read -= read_now;
    }
  g_warn_if_fail (samples_to_read == 0);

  /* create the out sample arrays */
  float * out_samples[channels];
  size_t out_samples_size =
    (size_t)
    math_round_double_to_size_t (
      rubberband_get_time_ratio (
        self->rubberband_state) * in_samples_size);
  for (unsigned int i = 0; i < channels; i++)
    {
      out_samples[i] =
        malloc (sizeof (float) * out_samples_size);
    }

  /* process */
  size_t processed = 0;
  size_t total_out_frames = 0;
  while (processed < in_samples_size)
    {
      size_t in_chunk_size =
        rubberband_get_samples_required (
          self->rubberband_state);
      size_t samples_left =
        in_samples_size - processed;

      if (samples_left < in_chunk_size)
        {
          in_chunk_size = samples_left;
        }

      /* move the in buffers */
      const float * tmp_in_arrays[2] = {
        in_buffers[0] + processed,
        in_buffers[1] + processed };

      /* process */
      rubberband_process (
        self->rubberband_state, tmp_in_arrays,
        in_chunk_size,
        samples_left == in_chunk_size);

      processed += in_chunk_size;

      /*g_message ("processed %lu, in samples %lu",*/
        /*processed, in_samples_size);*/

      size_t avail =
        (size_t)
        rubberband_available (
          self->rubberband_state);

      /* retrieve the output data in temporary
       * arrays */
      float tmp_out_l[avail];
      float tmp_out_r[avail];
      float * tmp_out_arrays[2] = {
        tmp_out_l, tmp_out_r };
      size_t out_chunk_size =
        rubberband_retrieve (
          self->rubberband_state,
          tmp_out_arrays, avail);

      /* save the result */
      for (size_t i = 0; i < channels; i++)
        {
          for (size_t j = 0; j < out_chunk_size;
               j++)
            {
              out_samples[i][j + total_out_frames] =
                tmp_out_arrays[i][j];
            }
        }

      total_out_frames += out_chunk_size;
    }

  g_message (
    "retrieved %zu samples (expected %zu)",
    total_out_frames, out_samples_size);
  g_warn_if_fail (
    /* allow 1 sample earlier */
    total_out_frames <= out_samples_size &&
    total_out_frames >= out_samples_size - 1);

  /* store the output data in the given arrays */
  * _out_samples =
    realloc (
      * _out_samples,
      (size_t) channels * total_out_frames *
        sizeof (float));
  for (unsigned int ch = 0; ch < channels; ch++)
    {
      for (size_t i = 0; i < total_out_frames; i++)
        {
          (*_out_samples)[
            i * (size_t) channels + ch] =
              out_samples[ch][i];
        }
    }

  return (ssize_t) total_out_frames;
}

/**
 * Frees the resampler.
 */
void
stretcher_free (
  Stretcher * self)
{
  if (self->rubberband_state)
    rubberband_delete (self->rubberband_state);

  free (self);
}
