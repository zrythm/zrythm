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

/**
 * @file
 */

#ifndef __AUDIO_PEAK_FALL_SMOOTH_H__
#define __AUDIO_PEAK_FALL_SMOOTH_H__

class PeakFallSmooth
{
public:
  void  calculate_coeff (float frequency, float sample_rate);
  void  set_value (float val);
  float get_smoothed_value () const;

private:
  mutable float history_ = 0.f;
  float         value_ = 0.f;
  float         coeff_ = 0.f;
};

#endif
