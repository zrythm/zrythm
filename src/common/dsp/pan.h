// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Panning mono sources.
 */

#ifndef __AUDIO_PAN_H__
#define __AUDIO_PAN_H__

/** The amplitude of -3dBfs (0.707945784f). */
#define PAN_MINUS_3DB_AMP (-0.292054216f)

/** The amplitude of -6dBfs (0.501187234f). */
#define PAN_MINUS_6DB_AMP (-0.498812766f)

/**
 * These are only useful when changing mono to
 * stereo.
 *
 * No compensation is needed for stereo to stereo.
 * See
 * https://www.hackaudio.com/digital-signal-processing/stereo-audio/square-law-panning/
 */
enum class PanLaw
{
  PAN_LAW_0DB,
  PAN_LAW_MINUS_3DB,
  PAN_LAW_MINUS_6DB
};

static const char * pan_law_str[] = {
  /* TRANSLATORS: decibels */
  QT_TR_NOOP_UTF8 ("0dB"),
  QT_TR_NOOP_UTF8 ("-3dB"),
  QT_TR_NOOP_UTF8 ("-6dB"),
};

static inline const char *
pan_law_to_string (PanLaw pan_law)
{
  return pan_law_str[static_cast<int> (pan_law)];
}

/**
 * See https://www.harmonycentral.com/articles/the-truth-about-panning-laws
 */
enum class PanAlgorithm
{
  PAN_ALGORITHM_LINEAR,
  PAN_ALGORITHM_SQUARE_ROOT,
  PAN_ALGORITHM_SINE_LAW
};

static const char * pan_algorithm_str[] = {
  QT_TR_NOOP_UTF8 ("Linear"),
  QT_TR_NOOP_UTF8 ("Square Root"),
  QT_TR_NOOP_UTF8 ("Sine"),
};

static inline const char *
pan_algorithm_to_string (PanAlgorithm pan_algo)
{
  return pan_algorithm_str[static_cast<int> (pan_algo)];
}

void
pan_get_calc_lr (
  PanLaw       law,
  PanAlgorithm algo,
  float        pan,
  float *      calc_l,
  float *      calc_r);

#endif
