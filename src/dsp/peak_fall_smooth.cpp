// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
 * SPDX-FileCopyrightText: © 2023 Patrick Desaulniers
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ---
 */

#include <cmath>
#include <numbers>

#include "dsp/peak_fall_smooth.h"

namespace zrythm::dsp
{

void
PeakFallSmooth::calculate_coeff (float frequency, float sample_rate)
{
  coeff_ = std::exp (-2.f * std::numbers::pi_v<float> * frequency / sample_rate);
}

void
PeakFallSmooth::set_value (float val)
{
  if (history_ < val)
    {
      history_ = val;
    }

  value_ = val;
}

float
PeakFallSmooth::get_smoothed_value () const
{
  float result = value_ + coeff_ * (history_ - value_);
  history_ = result;

  return result;
}

} // namespace zrythm::dsp
