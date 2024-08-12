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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#ifndef __AUDIO_KMETER_DSP__
#define __AUDIO_KMETER_DSP__

class KMeterDsp
{
public:
  /**
   * Process.
   *
   * @param p Frame array.
   * @param n Number of samples.
   */
  void process (float * p, int n);

  float read_f ();

  void read (float * rms, float * peak);

  void reset ();

  /**
   * Init with the samplerate.
   */
  void init (float samplerate);

private:
  float z1_;   // filter state
  float z2_;   // filter state
  float rms_;  // max rms value since last read()
  float peak_; // max peak value since last read()
  int   cnt_;  // digital peak hold counter
  int   fpp_;  // frames per period
  float fall_; // peak fallback
  bool  flag_; // flag set by read(), resets _rms

  float omega_; // ballistics filter constant.
  int   hold_;  // peak hold timeoute
  float fsamp_; // sample-rate
};

#endif
