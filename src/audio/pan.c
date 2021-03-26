/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "audio/pan.h"

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
