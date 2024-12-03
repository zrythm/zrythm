// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ---
 */

#ifndef ZRYTHM_DSP_PEAK_DSP
#define ZRYTHM_DSP_PEAK_DSP

namespace zrythm::dsp
{

/**
 * @brief Performs digital peak processing on an audio signal.
 *
 * This class provides methods for processing an audio signal to
 * detect the peak and RMS values. It maintains internal state to
 * track the maximum peak and RMS values since the last read.
 *
 * The `process()` method should be called for each frame of audio
 * data, and the `read()` method can be used to retrieve the current
 * peak and RMS values.
 *
 * The `init()` method should be called to initialize the class with
 * the sample rate of the audio data.
 */
class PeakDsp
{
public:
  /**
   * Process.
   *
   * @param p Frame array.
   * @param n Number of samples.
   */
  ATTR_HOT void process (float * p, int n);

  float read_f ();

  /**
   * @brief Returns RMS and Peak values.
   *
   * @return std::pair<float, float>
   */
  std::pair<float, float> read ();

  void reset ();

  /**
   * Init with the samplerate.
   */
  void init (float samplerate);

private:
  float rms_{};  // max rms value since last read()
  float peak_{}; // max peak value since last read()
  int   cnt_{};  // digital peak hold counter
  int   fpp_{};  // frames per period
  float fall_{}; // peak fallback
  bool  flag_{}; // flag set by read(), resets rms_

  int   hold_{};  // peak hold timeoute
  float fsamp_{}; // sample-rate};
};

}; // namespace zrythm::dsp

#endif
