// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
 * Copyright (C) 2017-2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ---
 */

/**
 * \file
 *
 * Math utils.
 *
 * For more, look at libs/pbd/pbd/control_math.h in
 * ardour.
 */

#ifndef __UTILS_MATH_H__
#define __UTILS_MATH_H__

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "utils/types.h"

#include <float.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Frames to skip when calculating the RMS.
 *
 * The lower the more CPU intensive.
 */
#define MATH_RMS_FRAMES 1

/** Tiny number to be used for denormaml prevention
 * (-140dB). */
#define MATH_TINY_NUMBER (0.0000001)

#define MATH_MINUS_INFINITY (-HUGE_VAL)

/**
 * Checks if 2 doubles are equal.
 *
 * @param epsilon The allowed difference.
 */
#define math_floats_equal_epsilon(a, b, e) \
  ((a) > (b) ? (a) - (b) < e : (b) - (a) < e)

#define math_doubles_equal_epsilon math_floats_equal_epsilon

/**
 * Checks if 2 doubles are equal.
 */
#define math_floats_equal(a, b) \
  ((a) > (b) \
     ? (a) - (b) < FLT_EPSILON \
     : (b) - (a) < FLT_EPSILON)

#define math_doubles_equal(a, b) \
  ((a) > (b) \
     ? (a) - (b) < DBL_EPSILON \
     : (b) - (a) < DBL_EPSILON)

/**
 * Rounds a double to a (minimum) signed 32-bit
 * integer.
 */
#define math_round_double_to_signed_32(x) (lround (x))

/**
 * Rounds a double to a (minimum) signed 64-bit
 * integer.
 */
#define math_round_double_to_signed_64(x) (llround (x))

#define math_round_double_to_signed_frame_t(x) \
  math_round_double_to_signed_64 (x)

/**
 * Rounds a float to a (minimum) signed 32-bit
 * integer.
 */
#define math_round_float_to_signed_32(x) (lroundf (x))

/**
 * Rounds a float to a (minimum) signed 64-bit
 * integer.
 */
#define math_round_float_to_signed_64(x) (llroundf (x))

#define math_round_float_to_signed_frame_t(x) \
  math_round_float_to_signed_64 (x)

/**
 * Fast log calculation to be used where precision
 * is not required (like log curves).
 *
 * Taken from ardour from code in the public domain.
 */
CONST
static inline float
math_fast_log2 (float val)
{
  union
  {
    float f;
    int   i;
  } t;
  t.f = val;
  int * const exp_ptr = &t.i;
  int         x = *exp_ptr;
  const float log_2 = (float) (((x >> 23) & 255) - 128);

  x &= ~(255 << 23);
  x += 127 << 23;

  *exp_ptr = x;

  val = ((-1.0f / 3) * t.f + 2) * t.f - 2.0f / 3;

  return (val + log_2);
}

CONST
static inline float
math_fast_log (const float val)
{
  return (math_fast_log2 (val) * 0.69314718f);
}

CONST
static inline float
math_fast_log10 (const float val)
{
  return math_fast_log2 (val) / 3.312500f;
}

/**
 * Returns fader value 0.0 to 1.0 from amp value
 * 0.0 to 2.0 (+6 dbFS).
 */
CONST
static inline sample_t
math_get_fader_val_from_amp (sample_t amp)
{
  static const float fader_coefficient1 =
    /*192.f * logf (2.f);*/
    133.084258667509499408f;
  static const float fader_coefficient2 =
    /*powf (logf (2.f), 8.f) * powf (198.f, 8.f);*/
    1.25870863180257576e17f;

  /* to prevent weird values when amp is very
   * small */
  if (amp <= 0.00001f)
    {
      return 1e-20f;
    }
  else
    {
      if (math_floats_equal (amp, 1.f))
        {
          amp = 1.f + 1e-20f;
        }
      sample_t fader =
        powf (6.f * logf (amp) + fader_coefficient1, 8.f)
        / fader_coefficient2;
      return (sample_t) fader;
    }
}

/**
 * Returns amp value 0.0 to 2.0 (+6 dbFS) from
 * fader value 0.0 to 1.0.
 */
CONST
static inline sample_t
math_get_amp_val_from_fader (sample_t fader)
{
  static const float val1 = 1.f / 6.f;
  return powf (
    2.f, (val1) * (-192.f + 198.f * powf (fader, 1.f / 8.f)));
}

/**
 * Convert from amplitude 0.0 to 2.0 to dbFS.
 */
CONST
static inline sample_t
math_amp_to_dbfs (sample_t amp)
{
  return 20.f * log10f (amp);
}

sample_t
math_calculate_rms_amp (
  sample_t *      buf,
  const nframes_t nframes);

/**
 * Calculate db using RMS method.
 *
 * @param buf Buffer containing the samples.
 * @param nframes Number of samples.
 */
sample_t
math_calculate_rms_db (sample_t * buf, const nframes_t nframes);

/**
 * Convert form dbFS to amplitude 0.0 to 2.0.
 */
CONST
static inline sample_t
math_dbfs_to_amp (sample_t dbfs)
{
  return powf (10.f, (dbfs / 20.f));
}

/**
 * Convert form dbFS to fader val 0.0 to 1.0.
 */
CONST
static inline sample_t
math_dbfs_to_fader_val (sample_t dbfs)
{
  return math_get_fader_val_from_amp (math_dbfs_to_amp (dbfs));
}

/**
 * Asserts that the value is non-nan.
 *
 * Not real-time safe.
 *
 * @return Whether the value is valid (nonnan).
 */
bool
math_assert_nonnann (float x);

/**
 * @}
 */

#endif
