// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Optimized DSP functions.
 *
 * @note More at
 * https://github.com/DISTRHO/DPF-Max-Gen/blob/master/plugins/common/gen_dsp/genlib_ops.h#L313
 */

#ifndef __UTILS_DSP_H__
#define __UTILS_DSP_H__

#include "zrythm-config.h"

#include <cstddef>

#include "common/utils/math.h"
#include "gui/cpp/backend/zrythm.h"

#include <glib.h>

#if HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif

/**
 * Fill the buffer with the given value.
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_fill (float * buf, float val, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::fill (buf, val, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          buf[i] = val;
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Clamp the buffer to min/max.
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_limit1 (float * buf, float minf, float maxf, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::limit1 (buf, minf, maxf, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          buf[i] = CLAMP (buf[i], minf, maxf);
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Compute dest[i] = src[i].
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_copy (float * dest, const float * src, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::copy (dest, src, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] = src[i];
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Scale: dst[i] = dst[i] * k.
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_mul_k2 (float * dest, float k, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::mul_k2 (dest, k, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] *= k;
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Gets the maximum absolute value of the buffer (as amplitude).
 */
[[nodiscard]] ATTR_NONNULL static inline float
dsp_abs_max (const float * buf, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      return lsp::dsp::abs_max (buf, size);
    }
  else
    {
#endif
      float ret = 1e-20f;
      for (size_t i = 0; i < size; i++)
        {
          if (fabsf (buf[i]) > ret)
            {
              ret = fabsf (buf[i]);
            }
        }
      return ret;
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Gets the absolute max of the buffer.
 *
 * @return Whether the peak changed.
 */
ATTR_HOT ATTR_NONNULL static inline bool
dsp_abs_max_with_existing_peak (float * buf, float * cur_peak, size_t size)
{
  float new_peak = *cur_peak;

#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      new_peak = lsp::dsp::abs_max (buf, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          float val = fabsf (buf[i]);
          if (val > new_peak)
            {
              new_peak = val;
            }
        }
#if HAVE_LSP_DSP
    }
#endif

  bool changed = !math_floats_equal (new_peak, *cur_peak);
  *cur_peak = new_peak;

  return changed;
}

/**
 * Gets the minimum of the buffer.
 */
ATTR_NONNULL float
dsp_min (const float * buf, size_t size);

/**
 * Gets the maximum of the buffer.
 */
ATTR_NONNULL float
dsp_max (const float * buf, size_t size);

/**
 * Calculate dst[i] = dst[i] + src[i].
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_add2 (float * dest, const float * src, size_t count)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::add2 (dest, src, count);
    }
  else
    {
#endif
      for (size_t i = 0; i < count; i++)
        {
          dest[i] = dest[i] + src[i];
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Calculate dest[i] = dest[i] * k1 + src[i] * k2.
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_mix2 (float * dest, const float * src, float k1, float k2, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::mix2 (dest, src, k1, k2, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] = dest[i] * k1 + src[i] * k2;
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Reverse the order of samples: dst[i] <=> dst[count - i - 1].
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_reverse1 (float * dest, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::reverse1 (dest, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] = dest[(size - i) - 1];
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Reverse the order of samples: dst[i] <=> src[count - i - 1].
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_reverse2 (float * dest, const float * src, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::reverse2 (dest, src, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] = src[(size - i) - 1];
        }
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Calculate normalized values:
 * dst[i] = src[i] / (max { abs(src) }).
 */
ATTR_NONNULL ATTR_HOT static inline void
dsp_normalize (float * dest, const float * src, size_t size)
{
#if HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp::dsp::normalize (dest, src, size);
    }
  else
    {
#endif
      dsp_copy (dest, src, size);
      float abs_peak = dsp_abs_max (dest, size);
      dsp_mul_k2 (dest, 1.f / abs_peak, size);
#if HAVE_LSP_DSP
    }
#endif
}

/**
 * Calculate
 * dst[i] = dst[i] + src1[i] * k1 + src2[i] * k2.
 */
ATTR_NONNULL void
dsp_mix_add2 (
  float *       dest,
  const float * src1,
  const float * src2,
  float         k1,
  float         k2,
  size_t        size);

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
ATTR_NONNULL void
dsp_linear_fade_in_from (
  float * dest,
  int32_t start_offset,
  int32_t total_frames_to_fade,
  size_t  size,
  float   fade_from_multiplier);

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
ATTR_NONNULL void
dsp_linear_fade_out_to (
  float * dest,
  int32_t start_offset,
  int32_t total_frames_to_fade,
  size_t  size,
  float   fade_to_multiplier);

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
ATTR_NONNULL void
dsp_make_mono (float * l, float * r, size_t size, bool equal_power);

#endif
