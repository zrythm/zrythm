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

#include "dsp/true_peak_dsp.h"
#include "utils/dsp.h"

#include "zita-resampler/resampler.h"

namespace zrythm::dsp
{

struct TruePeakDsp::Impl
{
  zita::Resampler src_;
};

TruePeakDsp::TruePeakDsp () : impl_ (std::make_unique<Impl> ()) { }

void
TruePeakDsp::process (float * data, int n)
{
  assert (n > 0);
  assert (n <= 8192);
  impl_->src_.inp_count = static_cast<unsigned int> (n);
  impl_->src_.inp_data = data;
  impl_->src_.out_count = static_cast<unsigned int> (n * 4);
  impl_->src_.out_data = buf_.data ();
  impl_->src_.process ();

  float         v;
  float         m = res_ ? 0 : m_;
  float         p = res_ ? 0 : p_;
  float         z1 = z1_ > 20 ? 20 : (z1_ < 0 ? 0 : z1_);
  float         z2 = z2_ > 20 ? 20 : (z2_ < 0 ? 0 : z2_);
  const float * b = buf_.data ();

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
  impl_->src_.inp_count = static_cast<unsigned int> (n);
  impl_->src_.inp_data = p;
  impl_->src_.out_count = static_cast<unsigned int> (n * 4);
  impl_->src_.out_data = buf_.data ();
  impl_->src_.process ();

  float        m = res_ ? 0 : m_;
  float        v{};
  const auto * b = buf_.data ();
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

std::pair<float, float>
TruePeakDsp::read ()
{
  res_ = true;
  return { m_, p_ };
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
  impl_->src_.setup (
    static_cast<unsigned int> (samplerate),
    static_cast<unsigned int> (samplerate * 4.f), 1, 24, 1.0);
  buf_.resize (32768);

  z1_ = z2_ = .0f;
  w1_ = 4000.f / samplerate / 4.f;
  w2_ = 17200.f / samplerate / 4.f;
  w3_ = 1.0f - 7.f / samplerate / 4.f;
  g_ = 0.502f;

  std::array<float, 8192> zero{};
  utils::float_ranges::fill (zero.data (), 0.f, zero.size ());
  impl_->src_.inp_count = 8192;
  impl_->src_.inp_data = zero.data ();
  impl_->src_.out_count = 32768;
  impl_->src_.out_data = buf_.data ();
  impl_->src_.process ();
}

TruePeakDsp::~TruePeakDsp () = default;

} // namespace zrythm::dsp
