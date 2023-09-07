// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <math.h>

#include "utils/dsp.h"
#include "utils/math.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#ifdef HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif

/**
 * Gets the minimum of the buffer.
 */
float
dsp_min (float * buf, size_t size)
{
  float min = 1000.f;
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      min = lsp_dsp_min (buf, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          if (buf[i] < min)
            {
              min = buf[i];
            }
        }
#ifdef HAVE_LSP_DSP
    }
#endif

  return min;
}

/**
 * Gets the maximum of the buffer.
 */
float
dsp_max (float * buf, size_t size)
{
  float max = -1000.f;
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      max = lsp_dsp_max (buf, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          if (buf[i] > max)
            {
              max = buf[i];
            }
        }
#ifdef HAVE_LSP_DSP
    }
#endif

  return max;
}

/**
 * Calculate
 * dst[i] = dst[i] + src1[i] * k1 + src2[i] * k2.
 */
void
dsp_mix_add2 (
  float *       dest,
  const float * src1,
  const float * src2,
  float         k1,
  float         k2,
  size_t        size)
{
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_mix_add2 (dest, src1, src2, k1, k2, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] = dest[i] + src1[i] * k1 + src2[i] * k2;
        }
#ifdef HAVE_LSP_DSP
    }
#endif
}

/**
 * Calculate linear fade by multiplying from 0 to 1 for
 * @param total_frames_to_fade samples.
 *
 * @note Does not work properly when @param
 *   fade_from_multiplier is < 1k.
 *
 * @param start_offset Start offset in the fade interval.
 * @param total_frames_to_fade Total frames that should be
 *   faded.
 * @param size Number of frames to process.
 * @param fade_from_multiplier Multiplier to fade from (0 to
 *   fade from silence.)
 */
void
dsp_linear_fade_in_from (
  float * dest,
  int32_t start_offset,
  int32_t total_frames_to_fade,
  size_t  size,
  float   fade_from_multiplier)
{
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_lin_inter_mul2 (
        dest, 0, fade_from_multiplier, total_frames_to_fade,
        1.f, start_offset, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          float k =
            (float) (i + (size_t) start_offset)
            / (float) total_frames_to_fade;
          k = fade_from_multiplier
              + (1.f - fade_from_multiplier) * k;
          dest[i] *= k;
        }
#ifdef HAVE_LSP_DSP
    }
#endif
}

/**
 * Calculate linear fade by multiplying from 0 to 1 for
 * @param total_frames_to_fade samples.
 *
 * @param start_offset Start offset in the fade interval.
 * @param total_frames_to_fade Total frames that should be
 *   faded.
 * @param size Number of frames to process.
 * @param fade_to_multiplier Multiplier to fade to (0 to fade
 *   to silence.)
 */
NONNULL void
dsp_linear_fade_out_to (
  float * dest,
  int32_t start_offset,
  int32_t total_frames_to_fade,
  size_t  size,
  float   fade_to_multiplier)
{
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_lin_inter_mul2 (
        dest, 0, 1.f, total_frames_to_fade,
        fade_to_multiplier, start_offset, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          float k =
            (float) ((size_t) total_frames_to_fade
                     - (i + (size_t) start_offset))
            / (float) total_frames_to_fade;
          k = fade_to_multiplier
              + (1.f - fade_to_multiplier) * k;
          dest[i] *= k;
        }
#ifdef HAVE_LSP_DSP
    }
#endif
}

/**
 * Makes the two signals mono.
 *
 * @param equal_power True for equal power, false for equal
 *   amplitude.
 *
 * @note Equal amplitude is more suitable for mono
 * compatibility checking. For reference:
 * - equal power sum = (L+R) * 0.7079 (-3dB)
 * - equal amplitude sum = (L+R) /2 (-6.02dB)
 */
void
dsp_make_mono (float * l, float * r, size_t size, bool equal_power)
{
  float multiple = equal_power ? 0.7079f : 0.5f;
  dsp_mix2 (l, r, multiple, multiple, size);
  dsp_copy (r, l, size);
}
