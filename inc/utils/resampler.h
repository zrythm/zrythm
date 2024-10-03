// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio resampler.
 */

#ifndef __UTILS_RESAMPLER_H__
#define __UTILS_RESAMPLER_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "utils/types.h"

/**
 * @addtogroup utils
 *
 * @{
 */

typedef enum ResamplerQuality
{
  RESAMPLER_QUALITY_QUICK,
  RESAMPLER_QUALITY_LOW,
  RESAMPLER_QUALITY_MEDIUM,
  RESAMPLER_QUALITY_HIGH,
  RESAMPLER_QUALITY_VERY_HIGH,
} ResamplerQuality;

/**
 * Resampler.
 *
 * This currently only supports 32-bit interleaved floats for
 * input and output.
 */
typedef struct Resampler
{
  /** Private data. */
  void * priv;

  double       input_rate;
  double       output_rate;
  unsigned int num_channels;

  /** Given input (interleaved) frames .*/
  const float * in_frames;

  /** Number of frames per channel. */
  size_t num_in_frames;

  /** Number of frames read so far. */
  size_t frames_read;

  /** Output (interleaved) frames to be allocated during
   * resampler_new(). */
  float * out_frames;

  /** Number of frames per channel. */
  size_t num_out_frames;

  /** Number of frames written so far. */
  size_t frames_written;

  ResamplerQuality quality;
} Resampler;

/**
 * Creates a new instance of a Resampler with the given
 * settings.
 *
 * Resampler.num_out_frames will be set to the number of output
 * frames required for resampling with the given settings.
 */
Resampler *
resampler_new (
  const float *          in_frames,
  const size_t           num_in_frames,
  const double           input_rate,
  const double           output_rate,
  const unsigned int     num_channels,
  const ResamplerQuality quality,
  GError **              error);

/**
 * To be called periodically until resampler_is_done() returns
 * true.
 */
NONNULL_ARGS (1) bool
resampler_process (Resampler * self, GError ** error);

/**
 * Indicates whether resampling is finished.
 */
NONNULL bool
resampler_is_done (Resampler * self);

NONNULL void
resampler_free (Resampler * self);

/**
 * @}
 */

#endif
