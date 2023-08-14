// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 */

#include <math.h>

#include "dsp/pan.h"

#define M_PIF 3.14159265358979323846f

void
pan_get_calc_lr (
  PanLaw       law,
  PanAlgorithm algo,
  float        pan,
  float *      calc_l,
  float *      calc_r)
{
  switch (algo)
    {
    case PAN_ALGORITHM_SINE_LAW:
      *calc_l = sinf ((1.f - pan) * (M_PIF / 2.f));
      *calc_r = sinf (pan * (M_PIF / 2.f));
      break;
    case PAN_ALGORITHM_SQUARE_ROOT:
      *calc_l = sqrtf (1.f - pan);
      *calc_r = sqrtf (pan);
      break;
    case PAN_ALGORITHM_LINEAR:
      *calc_l = 1.f - pan;
      *calc_r = pan;
      break;
    }
}
