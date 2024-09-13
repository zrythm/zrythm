// SPDX-FileCopyrightText: (C) 2006-2023 Fons Adriaensen <fons@linuxaudio.org>
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------
//
//  Copyright (C) 2006-2023 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef ENABLE_VEC4
#if defined(ENABLE_SSE2)
#  define ENABLE_VEC4
#  include <pmmintrin.h>
#elif defined(ENABLE_NEON)
#  define ENABLE_VEC4
#  include <arm_neon.h>
#endif

#include "zita-resampler/vresampler.h"

VResampler::VResampler (void)
    : _table (0), _nchan (0), _buff (0), _c1 (0), _c2 (0)
{
  reset ();
}

VResampler::~VResampler (void)
{
  clear ();
}

int
VResampler::setup (double ratio, unsigned int nchan, unsigned int hlen)
{
  return setup (ratio, nchan, hlen, 1.0 - 2.6 / hlen);
}

int
VResampler::
  setup (double ratio, unsigned int nchan, unsigned int hlen, double frel)
{
  unsigned int      hl, mi, n;
  double            dp;
  Resampler_table * T = 0;

  if (!nchan || (hlen < 8) || (hlen > 96) || (64 * ratio < 1) || (ratio > 256))
    {
      clear ();
      return 1;
    }

  dp = NPHASE / ratio;
  hl = hlen;
  mi = 32;
  if (ratio < 1.0)
    {
      frel *= ratio;
      hl = (unsigned int) (ceil (hl / ratio));
      mi = (unsigned int) (ceil (mi / ratio));
    }
#ifdef ENABLE_VEC4
  hl = (hl + 3) & ~3;
#endif
  T = Resampler_table::create (frel, hl, NPHASE);
  clear ();
  if (T)
    {
      _table = T;
      n = nchan * (2 * hl + mi);
#ifdef ENABLE_VEC4
      posix_memalign ((void **) (&_buff), 16, n * sizeof (float));
      posix_memalign ((void **) (&_c1), 16, hl * sizeof (float));
      posix_memalign ((void **) (&_c2), 16, hl * sizeof (float));
#else
      _buff = new float[n];
      _c1 = new float[hl];
      _c2 = new float[hl];
#endif
      _nchan = nchan;
      _ratio = ratio;
      _inmax = mi;
      _pstep = dp;
      _qstep = dp;
      _wstep = 1;
      return reset ();
    }
  else
    return 1;
}

void
VResampler::clear (void)
{
  Resampler_table::destroy (_table);
#ifdef ENABLE_VEC4
  free (_buff);
  free (_c1);
  free (_c2);
#else
  delete[] _buff;
  delete[] _c1;
  delete[] _c2;
#endif
  _buff = 0;
  _c1 = 0;
  _c2 = 0;
  _table = 0;
  _nchan = 0;
  _inmax = 0;
  _pstep = 0;
  _qstep = 0;
  _wstep = 1;
  reset ();
}

void
VResampler::set_phase (double p)
{
  if (!_table)
    return;
  _phase = (p - floor (p)) * _table->_np;
}

void
VResampler::set_rrfilt (double t)
{
  if (!_table)
    return;
  _wstep = (t < 1) ? 1 : 1 - exp (-1 / t);
}

void
VResampler::set_rratio (double r)
{
  if (!_table)
    return;
  if (r > 16.0)
    r = 16.0;
  if (r < 0.95)
    r = 0.95;
  _qstep = _table->_np / (_ratio * r);
}

double
VResampler::inpdist (void) const
{
  if (!_table)
    return 0;
  return (int) (_table->_hl + 1 - _nread) - _phase / _table->_np;
}

int
VResampler::inpsize (void) const
{
  if (!_table)
    return 0;
  return 2 * _table->_hl;
}

int
VResampler::reset (void)
{
  if (!_table)
    return 1;

  inp_count = 0;
  out_count = 0;
  inp_data = 0;
  out_data = 0;
  _index = 0;
  _nread = 0;
  _nzero = 0;
  _phase = 0;
  if (_table)
    {
      _nread = 2 * _table->_hl;
      return 0;
    }
  return 1;
}

int
VResampler::process (void)
{
  int          nr, np, hl, nz, di, i, n;
  unsigned int in, j;
  double       ph, dp, dd;
  float        a, b, *p1, *p2, *q1, *q2;

  if (!_table)
    return 1;

  hl = _table->_hl;
  np = _table->_np;
  in = _index;
  nr = _nread;
  nz = _nzero;
  ph = _phase;
  dp = _pstep;

  p1 = _buff + in;
  p2 = p1 + 2 * hl - nr;
  di = 2 * hl + _inmax;

  while (out_count)
    {
      while (nr && inp_count)
        {
          if (inp_data)
            {
              for (j = 0; j < _nchan; j++)
                p2[j * di] = inp_data[j];
              inp_data += _nchan;
              nz = 0;
            }
          else
            {
              for (j = 0; j < _nchan; j++)
                p2[j * di] = 0;
              if (nz < 2 * hl)
                nz++;
            }
          p2++;
          nr--;
          inp_count--;
        }
      if (nr)
        break;

      if (out_data)
        {
          if (nz < 2 * hl)
            {
              n = (unsigned int) ph;
              b = (float) (ph - n);
              a = 1.0f - b;
              q1 = _table->_ctab + hl * n;
              q2 = _table->_ctab + hl * (np - n);

#if defined(ENABLE_SSE2)
              __m128 C1, C2, Q1, Q2, S;
              C1 = _mm_load1_ps (&a);
              C2 = _mm_load1_ps (&b);
              for (i = 0; i < hl; i += 4)
                {
                  Q1 = _mm_load_ps (q1 + i);
                  Q2 = _mm_load_ps (q1 + i + hl);
                  S = _mm_add_ps (_mm_mul_ps (Q1, C1), _mm_mul_ps (Q2, C2));
                  _mm_store_ps (_c1 + i, S);
                  Q1 = _mm_load_ps (q2 + i);
                  Q2 = _mm_load_ps (q2 + i - hl);
                  S = _mm_add_ps (_mm_mul_ps (Q1, C1), _mm_mul_ps (Q2, C2));
                  _mm_store_ps (_c2 + i, S);
                }
              for (j = 0; j < _nchan; j++)
                {
                  q1 = p1 + j * di;
                  q2 = p2 + j * di;
                  S = _mm_setzero_ps ();
                  for (i = 0; i < hl; i += 4)
                    {
                      C1 = _mm_load_ps (_c1 + i);
                      Q1 = _mm_loadu_ps (q1);
                      q2 -= 4;
                      S = _mm_add_ps (S, _mm_mul_ps (C1, Q1));
                      C2 = _mm_loadr_ps (_c2 + i);
                      Q2 = _mm_loadu_ps (q2);
                      q1 += 4;
                      S = _mm_add_ps (S, _mm_mul_ps (C2, Q2));
                    }
                  *out_data++ = S[0] + S[1] + S[2] + S[3];
                }

#elif defined(ENABLE_NEON)
              // ARM64 version by Nicolas Belin <nbelin@baylibre.com>
              float32x4_t * C1 = (float32x4_t *) _c1;
              float32x4_t * C2 = (float32x4_t *) _c2;
              float32x4_t   S, T;
              for (i = 0; i < (hl >> 2); i++)
                {
                  T = vmulq_n_f32 (vld1q_f32 (q1 + hl), b);
                  C1[i] = vmlaq_n_f32 (T, vld1q_f32 (q1), a);
                  T = vmulq_n_f32 (vld1q_f32 (q2 - hl), b);
                  C2[i] = vmlaq_n_f32 (T, vld1q_f32 (q2), a);
                  q2 += 4;
                  q1 += 4;
                }
              for (j = 0; j < _nchan; j++)
                {
                  q1 = p1 + j * di;
                  q2 = p2 + j * di - 4;
                  T = vrev64q_f32 (vld1q_f32 (q2));
                  S = vmulq_f32 (vextq_f32 (T, T, 2), C2[0]);
                  S = vmlaq_f32 (S, vld1q_f32 (q1), C1[0]);
                  for (i = 1; i < (hl >> 2); i++)
                    {
                      q2 -= 4;
                      q1 += 4;
                      T = vrev64q_f32 (vld1q_f32 (q2));
                      S = vmlaq_f32 (S, vextq_f32 (T, T, 2), C2[i]);
                      S = vmlaq_f32 (S, vld1q_f32 (q1), C1[i]);
                    }
                  *out_data++ = vaddvq_f32 (S);
                }

#else
              float s;
              for (i = 0; i < hl; i++)
                {
                  _c1[i] = a * q1[i] + b * q1[i + hl];
                  _c2[i] = a * q2[i] + b * q2[i - hl];
                }
              for (j = 0; j < _nchan; j++)
                {
                  q1 = p1 + j * di;
                  q2 = p2 + j * di;
                  s = 1e-30f;
                  for (i = 0; i < hl; i++)
                    {
                      q2--;
                      s += *q1 * _c1[i] + *q2 * _c2[i];
                      q1++;
                    }
                  *out_data++ = s - 1e-30f;
                }
#endif
            }
          else
            {
              for (j = 0; j < _nchan; j++)
                *out_data++ = 0;
            }
        }
      out_count--;

      dd = _qstep - dp;
      if (fabs (dd) < 1e-20)
        dp = _qstep;
      else
        dp += _wstep * dd;
      ph += dp;
      if (ph >= np)
        {
          nr = (unsigned int) floor (ph / np);
          ph -= nr * np;
          ;
          in += nr;
          p1 += nr;

          if (in >= _inmax)
            {
              n = 2 * hl - nr;
              p2 = _buff;
              for (j = 0; j < _nchan; j++)
                {
                  memmove (p2 + j * di, p1 + j * di, n * sizeof (float));
                }
              in = 0;
              p1 = _buff;
              p2 = p1 + n;
            }
        }
    }

  _index = in;
  _nread = nr;
  _phase = ph;
  _pstep = dp;
  _nzero = nz;

  return 0;
}
