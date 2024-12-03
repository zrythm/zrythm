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

namespace zrythm::dsp
{

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

  if (fpp_ != n)
    {
      /*const float FALL = 15.f;*/
      constexpr float FALL = 5.f;
      const float     tme = (float) n / fsamp_; // period time in seconds
      fall_ = powf (
        10.f,
        -0.05f * FALL * tme); // per period fallback multiplier
      fpp_ = n;
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

  if (flag_) // Display thread has read the rms value.
    {
      rms_ = max;
      flag_ = false;
    }
  else
    {
      // Adjust RMS value and update maximum since last read().
      if (max > rms_)
        rms_ = max;
    }

  // Digital peak hold and fallback.
  if (t >= peak_)
    {
      // If higher than current value, update and set hold counter.
      peak_ = t;
      cnt_ = hold_;
    }
  else if (cnt_ > 0)
    {
      // else decrement counter if not zero,
      cnt_ -= fpp_;
    }
  else
    {
      peak_ *= fall_;  // else let the peak value fall back,
      peak_ += 1e-10f; // and avoid denormals.
    }
}

float
PeakDsp::read_f ()
{
  float rv = rms_;
  flag_ = true; // Resets _rms in next process().
  return rv;
}

std::pair<float, float>
PeakDsp::read ()
{
  flag_ = true; // Resets _rms in next process().
  return { rms_, peak_ };
}

void
PeakDsp::reset ()
{
  rms_ = peak_ = .0f;
  cnt_ = 0;
  flag_ = false;
}

void
PeakDsp::init (float samplerate)
{
  /*const float hold = 0.5f;*/
  constexpr float HOLD = 1.5f;
  fsamp_ = samplerate;

  hold_ = (int) (HOLD * samplerate + 0.5f); // number of samples to hold peak
}

}; // namespace zrythm::dsp
