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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "resampler_table.h"

static ZitaResamplerTable * list = NULL;
static bool mutex_inited = false;
static pthread_mutex_t mutex;

static double sinc (double x)
{
  x = fabs (x);
  if (x < 1e-6)
    return 1.0;
  x *= M_PI;
  return sin (x) / x;
}

static double wind (double x)
{
  x = fabs (x);
  if (x >= 1.0)
    return 0.0f;
  x *= M_PI;
  return
    0.384 + 0.500 * cos (x) + 0.116 * cos (2 * x);
}

ZitaResamplerTable *
zita_resampler_table_new (
  double       fr,
  unsigned int hl,
  unsigned int np)
{
  unsigned int  i, j;
  double        t;
  float         *p;

  ZitaResamplerTable * self =
    calloc (1, sizeof (ZitaResamplerTable));
  self->fr = fr;
  self->hl = hl;
  self->np = np;

  self->ctab =
    malloc (sizeof (float) * hl * (np + 1));
  p = self->ctab;
  for (j = 0; j <= np; j++)
    {
      t = (double) j / (double) np;
      for (i = 0; i < hl; i++)
        {
          p[hl - i - 1] =
            (float)
            (fr * sinc (t * fr) * wind (t / hl));
          t += 1;
        }
      p += hl;
    }

  return self;
}

void
zita_resampler_table_free (
  ZitaResamplerTable * self)
{
  free (self->ctab);
  free (self);
}

ZitaResamplerTable *
zita_resampler_table_create (
  double       fr,
  unsigned int hl,
  unsigned int np)
{
  ZitaResamplerTable * P;

  if (!mutex_inited)
    {
      pthread_mutex_init (&mutex, 0);
    }

  pthread_mutex_lock (&mutex);
  P = list;
  while (P)
    {
      if ((fr >= P->fr * 0.999) &&
          (fr <= P->fr * 1.001) &&
          (hl == P->hl) && (np == P->np))
        {
          P->refc++;
          pthread_mutex_unlock (&mutex);
          return P;
        }
      P = P->next;
    }
  P = zita_resampler_table_new (fr, hl, np);
  P->refc = 1;
  P->next = list;
  list = P;
  pthread_mutex_unlock (&mutex);

  return P;
}

void
zita_resampler_table_destroy (
  ZitaResamplerTable * T)
{
  ZitaResamplerTable *P, *Q;

  pthread_mutex_unlock (&mutex);
  if (T)
    {
      T->refc--;
      if (T->refc == 0)
        {
          P = list;
          Q = 0;
          while (P)
            {
              if (P == T)
                {
                  if (Q)
                    Q->next = T->next;
                  else
                    list = T->next;
                  break;
                }
              Q = P;
              P = P->next;
            }
          zita_resampler_table_free (T);
        }
    }
  pthread_mutex_unlock (&mutex);
}

void
zita_resampler_table_print_list (void)
{
  ZitaResamplerTable *P;

  printf ("Resampler table\n----\n");
  for (P = list; P; P = P->next)
  {
printf (
  "refc = %3d   fr = %10.6lf  hl = %4d  np = %4d\n",
  P->refc, P->fr, P->hl, P->np);
  }
  printf ("----\n\n");
}
