// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_DSP_PAN_H
#define ZRYTHM_DSP_PAN_H

#include "utils/format.h"
#include "utils/logger.h"

#include <QtTranslation>

namespace zrythm::dsp
{

/**
 * These are only useful when changing mono to stereo.
 *
 * No compensation is needed for stereo to stereo.
 * See
 * https://www.hackaudio.com/digital-signal-processing/stereo-audio/square-law-panning/
 */
enum class PanLaw
{
  ZerodB,
  Minus3dB,
  Minus6dB
};

/**
 * See https://www.harmonycentral.com/articles/the-truth-about-panning-laws
 */
enum class PanAlgorithm
{
  Linear,
  SquareRoot,
  SineLaw
};

/**
 * @brief Balance control of stereo sources, mainly used in the mixer channels.
 *
 * @see https://www.harmonycentral.com/articles/the-truth-about-panning-laws
 */
enum class BalanceControlAlgorithm
{
  /**
   * Classic "Balance" mode.
   *
   * Attennuates one channel only with no positive gain.
   *
   * Examples:
   * hard left: mute right channel, left channel untouched.
   * mid left: attenuate right channel by (signal * pan - 0.5), left channel
   * untouched.
   */
  Linear,
};

/**
 * Calculates stereo panning gain coefficients for left and right channels.
 *
 * @param law The pan law to use for gain compensation
 * @param algo The panning algorithm that determines the curve shape
 * @param pan Pan position from 0.0 (full left) to 1.0 (full right), with 0.5
 * being center
 * @return A pair of floats {left_gain, right_gain} where each gain is in range
 * [0.0, 1.0]
 */
[[nodiscard]] std::pair<float, float>
calculate_panning (PanLaw law, PanAlgorithm algo, float pan);

/**
 * Calculates the balance control gain coefficients for the left and right
 * channels.
 *
 * The balance control algorithm attenuates one channel only with no positive
 * gain. For example, when the balance is fully left, the right channel is muted
 * while the left channel is untouched.
 *
 * @param algorithm The balance control algorithm to use
 * @param balance_control_position The balance control position, from 0.0 (full
 * left) to 1.0 (full right), with 0.5 being center
 * @return A pair of floats {left_gain, right_gain} where each gain is in the
 * range [0.0, 1.0]
 */
[[nodiscard]] std::pair<float, float>
calculate_balance_control (
  BalanceControlAlgorithm algorithm,
  float                   balance_control_position);

}; // namespace zrythm::dsp

DEFINE_ENUM_FORMATTER (
  zrythm::dsp::PanAlgorithm,
  PanAlgorithm,
  QT_TR_NOOP_UTF8 ("Linear"),
  QT_TR_NOOP_UTF8 ("Square Root"),
  QT_TR_NOOP_UTF8 ("Sine"));

DEFINE_ENUM_FORMATTER (
  zrythm::dsp::PanLaw,
  PanLaw,
  /* TRANSLATORS: decibels */
  QT_TR_NOOP_UTF8 ("0dB"),
  QT_TR_NOOP_UTF8 ("-3dB"),
  QT_TR_NOOP_UTF8 ("-6dB"));

DEFINE_ENUM_FORMATTER (
  zrythm::dsp::BalanceControlAlgorithm,
  BalanceControlAlgorithm,
  QT_TR_NOOP_UTF8 ("Linear"));

#endif
