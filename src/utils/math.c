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

#include <float.h>
#include <math.h>

#include "utils/math.h"

#include <gtk/gtk.h>

/**
 * Returns fader value 0.0 to 1.0 from amp value
 * 0.0 to 2.0 (+6 dbFS).
 */
sample_t
math_get_fader_val_from_amp (
  sample_t _amp)
{
  double amp = (double) _amp;

  /* to prevent weird values when amp is very
   * small */
  if (amp <= 0.000001)
    return 0.f;
  else
    {
      double fader =
        pow (6.0 * log (amp) +
             192.0 * log (2.0), 8.0) /
        (pow (log (2.0), 8.0) * pow (198.0, 8.0));
      return (sample_t) fader;
    }
}

/**
 * Returns amp value 0.0 to 2.0 (+6 dbFS) from
 * fader value 0.0 to 1.0.
 */
sample_t
math_get_amp_val_from_fader (
  sample_t _fader)
{
  double fader = (double) _fader;
  double amp =
    pow (
      2.0,
      (1.0 / 6) *
      (-192 + 198.0 * pow (fader, 1.0 / 8)));
  return (sample_t) amp;
}

/**
 * Calculate db using RMS method.
 *
 * @param buf Buffer containing the samples.
 * @param nframes Number of samples.
 */
sample_t
math_calculate_rms_db (
  sample_t *      buf,
  const nframes_t nframes)
{
  sample_t sum = 0, sample = 0;
  for (unsigned int i = 0;
       i < nframes; i += RMS_FRAMES)
  {
    sample = buf[i];
    sum += (sample * sample);
  }
  return
    math_amp_to_dbfs (
      sqrtf (
        sum /
        ((sample_t) nframes /
           (sample_t) RMS_FRAMES)));
}

/**
 * Convert from amplitude 0.0 to 2.0 to dbFS.
 */
sample_t
math_amp_to_dbfs (
  sample_t amp)
{
  return 20.f * log10f (amp);
}

/**
 * Convert form dbFS to amplitude 0.0 to 2.0.
 */
sample_t
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
int
math_floats_equal (
  const float a,
  const float b,
  const float epsilon)
{
  return fabsf (a - b) < epsilon;
}

/**
 * Checks if 2 floating points are equal.
 *
 * @param epsilon The allowed differene.
 */
int
math_doubles_equal (
  const double a,
  const double b,
  const double epsilon)
{
  return fabs (a - b) < epsilon;
}
