// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format off
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
 *
 * Tracktion Engine is published under a dual [GPL3 (or later)](https://www.gnu.org/licenses/gpl-3.0.en.html)/[Commercial license](https://www.tracktion.com/develop/tracktion-engine).
 */
// clang-format on

#include <cmath>
#include <cstdlib>

#include "dsp/ditherer.h"

namespace zrythm::dsp
{

void
Ditherer::reset (int numBits) noexcept
{
  auto wordLen = std::pow (2.0f, (float) (numBits - 1));
  auto invWordLen = 1.0f / wordLen;
  amp_ = invWordLen / static_cast<float> (RAND_MAX);
  offset_ = invWordLen * 0.5f;
}

void
Ditherer::process (float * samps, int num) noexcept
{
  while (--num >= 0)
    {
      random2_ = random1_;
      random1_ = rand ();

      auto in = *samps;

      // check for dodgy numbers coming in..
      if (in < -0.000001f || in > 0.000001f)
        {
          in += 0.5f * (s1_ + s1_ - s2_);
          auto out = in + offset_ + amp_ * (float) (random1_ - random2_);
          *samps++ = out;
          s2_ = s1_;
          s1_ = in - out;
        }
      else
        {
          *samps++ = in;
          in += 0.5f * (s1_ + s1_ - s2_);
          auto out = in + offset_ + amp_ * (float) (random1_ - random2_);
          s2_ = s1_;
          s1_ = in - out;
        }
    }
}

} // namespace zrythm::dsp
