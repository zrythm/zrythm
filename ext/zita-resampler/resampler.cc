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

#include "zita-resampler/resampler.h"

static unsigned int
gcd (unsigned int a, unsigned int b)
{
  if (a == 0)
    return b;
  if (b == 0)
    return a;
  while (1)
    {
      if (a > b)
        {
          a = a % b;
          if (a == 0)
            return b;
          if (a == 1)
            return 1;
        }
      else
        {
          b = b % a;
          if (b == 0)
            return a;
          if (b == 1)
            return 1;
        }
    }
  return 1;
}

Resampler::Resampler (void) : _table (0), _nchan (0), _buff (0)
{
  reset ();
}

Resampler::~Resampler (void)
{
  clear ();
}

int
Resampler::setup (
  unsigned int fs_inp,
  unsigned int fs_out,
  unsigned int nchan,
  unsigned int hlen)
{
  return setup (fs_inp, fs_out, nchan, hlen, 1.0 - 2.6 / hlen);
}

int
Resampler::setup (
  unsigned int fs_inp,
  unsigned int fs_out,
  unsigned int nchan,
  unsigned int hlen,
  double       frel)
{
  unsigned int      np, dp, mi, hl, n;
  double            r;
  Resampler_table * T = 0;

  if (!nchan || (hlen < 8) || (hlen > 96))
    {
      clear ();
      return 1;
    }

  r = (double) fs_out / (double) fs_inp;
  n = gcd (fs_out, fs_inp);
  np = fs_out / n;
  dp = fs_inp / n;
  if ((64 * r < 1.0) || (np > 1000))
    {
      clear ();
      return 1;
    }

  hl = hlen;
  mi = 32;
  if (r < 1.0)
    {
      frel *= r;
      hl = (unsigned int) (ceil (hl / r));
      mi = (unsigned int) (ceil (mi / r));
    }
#ifdef ENABLE_VEC4
  hl = (hl + 3) & ~3;
#endif
  T = Resampler_table::create (frel, hl, np);

  clear ();
  if (T)
    {
      _table = T;
      n = nchan * (2 * hl + mi);
#ifdef ENABLE_VEC4
      posix_memalign ((void **) (&_buff), 16, n * sizeof (float));
      memset (_buff, 0, n * sizeof (float));
#else
      _buff = new float[n];
#endif
      _nchan = nchan;
      _inmax = mi;
      _pstep = dp;
      return reset ();
    }
  else
    return 1;
}

void
Resampler::clear (void)
{
  Resampler_table::destroy (_table);
#ifdef ENABLE_VEC4
  free (_buff);
#else
  delete[] _buff;
#endif
  _buff = 0;
  _table = 0;
  _nchan = 0;
  _inmax = 0;
  _pstep = 0;
  reset ();
}

double
Resampler::inpdist (void) const
{
  if (!_table)
    return 0;
  return (int) (_table->_hl + 1 - _nread) - (double) _phase / _table->_np;
}

int
Resampler::inpsize (void) const
{
  if (!_table)
    return 0;
  return 2 * _table->_hl;
}

int
Resampler::reset (void)
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
Resampler::process (void)
{
  unsigned int hl, np, ph, dp, in, nr, nz, di, i, j, n;
  float *      c1, *c2, *p1, *p2, *q1, *q2;

  if (!_table)
    return 1;
  hl = _table->_hl;
  np = _table->_np;
  dp = _pstep;
  in = _index;
  nr = _nread;
  nz = _nzero;
  ph = _phase;

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
              c1 = _table->_ctab + hl * ph;
              c2 = _table->_ctab + hl * (np - ph);

#if defined(ENABLE_SSE2)
              __m128 C1, C2, Q1, Q2, S;
              for (j = 0; j < _nchan; j++)
                {
                  q1 = p1 + j * di;
                  q2 = p2 + j * di;
                  S = _mm_setzero_ps ();
                  for (i = 0; i < hl; i += 4)
                    {
                      C1 = _mm_load_ps (c1 + i);
                      Q1 = _mm_loadu_ps (q1);
                      q2 -= 4;
                      S = _mm_add_ps (S, _mm_mul_ps (C1, Q1));
                      C2 = _mm_loadr_ps (c2 + i);
                      Q2 = _mm_loadu_ps (q2);
                      q1 += 4;
                      S = _mm_add_ps (S, _mm_mul_ps (C2, Q2));
                    }
                  *out_data++ = S[0] + S[1] + S[2] + S[3];
                }

#elif defined(ENABLE_NEON)
              // ARM64 version by Nicolas Belin <nbelin@baylibre.com>
              float32x4_t * C1 = (float32x4_t *) c1;
              float32x4_t * C2 = (float32x4_t *) c2;
              float32x4_t   S, T;
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
              for (j = 0; j < _nchan; j++)
                {
                  q1 = p1 + j * di;
                  q2 = p2 + j * di;
                  s = 1e-30f;
                  for (i = 0; i < hl; i++)
                    {
                      q2--;
                      s += *q1 * c1[i] + *q2 * c2[i];
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

      ph += dp;
      if (ph >= np)
        {
          nr = ph / np;
          ph -= nr * np;
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
  _nzero = nz;

  return 0;
}
