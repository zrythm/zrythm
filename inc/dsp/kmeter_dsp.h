// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
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

#include <stdbool.h>

typedef struct KMeterDsp
{
  float z1;   // filter state
  float z2;   // filter state
  float rms;  // max rms value since last read()
  float peak; // max peak value since last read()
  int   cnt;  // digital peak hold counter
  int   fpp;  // frames per period
  float fall; // peak fallback
  bool  flag; // flag set by read(), resets _rms

  float omega; // ballistics filter constant.
  int   hold;  // peak hold timeoute
  float fsamp; // sample-rate
} KMeterDsp;

/**
 * Process.
 *
 * @param p Frame array.
 * @param n Number of samples.
 */
void
kmeter_dsp_process (KMeterDsp * self, float * p, int n);

float
kmeter_dsp_read_f (KMeterDsp * self);

void
kmeter_dsp_read (KMeterDsp * self, float * rms, float * peak);

void
kmeter_dsp_reset (KMeterDsp * self);

/**
 * Init with the samplerate.
 */
void
kmeter_dsp_init (KMeterDsp * self, float samplerate);

MALLOC
KMeterDsp *
kmeter_dsp_new (void);

void
kmeter_dsp_free (KMeterDsp * self);

#endif
