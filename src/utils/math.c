/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
#include "utils/string.h"

#include <gtk/gtk.h>

/**
 * Returns fader value 0.0 to 1.0 from amp value
 * 0.0 to 2.0 (+6 dbFS).
 */
sample_t
math_get_fader_val_from_amp (
  sample_t amp)
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
 * Asserts that the value is non-nan.
 *
 * Not real-time safe.
 *
 * @return Whether the value is valid (nonnan).
 */
bool
math_assert_nonnann (
  float x)
{
  char * val =
    g_strdup_printf ("%f", (double) x);
  if (isnan (x) ||
      string_contains_substr (val, "nan"))
    {
      g_critical ("nan");
      g_free (val);
      return false;
    }
  if (!isfinite (x) ||
      string_contains_substr (val, "inf"))
    {
      g_critical ("inf");
      g_free (val);
      return false;
    }
  return true;
}
