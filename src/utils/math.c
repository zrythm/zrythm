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

static float fader_coefficient1 = 0.f;
static float fader_coefficient2 = 0.f;

/**
 * Returns fader value 0.0 to 1.0 from amp value
 * 0.0 to 2.0 (+6 dbFS).
 */
sample_t
math_get_fader_val_from_amp (
  sample_t amp)
{
  /* to prevent weird values when amp is very
   * small */
  if (amp <= 0.00001f)
    return 1e-20f;
  else
    {
      sample_t fader =
        powf (
          6.f * logf (amp) + fader_coefficient1, 8.f) /
        fader_coefficient2;
      return (sample_t) fader;
    }
}

/**
 * Returns amp value 0.0 to 2.0 (+6 dbFS) from
 * fader value 0.0 to 1.0.
 */
sample_t
math_get_amp_val_from_fader (
  sample_t fader)
{
  static float val1 = 1.f / 6.f;
  return
    powf (
      2.f,
      (val1) *
      (-192.f + 198.f * powf (fader, 1.f / 8.f)));
}

/**
 * Gets the digital peak of the given signal as
 * amplitude (0-2).
 */
sample_t
math_calculate_max_amp (
  sample_t *      buf,
  const nframes_t nframes)
{
  sample_t ret = 1e-20f;
  for (nframes_t i = 0; i < nframes; i++)
    {
      if (fabsf (buf[i]) > ret)
        {
          ret = fabsf (buf[i]);
        }
    }

  return ret;
}

/**
 * Gets the RMS of the given signal as amplitude
 * (0-2).
 */
sample_t
math_calculate_rms_amp (
  sample_t *      buf,
  const nframes_t nframes)
{
  sample_t sum = 0, sample = 0;
  for (unsigned int i = 0;
       i < nframes; i += MATH_RMS_FRAMES)
  {
    sample = buf[i];
    sum += (sample * sample);
  }
  return
    sqrtf (
      sum /
      ((sample_t) nframes /
         (sample_t) MATH_RMS_FRAMES));
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
  return
    math_amp_to_dbfs (
      math_calculate_rms_amp (buf, nframes));
}

/**
 * Initializes coefficients to be used later.
 */
void
math_init ()
{
  fader_coefficient1 = 192.f * logf (2.f);
  fader_coefficient2 =
    powf (logf (2.f), 8.f) * powf (198.f, 8.f);
}
