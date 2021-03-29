/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/balance_control.h"

#include <gtk/gtk.h>

/**
 * Returns the coefficients to multiply the L and
 * R signal with.
 */
void
balance_control_get_calc_lr (
  BalanceControlAlgorithm algo,
  float                   pan,
  float *                 calc_l,
  float *                 calc_r)
{
  switch (algo)
    {
    case BALANCE_CONTROL_ALGORITHM_LINEAR:
      if (pan < 0.5f)
        {
          *calc_l = 1.f;
          *calc_r = pan / 0.5f;
        }
      else
        {
          *calc_l = (1.0f - pan) / 0.5f;
          *calc_r = 1.f;
        }
      break;
    default:
      g_warning (
        "balance control algorithm not implemented "
        "yet");
      break;
    }
}
