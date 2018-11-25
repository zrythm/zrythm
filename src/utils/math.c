/*
 * utils/math.c - math utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <math.h>

#include "utils/math.h"

/**
 * Returns fader value 0.0 to 1.0 from amp value 0.0 to 2.0 (+6 dbFS).
 */
double math_get_fader_val_from_amp (double amp)
{
  double fader = pow (6.0 * log (amp) + 192 * log (2.0), 8.0) / (pow (log (2.0), 8.0) * pow (198.0, 8.0));
  return fader;
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
float math_calculate_rms_db (float *   buf, ///< buffer containing the samples
                             uint32_t  nframes) ///< number of samples
{
  float sum = 0, sample = 0;
  for (int i = 0; i < nframes; i += RMS_FRAMES)
  {
    sample = buf[i];
    sum += (sample * sample);
  }
  return math_amp_to_dbfs (sqrt (sum / (nframes / (float) RMS_FRAMES)));
}

/**
 * Convert from amplitude 0.0 to 2.0 to dbFS.
 */
float math_amp_to_dbfs (float amp)
{
  return 20.f * log10f (amp);
}

/**
 * Convert form dbFS to amplitude 0.0 to 2.0.
 */
float math_dbfs_to_amp (float dbfs)
{
  return 10.f * (dbfs / 20.f);
}
