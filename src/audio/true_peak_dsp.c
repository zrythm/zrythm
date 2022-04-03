/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 *
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

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "audio/true_peak_dsp.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

/**
 * Process.
 *
 * @param p Frame array.
 * @param n Number of samples.
 */
void
true_peak_dsp_process (
  TruePeakDsp * self,
  float *       data,
  int           n)
{
  assert (n > 0);
  assert (n <= 8192);
  self->src->inp_count = (unsigned int) n;
  self->src->inp_data = data;
  self->src->out_count = (unsigned int) n * 4;
  self->src->out_data = self->buf;
  zita_resampler_process (self->src);

  float v;
  float m = self->res ? 0 : self->m;
  float p = self->res ? 0 : self->p;
  float z1 =
    self->z1 > 20 ? 20 : (self->z1 < 0 ? 0 : self->z1);
  float z2 =
    self->z2 > 20 ? 20 : (self->z2 < 0 ? 0 : self->z2);
  float * b = self->buf;

  while (n--)
    {
      z1 *= self->w3;
      z2 *= self->w3;

      v = fabsf (*b++);
      if (v > z1)
        z1 += self->w1 * (v - z1);
      if (v > z2)
        z2 += self->w2 * (v - z2);
      if (v > p)
        p = v;

      v = fabsf (*b++);
      if (v > z1)
        z1 += self->w1 * (v - z1);
      if (v > z2)
        z2 += self->w2 * (v - z2);
      if (v > p)
        p = v;

      v = fabsf (*b++);
      if (v > z1)
        z1 += self->w1 * (v - z1);
      if (v > z2)
        z2 += self->w2 * (v - z2);
      if (v > p)
        p = v;

      v = fabsf (*b++);
      if (v > z1)
        z1 += self->w1 * (v - z1);
      if (v > z2)
        z2 += self->w2 * (v - z2);
      if (v > p)
        p = v;

      v = z1 + z2;
      if (v > m)
        m = v;
    }

  self->z1 = z1 + 1e-20f;
  self->z2 = z2 + 1e-20f;

  m *= self->g;

  if (self->res)
    {
      self->m = m;
      self->p = p;
      self->res = false;
    }
  else
    {
      if (m > self->m)
        {
          self->m = m;
        }
      if (p > self->p)
        {
          self->p = p;
        }
    }
}

void
true_peak_dsp_process_max (
  TruePeakDsp * self,
  float *       p,
  int           n)
{
  assert (n <= 8192);
  self->src->inp_count = (unsigned int) n;
  self->src->inp_data = p;
  self->src->out_count = (unsigned int) n * 4;
  self->src->out_data = self->buf;
  zita_resampler_process (self->src);

  float   m = self->res ? 0 : self->m;
  float   v;
  float * b = self->buf;
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
  self->m = m;
}

float
true_peak_dsp_read_f (TruePeakDsp * self)
{
  self->res = true;
  return self->m;
}

void
true_peak_dsp_read (
  TruePeakDsp * self,
  float *       m,
  float *       p)
{
  self->res = true;
  *m = self->m;
  *p = self->p;
}

void
true_peak_dsp_reset (TruePeakDsp * self)
{
  self->res = true;
  self->m = 0;
  self->p = 0;
}

/**
 * Init with the samplerate.
 */
void
true_peak_dsp_init (
  TruePeakDsp * self,
  float         samplerate)
{
  zita_resampler_setup_with_frel (
    self->src, (unsigned int) samplerate,
    (unsigned int) (samplerate * 4.f), 1, 24, 1.0);
  self->buf =
    (float *) g_malloc (32768 * sizeof (float));

  self->z1 = self->z2 = .0f;
  self->w1 = 4000.f / samplerate / 4.f;
  self->w2 = 17200.f / samplerate / 4.f;
  self->w3 = 1.0f - 7.f / samplerate / 4.f;
  self->g = 0.502f;

  /* q/d initialize */
  float zero[8192];
  for (int i = 0; i < 8192; ++i)
    {
      zero[i] = 0.0;
    }
  self->src->inp_count = 8192;
  self->src->inp_data = zero;
  self->src->out_count = 32768;
  self->src->out_data = self->buf;
  zita_resampler_process (self->src);
}

TruePeakDsp *
true_peak_dsp_new (void)
{
  TruePeakDsp * self = object_new (TruePeakDsp);

  self->res = true;

  self->src = zita_resampler_new ();

  return self;
}

void
true_peak_dsp_free (TruePeakDsp * self)
{
  zita_resampler_free (self->src);
  free (self->buf);
}
