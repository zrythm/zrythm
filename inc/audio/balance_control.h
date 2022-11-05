// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Balance control of stereo sources, mainly used
 * in the mixer channels.
 */

#ifndef __AUDIO_BALANCE_CONTROL_H__
#define __AUDIO_BALANCE_CONTROL_H__

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * See https://www.harmonycentral.com/articles/the-truth-about-panning-laws
 */
typedef enum BalanceControlAlgorithm
{
  /**
   * Classic "Balance" mode.
   *
   * Attennuates one channel only with no positive
   * gain.
   *
   * Examples:
   * hard left: mute right channel, left channel
   *   untouched.
   * mid left: attenuate right channel by (signal *
   *   pan - 0.5), left channel untouched.
   */
  BALANCE_CONTROL_ALGORITHM_LINEAR,
} BalanceControlAlgorithm;

/**
 * Returns the coefficients to multiply the L and
 * R signal with.
 */
void
balance_control_get_calc_lr (
  BalanceControlAlgorithm algo,
  float                   pan,
  float *                 calc_l,
  float *                 calc_r);

/**
 * @}
 */

#endif
