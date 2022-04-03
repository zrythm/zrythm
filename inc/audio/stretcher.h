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

/**
 * \file
 *
 * Time and pitch stretching API.
 */

#ifndef __AUDIO_STRETCHER_H__
#define __AUDIO_STRETCHER_H__

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "utils/types.h"

#include <rubberband/rubberband-c.h>

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum StretcherBackend
{
  /** Lib rubberband. */
  STRETCHER_BACKEND_RUBBERBAND,

  /** Paulstretch. */
  STRETCHER_BACKEND_PAULSTRETCH,

  /** SBSMS - Subband Sinusoidal Modeling
   * Synthesis. */
  STRETCHER_BACKEND_SBSMS,
} StretcherBackend;

/**
 * Stretcher interface.
 */
typedef struct Stretcher
{
  StretcherBackend backend;

  /** For rubberband API. */
  RubberBandState rubberband_state;

  unsigned int samplerate;
  unsigned int channels;

  bool is_realtime;

  /**
   * Size of the block to process in each
   * iteration.
   *
   * Somewhere around 6k should be fine.
   */
  unsigned int block_size;
} Stretcher;

/**
 * Create a new Stretcher using the rubberband
 * backend.
 *
 * @param samplerate The new samplerate.
 * @param time_ratio The ratio to multiply time by
 *   (eg if the BPM is doubled, this will be 0.5).
 * @param pitch_ratio The ratio to pitch by. This
 *   will normally be 1.0 when time-stretching).
 * @param realtime Whether to perform realtime
 *   stretching (lower quality but fast enough to
 *   be used real-time).
 */
Stretcher *
stretcher_new_rubberband (
  unsigned int samplerate,
  unsigned int channels,
  double       time_ratio,
  double       pitch_ratio,
  bool         realtime);

/**
 * Perform stretching.
 *
 * @param in_samples_l The left samples.
 * @param in_samples_r The right channel samples. If
 *   this is NULL, the audio is assumed to be mono.
 * @param in_samples_size The number of input samples
 *   per channel.
 *
 * @return The number of output samples generated per
 *   channel.
 */
ssize_t
stretcher_stretch (
  Stretcher * self,
  float *     in_samples_l,
  float *     in_samples_r,
  size_t      in_samples_size,
  float *     out_samples_l,
  float *     out_samples_r,
  size_t      out_samples_wanted);

/**
 * Get latency in number of samples.
 */
unsigned int
stretcher_get_latency (Stretcher * self);

void
stretcher_set_time_ratio (
  Stretcher * self,
  double      ratio);

/**
 * Perform stretching.
 *
 * @note Not real-time safe, does allocations.
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
  float **    _out_samples);

/**
 * Frees the resampler.
 */
void
stretcher_free (Stretcher * self);

/**
 * @}
 */

#endif
