// SPDX-FileCopyrightText: © 2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
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

#include <cmath>
#include <cstdlib>

#include "dsp/peak_dsp.h"
#include "utils/objects.h"

/**
 * Process.
 *
 * @param p Frame array.
 * @param n Number of samples.
 */
void
PeakDsp::process (float * p, int n)
{
  float s = 0.f, t;

  if (this->fpp != n)
    {
      /*const float FALL = 15.f;*/
      constexpr float FALL = 5.f;
      const float     tme = (float) n / this->fsamp; // period time in seconds
      this->fall = powf (
        10.f,
        -0.05f * FALL * tme); // per period fallback multiplier
      this->fpp = n;
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

  if (!std::isfinite (t))
    t = 0;

  if (this->flag) // Display thread has read the rms value.
    {
      this->rms = max;
      this->flag = false;
    }
  else
    {
      // Adjust RMS value and update maximum since last read().
      if (max > this->rms)
        this->rms = max;
    }

  // Digital peak hold and fallback.
  if (t >= this->peak)
    {
      // If higher than current value, update and set hold counter.
      this->peak = t;
      this->cnt = this->hold;
    }
  else if (this->cnt > 0)
    {
      // else decrement counter if not zero,
      this->cnt -= this->fpp;
    }
  else
    {
      this->peak *= this->fall; // else let the peak value fall back,
      this->peak += 1e-10f;     // and avoid denormals.
    }
}

float
PeakDsp::read_f ()
{
  float rv = this->rms;
  this->flag = true; // Resets _rms in next process().
  return rv;
}

void
PeakDsp::read (float * irms, float * ipeak)
{
  *irms = this->rms;
  *ipeak = this->peak;
  this->flag = true; // Resets _rms in next process().
}

void
PeakDsp::reset ()
{
  this->rms = this->peak = .0f;
  this->cnt = 0;
  this->flag = false;
}

void
PeakDsp::init (float samplerate)
{
  /*const float hold = 0.5f;*/
  const float HOLD = 1.5f;
  this->fsamp = samplerate;

  this->hold =
    (int) (HOLD * samplerate + 0.5f); // number of samples to hold peak
}