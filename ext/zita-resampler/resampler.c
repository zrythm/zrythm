// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense OR GPL-3.0-or-later
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2006-2012 Fons Adriaensen <fons@linuxaudio.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ---
 */

/*
 * This is a C-ified version of the original code.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../zita-resampler/resampler.h"

static unsigned int gcd (
  unsigned int a, unsigned int b)
{
  if (a == 0) return b;
  if (b == 0) return a;
  while (1)
    {
      if (a > b)
        {
          a = a % b;
          if (a == 0) return b;
          if (a == 1) return 1;
        }
      else
        {
          b = b % a;
          if (b == 0) return a;
          if (b == 1) return 1;
        }
    }
  return 1;
}

ZitaResampler *
zita_resampler_new (void)
{
  ZitaResampler * self =
    calloc (1, sizeof (ZitaResampler));

  zita_resampler_reset (self);

  return self;
}

void
zita_resampler_free (
  ZitaResampler * self)
{
  zita_resampler_clear (self);

  free (self);
}

int
zita_resampler_setup (
  ZitaResampler * self,
  unsigned int fs_inp,
  unsigned int fs_out,
  unsigned int nchan,
  unsigned int hlen)
{
  if ((hlen < 8) || (hlen > 96)) return 1;
  return
    zita_resampler_setup_with_frel (
      self, fs_inp, fs_out, nchan, hlen,
      1.0 - 2.6 / hlen);
}

int
zita_resampler_setup_with_frel (
  ZitaResampler * self,
  unsigned int fs_inp,
  unsigned int fs_out,
  unsigned int nchan,
  unsigned int hlen,
  double       frel)
{
  unsigned int       g, h, k, n, s;
  double             r;
  float              *B = 0;
  ZitaResamplerTable    *T = 0;

  k = s = 0;
  if (fs_inp && fs_out && nchan)
    {
      r = (double) fs_out / (double) fs_inp;
      g = gcd (fs_out, fs_inp);
      n = fs_out / g;
      s = fs_inp / g;
      if ((16 * r >= 1) && (n <= 1000))
        {
          h = hlen;
          k = 250;
          if (r < 1)
            {
              frel *= r;
              h = (unsigned int)(ceil (h / r));
              k = (unsigned int)(ceil (k / r));
            }
          T = zita_resampler_table_create (frel, h, n);
          B =
            malloc (
              sizeof (float) *
              nchan * (2 * h - 1 + k));
        }
    }
  zita_resampler_clear (self);

  if (T)
    {
      self->table = T;
      self->buff  = B;
      self->nchan = nchan;
      self->inmax = k;
      self->pstep = s;

      return zita_resampler_reset (self);
    }
  else
    return 1;
}

void
zita_resampler_clear (
  ZitaResampler * self)
{
  zita_resampler_table_destroy (self->table);
  free (self->buff);

  self->buff  = 0;
  self->table = 0;
  self->nchan = 0;
  self->inmax = 0;
  self->pstep = 0;

  zita_resampler_reset (self);
}

double
zita_resampler_inpdist (
  ZitaResampler * self)
{
  if (!self->table) return 0;
    return
      (int)
      (self->table->hl + 1 - self->nread) -
      (double) self->phase / self->table->np;
}

int
zita_resampler_inpsize (
  ZitaResampler * self)
{
  if (!self->table)
    return 0;

  return 2 * self->table->hl;
}

int
zita_resampler_reset (
  ZitaResampler * self)
{
  if (!self->table)
    return 1;

  self->inp_count = 0;
  self->out_count = 0;
  self->inp_data = 0;
  self->out_data = 0;
  self->index = 0;
  self->nread = 0;
  self->nzero = 0;
  self->phase = 0;
  if (self->table)
    {
      self->nread = 2 * self->table->hl;
      return 0;
    }
  return 1;
}

int
zita_resampler_process (
  ZitaResampler * self)
{
  unsigned int   hl, ph, np, dp, in, nr, nz, i, n, c;
  float          *p1, *p2;

  if (!self->table) return 1;

  hl = self->table->hl;
  np = self->table->np;
  dp = self->pstep;
  in = self->index;
  nr = self->nread;
  ph = self->phase;
  nz = self->nzero;
  n = (2 * hl - nr) * self->nchan;
  p1 = self->buff + in * self->nchan;
  p2 = p1 + n;

  while (self->out_count)
    {
      if (nr)
        {
          if (self->inp_count == 0)
            break;

          if (self->inp_data)
            {
              for (c = 0; c < self->nchan; c++)
                p2 [c] = self->inp_data [c];

              self->inp_data += self->nchan;

              nz = 0;
            }
          else
            {
              for (c = 0; c < self->nchan; c++)
                p2 [c] = 0;

              if (nz < 2 * hl)
                nz++;
            }
          nr--;
          p2 += self->nchan;
          self->inp_count--;
        }
      else
        {
          if (self->out_data)
            {
              if (nz < 2 * hl)
                {
                  float *c1 =
                    self->table->ctab + hl * ph;
                  float *c2 =
                    self->table->ctab + hl * (np - ph);
                  for (c = 0; c < self->nchan; c++)
                    {
                      float *q1 = p1 + c;
                      float *q2 = p2 + c;
                      float s = 1e-20f;
                      for (i = 0; i < hl; i++)
                        {
                          q2 -= self->nchan;
                          s +=
                            *q1 * c1 [i] +
                            *q2 * c2 [i];
                          q1 += self->nchan;
                        }
                      *self->out_data++ = s - 1e-20f;
                    }
                }
              else
                {
                  for (c = 0; c < self->nchan; c++)
                    *self->out_data++ = 0;
                }
            }
          self->out_count--;

          ph += dp;
          if (ph >= np)
            {
              nr = ph / np;
              ph -= nr * np;
              in += nr;
              p1 += nr * self->nchan;;
              if (in >= self->inmax)
                {
                  n = (2 * hl - nr) * self->nchan;
                  memcpy (
                    self->buff, p1, n * sizeof (float));
                  in = 0;
                  p1 = self->buff;
                  p2 = p1 + n;
                }
            }
        }
    }

  self->index = in;
  self->nread = nr;
  self->phase = ph;
  self->nzero = nz;

  return 0;
}
