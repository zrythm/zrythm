/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <float.h>
#include <math.h>

#include "utils/math.h"

#include <gtk/gtk.h>

/**
 * Returns fader value 0.0 to 1.0 from amp value 0.0 to 2.0 (+6 dbFS).
 */
double math_get_fader_val_from_amp (double amp)
{
  /* to prevent weird values when amp is very small */
  if (amp <= 0.000001)
    return 0.0;
  else
    {
      double fader =
        pow (6.0 * log (amp) +
             192.0 * log (2.0), 8.0) /
        (pow (log (2.0), 8.0) * pow (198.0, 8.0));
      return fader;
    }
}

/**
 * Returns amp value 0.0 to 2.0 (+6 dbFS) from fader value 0.0 to 1.0.
 */
double math_get_amp_val_from_fader (double fader)
{
  double amp = pow (2.0, (1.0 / 6) * (-192 + 198.0 * pow (fader, 1.0 / 8)));
  return amp;
}

/**
 * Calculate db using RMS method.
 */
double math_calculate_rms_db (
  float *   buf, ///< buffer containing the samples
  int       nframes) ///< number of samples
{
  double sum = 0, sample = 0;
  for (int i = 0; i < nframes; i += RMS_FRAMES)
  {
    sample = buf[i];
    sum += (sample * sample);
  }
  return math_amp_to_dbfs (sqrt (sum / (nframes / (double) RMS_FRAMES)));
}

/**
 * Convert from amplitude 0.0 to 2.0 to dbFS.
 */
double math_amp_to_dbfs (double amp)
{
  return 20.f * log10 (amp);
}

/**
 * Convert form dbFS to amplitude 0.0 to 2.0.
 */
double math_dbfs_to_amp (double dbfs)
{
  return pow (10.0, (dbfs / 20.0));
}
