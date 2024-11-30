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
 * Calculates stereo panning gains for left and right channels.
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

#endif
