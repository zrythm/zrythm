// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
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
 * \file
 */

#ifndef __AUDIO_PEAK_FALL_SMOOTH_H__
#define __AUDIO_PEAK_FALL_SMOOTH_H__

typedef struct PeakFallSmooth
{
  float history;
  float value;
  float coeff;
} PeakFallSmooth;

PeakFallSmooth *
peak_fall_smooth_new (void);

void
peak_fall_smooth_calculate_coeff (
  PeakFallSmooth * self,
  const float      frequency,
  const float      sample_rate);

void
peak_fall_smooth_set_value (
  PeakFallSmooth * self,
  const float      val);

float
peak_fall_smooth_get_smoothed_value (PeakFallSmooth * self);

void
peak_fall_smooth_free (PeakFallSmooth * self);

#endif
