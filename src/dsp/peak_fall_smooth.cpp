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

#include <cmath>

#include "dsp/peak_fall_smooth.h"
#include "utils/objects.h"

PeakFallSmooth *
peak_fall_smooth_new (void)
{
  PeakFallSmooth * self = object_new (PeakFallSmooth);
  return self;
}

void
peak_fall_smooth_calculate_coeff (
  PeakFallSmooth * self,
  const float      frequency,
  const float      sample_rate)
{
  self->coeff = expf (-2.f * (float) M_PI * frequency / sample_rate);
}

void
peak_fall_smooth_set_value (PeakFallSmooth * self, const float val)
{
  if (self->history < val)
    {
      self->history = val;
    }

  self->value = val;
}

float
peak_fall_smooth_get_smoothed_value (PeakFallSmooth * self)
{
  float result = self->value + self->coeff * (self->history - self->value);
  self->history = result;

  return result;
}

void
peak_fall_smooth_free (PeakFallSmooth * self)
{
  object_zero_and_free (self);
}
