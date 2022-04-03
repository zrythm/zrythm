// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
      g_critical (
        "balance control algorithm not implemented "
        "yet");
      break;
    }
}
