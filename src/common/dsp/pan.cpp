// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 */

#include <cmath>

#include "common/dsp/pan.h"

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
    case PanAlgorithm::PAN_ALGORITHM_SINE_LAW:
      *calc_l = sinf ((1.f - pan) * (M_PIF / 2.f));
      *calc_r = sinf (pan * (M_PIF / 2.f));
      break;
    case PanAlgorithm::PAN_ALGORITHM_SQUARE_ROOT:
      *calc_l = sqrtf (1.f - pan);
      *calc_r = sqrtf (pan);
      break;
    case PanAlgorithm::PAN_ALGORITHM_LINEAR:
      *calc_l = 1.f - pan;
      *calc_r = pan;
      break;
    }
}
