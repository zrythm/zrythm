// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/balance_control.h"
#include "utils/logger.h"

#include "gtk_wrapper.h"

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
    case BalanceControlAlgorithm::BALANCE_CONTROL_ALGORITHM_LINEAR:
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
      z_error ("balance control algorithm not implemented yet");
      break;
    }
}
