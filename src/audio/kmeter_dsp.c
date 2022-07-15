/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
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

#include "audio/kmeter_dsp.h"

#include <glib.h>

/**
 * Process.
 *
 * @param p Frame array.
 * @param n Number of samples.
 */
void
kmeter_dsp_process (KMeterDsp * self, float * p, int n)
{
  float s, t, z1, z2;

  if (G_UNLIKELY (self->fpp != n))
    {
      /*const float fall = 15.f;*/
      const float fall = 5.f;
      const float tme =
        (float) n / self->fsamp; // period time in seconds
      self->fall = powf (
        10.0f,
        -0.05f * fall * tme); // per period fallback multiplier
      self->fpp = n;
    }

  t = 0;
  // Get filter state.
  z1 = self->z1 > 50 ? 50 : (self->z1 < 0 ? 0 : self->z1);
  z2 = self->z2 > 50 ? 50 : (self->z2 < 0 ? 0 : self->z2);

  // Perform filtering. The second filter is evaluated
  // only every 4th sample - this is just an optimisation.
  n /= 4; // Loop is unrolled by 4.
  while (G_LIKELY (n--))
    {
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                      // Update digital peak.
      z1 += self->omega * (s - z1); // Update first filter.
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                      // Update digital peak.
      z1 += self->omega * (s - z1); // Update first filter.
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                      // Update digital peak.
      z1 += self->omega * (s - z1); // Update first filter.
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                      // Update digital peak.
      z1 += self->omega * (s - z1); // Update first filter.
      z2 +=
        4 * self->omega * (z1 - z2); // Update second filter.
    }

  if (isnan (z1))
    z1 = 0;
  if (isnan (z2))
    z2 = 0;
  if (!isfinite (t))
    t = 0;

  // Save filter state. The added constants avoid denormals.
  self->z1 = z1 + 1e-20f;
  self->z2 = z2 + 1e-20f;

  s = sqrtf (2.0f * z2);
  t = sqrtf (t);

  if (self->flag) // Display thread has read the rms value.
    {
      self->rms = s;
      self->flag = false;
    }
  else
    {
      // Adjust RMS value and update maximum since last read().
      if (s > self->rms)
        self->rms = s;
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
kmeter_dsp_read_f (KMeterDsp * self)
{
  float rv = self->rms;
  self->flag = true; // Resets _rms in next process().
  return rv;
}

void
kmeter_dsp_read (KMeterDsp * self, float * rms, float * peak)
{
  *rms = self->rms;
  *peak = self->peak;
  self->flag = true; // Resets _rms in next process().
}

void
kmeter_dsp_reset (KMeterDsp * self)
{
  self->z1 = self->z2 = self->rms = self->peak = .0f;
  self->cnt = 0;
  self->flag = false;
}

/**
 * Init with the samplerate.
 */
void
kmeter_dsp_init (KMeterDsp * self, float samplerate)
{
  /*const float hold = 0.5f;*/
  const float hold = 1.5f;
  self->fsamp = samplerate;

  self->hold =
    (int) (hold * samplerate + 0.5f); // number of samples to hold peak
  self->omega =
    9.72f / samplerate; // ballistic filter coefficient
}

KMeterDsp *
kmeter_dsp_new (void)
{
  KMeterDsp * self = calloc (1, sizeof (KMeterDsp));

  return self;
}

void
kmeter_dsp_free (KMeterDsp * self)
{
  free (self);
}
