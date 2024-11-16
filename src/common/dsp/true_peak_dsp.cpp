// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
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

#include <cassert>
#include <cmath>
#include <cstdlib>

#include "common/dsp/true_peak_dsp.h"

void
TruePeakDsp::process (float * data, int n)
{
  assert (n > 0);
  assert (n <= 8192);
  src_.inp_count = static_cast<unsigned int> (n);
  src_.inp_data = data;
  src_.out_count = static_cast<unsigned int> (n * 4);
  src_.out_data = buf_;
  src_.process ();

  float   v;
  float   m = res_ ? 0 : m_;
  float   p = res_ ? 0 : p_;
  float   z1 = z1_ > 20 ? 20 : (z1_ < 0 ? 0 : z1_);
  float   z2 = z2_ > 20 ? 20 : (z2_ < 0 ? 0 : z2_);
  float * b = buf_;

  while (n--)
    {
      z1 *= w3_;
      z2 *= w3_;

      v = fabsf (*b++);
      if (v > z1)
        z1 += w1_ * (v - z1);
      if (v > z2)
        z2 += w2_ * (v - z2);
      if (v > p)
        p = v;

      v = fabsf (*b++);
      if (v > z1)
        z1 += w1_ * (v - z1);
      if (v > z2)
        z2 += w2_ * (v - z2);
      if (v > p)
        p = v;

      v = fabsf (*b++);
      if (v > z1)
        z1 += w1_ * (v - z1);
      if (v > z2)
        z2 += w2_ * (v - z2);
      if (v > p)
        p = v;

      v = fabsf (*b++);
      if (v > z1)
        z1 += w1_ * (v - z1);
      if (v > z2)
        z2 += w2_ * (v - z2);
      if (v > p)
        p = v;

      v = z1 + z2;
      if (v > m)
        m = v;
    }

  z1_ = z1 + 1e-20f;
  z2_ = z2 + 1e-20f;

  m *= g_;

  if (res_)
    {
      m_ = m;
      p_ = p;
      res_ = false;
    }
  else
    {
      if (m > m_)
        {
          m_ = m;
        }
      if (p > p_)
        {
          p_ = p;
        }
    }
}

void
TruePeakDsp::process_max (float * p, int n)
{
  assert (n <= 8192);
  src_.inp_count = static_cast<unsigned int> (n);
  src_.inp_data = p;
  src_.out_count = static_cast<unsigned int> (n * 4);
  src_.out_data = buf_;
  src_.process ();

  float   m = res_ ? 0 : m_;
  float   v;
  float * b = buf_;
  while (n--)
    {
      v = fabsf (*b++);
      if (v > m)
        m = v;
      v = fabsf (*b++);
      if (v > m)
        m = v;
      v = fabsf (*b++);
      if (v > m)
        m = v;
      v = fabsf (*b++);
      if (v > m)
        m = v;
    }
  m_ = m;
}

float
TruePeakDsp::read_f ()
{
  res_ = true;
  return m_;
}

void
TruePeakDsp::read (float * m, float * p)
{
  res_ = true;
  *m = m_;
  *p = p_;
}

void
TruePeakDsp::reset ()
{
  res_ = true;
  m_ = 0;
  p_ = 0;
}

void
TruePeakDsp::init (float samplerate)
{
  src_.setup (
    static_cast<unsigned int> (samplerate),
    static_cast<unsigned int> (samplerate * 4.f), 1, 24, 1.0);
  buf_ = static_cast<float *> (malloc (32768 * sizeof (float)));

  z1_ = z2_ = .0f;
  w1_ = 4000.f / samplerate / 4.f;
  w2_ = 17200.f / samplerate / 4.f;
  w3_ = 1.0f - 7.f / samplerate / 4.f;
  g_ = 0.502f;

  float zero[8192];
  for (int i = 0; i < 8192; ++i)
    {
      zero[i] = 0.0;
    }
  src_.inp_count = 8192;
  src_.inp_data = zero;
  src_.out_count = 32768;
  src_.out_data = buf_;
  src_.process ();
}

TruePeakDsp::TruePeakDsp () : res_ (true) { }

TruePeakDsp::~TruePeakDsp ()
{
  if (buf_)
    free (buf_);
}
