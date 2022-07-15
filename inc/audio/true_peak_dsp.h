/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __AUDIO_TRUE_PEAK_DSP__
#define __AUDIO_TRUE_PEAK_DSP__

#include <stdbool.h>

#include "ext/zita-resampler/resampler.h"

typedef struct TruePeakDsp
{
  float           m;
  float           p;
  float           z1;
  float           z2;
  bool            res;
  float *         buf;
  ZitaResampler * src;

  float w1; // attack filter coefficient
  float w2; // attack filter coefficient
  float w3; // release filter coefficient
  float g;  // gain factor
} TruePeakDsp;

/**
 * Process.
 *
 * @param p Frame array.
 * @param n Number of samples.
 */
void
true_peak_dsp_process (TruePeakDsp * self, float * p, int n);

void
true_peak_dsp_process_max (TruePeakDsp * self, float * p, int n);

float
true_peak_dsp_read_f (TruePeakDsp * self);

void
true_peak_dsp_read (TruePeakDsp * self, float * m, float * p);

void
true_peak_dsp_reset (TruePeakDsp * self);

/**
 * Init with the samplerate.
 */
void
true_peak_dsp_init (TruePeakDsp * self, float samplerate);

TruePeakDsp *
true_peak_dsp_new (void);

void
true_peak_dsp_free (TruePeakDsp * self);

#endif
