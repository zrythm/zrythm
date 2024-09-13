// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
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
 */

#ifndef __AUDIO_TRUE_PEAK_DSP__
#define __AUDIO_TRUE_PEAK_DSP__

#include "zita-resampler/resampler.h"

class TruePeakDsp
{
public:
  TruePeakDsp ();
  ~TruePeakDsp ();

  /**
   * Process.
   *
   * @param p Frame array.
   * @param n Number of samples.
   */
  void process (float * p, int n);

  void process_max (float * p, int n);

  float read_f ();

  void read (float * m, float * p);

  void reset ();

  /**
   * Init with the samplerate.
   */
  void init (float samplerate);

private:
  float           m_ = 0.0f;
  float           p_ = 0.0f;
  float           z1_ = 0.0f;
  float           z2_ = 0.0f;
  bool            res_ = false;
  float *         buf_ = nullptr;
  zita::Resampler src_;

  float w1_ = 0.0f; // attack filter coefficient
  float w2_ = 0.0f; // attack filter coefficient
  float w3_ = 0.0f; // release filter coefficient
  float g_ = 1.0f;  // gain factor
};

#endif
