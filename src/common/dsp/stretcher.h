// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Time and pitch stretching API.
 */

#ifndef __AUDIO_STRETCHER_H__
#define __AUDIO_STRETCHER_H__

#include <cstddef>

#include <sys/types.h>

#include "common/utils/types.h"

#include <rubberband/rubberband-c.h>

/**
 * @addtogroup dsp
 *
 * @{
 */

enum class StretcherBackend
{
  /** Lib rubberband. */
  STRETCHER_BACKEND_RUBBERBAND,

  /** Paulstretch. */
  STRETCHER_BACKEND_PAULSTRETCH,

  /** SBSMS - Subband Sinusoidal Modeling
   * Synthesis. */
  STRETCHER_BACKEND_SBSMS,
};

/**
 * Stretcher interface.
 */
struct Stretcher
{
  StretcherBackend backend;

  /** For rubberband API. */
  RubberBandState rubberband_state;

  unsigned int samplerate;
  unsigned int channels;

  bool is_realtime;

  /**
   * Size of the block to process in each iteration.
   *
   * Somewhere around 6k should be fine.
   */
  unsigned int block_size;
};

/**
 * Create a new Stretcher using the rubberband backend.
 *
 * @param samplerate The new samplerate.
 * @param time_ratio The ratio to multiply time by (eg if the
 *   BPM is doubled, this will be 0.5).
 * @param pitch_ratio The ratio to pitch by. This will normally
 *   be 1.0 when time-stretching).
 * @param realtime Whether to perform realtime stretching
 *   (lower quality but fast enough to be used real-time).
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
 *   this is nullptr, the audio is assumed to be mono.
 * @param in_samples_size The number of input samples
 *   per channel.
 *
 * @return The number of output samples generated per
 *   channel.
 */
signed_frame_t
stretcher_stretch (
  Stretcher *   self,
  const float * in_samples_l,
  const float * in_samples_r,
  size_t        in_samples_size,
  float *       out_samples_l,
  float *       out_samples_r,
  size_t        out_samples_wanted);

/**
 * Get latency in number of samples.
 */
unsigned int
stretcher_get_latency (Stretcher * self);

void
stretcher_set_time_ratio (Stretcher * self, double ratio);

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
signed_frame_t
stretcher_stretch_interleaved (
  Stretcher *   self,
  const float * in_samples,
  size_t        in_samples_size,
  float **      _out_samples);

/**
 * Frees the resampler.
 */
void
stretcher_free (Stretcher * self);

/**
 * @}
 */

#endif
