/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UTILS_MATH_H__
#define __UTILS_MATH_H__

#include <float.h>
#include <math.h>
#include <stdint.h>

#include "utils/types.h"

/**
 * Frames to skip when calculating the RMS.
 *
 * The higher the more CPU intensive.
 */
#define MATH_RMS_FRAMES 1
#define MATH_FLOATS_EQUAL(a,b,epsilon) \
  (fabsf (a - b) < epsilon)

/**
 * Returns fader value 0.0 to 1.0 from amp value 0.0 to 2.0 (+6 dbFS).
 */
sample_t
math_get_fader_val_from_amp (
  sample_t amp);

/**
 * Returns amp value 0.0 to 2.0 (+6 dbFS) from fader value 0.0 to 1.0.
 */
sample_t
math_get_amp_val_from_fader (
  sample_t fader);

/**
 * Calculate db using RMS method.
 *
 * @param buf Buffer containing the samples.
 * @param nframes Number of samples.
 */
sample_t
math_calculate_rms_db (
  sample_t *      buf,
  const nframes_t nframes);

/**
 * Convert from amplitude 0.0 to 2.0 to dbFS.
 */
static inline sample_t
math_amp_to_dbfs (
  sample_t amp)
{
  return 20.f * log10f (amp);
}

/**
 * Convert form dbFS to amplitude 0.0 to 2.0.
 */
static inline sample_t
math_dbfs_to_amp (
  sample_t dbfs)
{
  return powf (10.f, (dbfs / 20.f));
}

/**
 * Checks if 2 floating points are equal.
 *
 * @param epsilon The allowed differene.
 */
static inline int
math_floats_equal (
  const float a,
  const float b,
  const float epsilon)
{
  return fabsf (a - b) < epsilon;
}

/**
 * Checks if 2 doubles are equal.
 *
 * @param epsilon The allowed differene.
 */
static inline int
math_doubles_equal (
  const double a,
  const double b,
  const double epsilon)
{
  return fabs (a - b) < epsilon;
}

/**
 * Initializes coefficients to be used later.
 */
void
math_init ();

#endif
