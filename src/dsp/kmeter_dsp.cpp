/*
 * SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou
 * <alex@zrythm.org> SPDX-License-Identifier: LicenseRef-ZrythmLicense
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

#include <cmath>
#include <cstdlib>

#include "dsp/kmeter_dsp.h"

namespace zrythm::dsp
{

void
KMeterDsp::process (const float * p, int n)
{
  float s, t, z1, z2;

  if (fpp_ != n) [[unlikely]]
    {
      /*const float fall = 15.f;*/
      constexpr float fall = 5.f;
      const float     tme = (float) n / fsamp_; // period time in seconds
      fall_ = std::pow (
        10.0f,
        -0.05f * fall * tme); // per period fallback multiplier
      fpp_ = n;
    }

  t = 0;
  // Get filter state.
  z1 = z1_ > 50 ? 50 : (z1_ < 0 ? 0 : z1_);
  z2 = z2_ > 50 ? 50 : (z2_ < 0 ? 0 : z2_);

  // Perform filtering. The second filter is evaluated
  // only every 4th sample - this is just an optimisation.
  n /= 4; // Loop is unrolled by 4.
  while (n--) [[likely]]
    {
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                 // Update digital peak.
      z1 += omega_ * (s - z1); // Update first filter.
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                 // Update digital peak.
      z1 += omega_ * (s - z1); // Update first filter.
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                 // Update digital peak.
      z1 += omega_ * (s - z1); // Update first filter.
      s = *p++;
      s *= s;
      if (t < s)
        t = s;                      // Update digital peak.
      z1 += omega_ * (s - z1);      // Update first filter.
      z2 += 4 * omega_ * (z1 - z2); // Update second filter.
    }

  if (std::isnan (z1))
    z1 = 0;
  if (std::isnan (z2))
    z2 = 0;
  if (!std::isfinite (t))
    t = 0;

  // Save filter state. The added constants avoid denormals.
  z1_ = z1 + 1e-20f;
  z2_ = z2 + 1e-20f;

  s = sqrtf (2.0f * z2);
  t = sqrtf (t);

  if (flag_) // Display thread has read the rms value.
    {
      rms_ = s;
      flag_ = false;
    }
  else
    {
      // Adjust RMS value and update maximum since last read().
      if (s > rms_)
        rms_ = s;
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
KMeterDsp::read_f ()
{
  float rv = rms_;
  flag_ = true; // Resets _rms in next process().
  return rv;
}

std::pair<float, float>
KMeterDsp::read ()
{
  flag_ = true; // Resets _rms in next process().
  return std::make_pair (rms_, peak_);
}

void
KMeterDsp::reset ()
{
  z1_ = z2_ = rms_ = peak_ = .0f;
  cnt_ = 0;
  flag_ = false;
}

void
KMeterDsp::init (float samplerate)
{
  /*const float hold = 0.5f;*/
  const float hold = 1.5f;
  fsamp_ = samplerate;

  hold_ = (int) (hold * samplerate + 0.5f); // number of samples to hold peak
  omega_ = 9.72f / samplerate;              // ballistic filter coefficient
}

} // namespace zrythm::dsp
