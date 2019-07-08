/*
 * utils/math - math utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <stdint.h>

#define RMS_FRAMES 32

/**
 * Returns fader value 0.0 to 1.0 from amp value 0.0 to 2.0 (+6 dbFS).
 */
double math_get_fader_val_from_amp (double amp);

/**
 * Returns amp value 0.0 to 2.0 (+6 dbFS) from fader value 0.0 to 1.0.
 */
double math_get_amp_val_from_fader (double fader);

/**
 * Calculate db using RMS method.
 */
double math_calculate_rms_db (
  float *   buf, ///< buffer containing the samples
  int  nframes); ///< number of samples

/**
 * Convert from amplitude 0.0 to 2.0 to dbFS.
 */
double math_amp_to_dbfs (double amp);

/**
 * Convert form dbFS to amplitude 0.0 to 2.0.
 */
double math_dbfs_to_amp (double dbfs);

#endif
