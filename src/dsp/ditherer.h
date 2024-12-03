// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format off
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 *     ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
 *
 * Tracktion Engine is published under a dual [GPL3 (or later)](https://www.gnu.org/licenses/gpl-3.0.en.html)/[Commercial license](https://www.tracktion.com/develop/tracktion-engine).
 *
 * ---
 */
// clang-format on

#ifndef ZRYTHM_DSP_DITHER_H
#define ZRYTHM_DSP_DITHER_H

namespace zrythm::dsp
{

/**
    An extremely naive ditherer.
    TODO: this could be done much better!
*/
class Ditherer
{
public:
  void reset (int numBits) noexcept;

  void process (float * samps, int num) noexcept;

private:
  int   random1_ = 0, random2_ = 0;
  float amp_ = 0, offset_ = 0;
  float s1_ = 0, s2_ = 0;
};

} // namespace zrythm::dsp

#endif
