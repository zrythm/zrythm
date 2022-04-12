/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2008-2011 Fons Adriaensen <fons@linuxaudio.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "audio/peak_dsp.h"

/**
 * Process.
 *
 * @param p Frame array.
 * @param n Number of samples.
 */
void
peak_dsp_process (PeakDsp * self, float * p, int n)
{
  float s = 0.f, t;

  if (self->fpp != n)
    {
      /*const float fall = 15.f;*/
      const float fall = 5.f;
      const float tme =
        (float) n
        / self->fsamp; // period time in seconds
      self->fall = powf (
        10.f,
        -0.05f * fall
          * tme); // per period fallback multiplier
      self->fpp = n;
    }

  t = 0;

  // Perform processing
  float max = 0.f;
  while (n--)
    {
      s = *p++;
      if (fabsf (s) > max)
        {
          max = fabsf (s);
        }
      if (t < max)
        t = max; // Update digital peak.
    }

  if (!isfinite (t))
    t = 0;

  if (self->flag) // Display thread has read the rms value.
    {
      self->rms = max;
      self->flag = false;
    }
  else
    {
      // Adjust RMS value and update maximum since last read().
      if (max > self->rms)
        self->rms = max;
    }

  // Digital peak hold and fallback.
  if (t >= self->peak)
    {
      // If higher than current value, update and set hold counter.
      self->peak = t;
      self->cnt = self->hold;
    }
  else if (self->cnt > 0)
    {
      // else decrement counter if not zero,
      self->cnt -= self->fpp;
    }
  else
    {
      self->peak *=
        self->fall; // else let the peak value fall back,
      self->peak += 1e-10f; // and avoid denormals.
    }
}

float
peak_dsp_read_f (PeakDsp * self)
{
  float rv = self->rms;
  self->flag =
    true; // Resets _rms in next process().
  return rv;
}

void
peak_dsp_read (
  PeakDsp * self,
  float *   rms,
  float *   peak)
{
  *rms = self->rms;
  *peak = self->peak;
  self->flag =
    true; // Resets _rms in next process().
}

void
peak_dsp_reset (PeakDsp * self)
{
  self->rms = self->peak = .0f;
  self->cnt = 0;
  self->flag = false;
}

/**
 * Init with the samplerate.
 */
void
peak_dsp_init (PeakDsp * self, float samplerate)
{
  /*const float hold = 0.5f;*/
  const float hold = 1.5f;
  self->fsamp = samplerate;

  self->hold =
    (int) (hold * samplerate + 0.5f); // number of samples to hold peak
}

PeakDsp *
peak_dsp_new (void)
{
  PeakDsp * self = calloc (1, sizeof (PeakDsp));

  return self;
}

void
peak_dsp_free (PeakDsp * self)
{
  free (self);
}
