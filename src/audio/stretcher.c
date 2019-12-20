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

#include "audio/stretcher.h"

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
  self->rubberband_state =
    rubberband_new (
      samplerate, channels,
      RubberBandOptionProcessOffline |
        RubberBandOptionStretchElastic |
        RubberBandOptionTransientsCrisp |
        RubberBandOptionDetectorCompound |
        RubberBandOptionPhaseLaminar |
        RubberBandOptionThreadingAlways |
        RubberBandOptionWindowStandard |
        RubberBandOptionSmoothingOff |
        RubberBandOptionFormantShifted |
        RubberBandOptionPitchHighQuality |
        RubberBandOptionChannelsApart,
      time_ratio, pitch_ratio);

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
  float *     _in_samples,
  size_t      in_samples_size,
  float *     _out_samples)
{
  g_return_val_if_fail (_in_samples, -1);

  /* create the de-interleaved array */
  unsigned int channels = self->channels;
  float in_samples_l[in_samples_size];
  float in_samples_r[in_samples_size];
  for (size_t i = 0; i < in_samples_size; i++)
    {
      in_samples_l[i] = _in_samples[i * 2];
      if (channels == 2)
        in_samples_r[i] = _in_samples[i * 2 + 1];
    }
  const float * in_samples[2] = {
    in_samples_l, in_samples_r };

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
  for (size_t i = 0;
       i < (size_t) available_out_samples; i++)
    {
      _out_samples[i * (size_t) channels] =
        out_samples[0][i];
      if (channels == 2)
        _out_samples[i * channels + 1] =
          out_samples[1][i];
    }
  (void) _out_samples;

  return available_out_samples;
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
