// SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
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

#ifndef __ZITA_RESAMPLER_RESAMPLER_H__
#define __ZITA_RESAMPLER_RESAMPLER_H__

#include "resampler_table.h"

typedef struct ZitaResampler
{
  unsigned int         inp_count;
  unsigned int         out_count;
  float               *inp_data;
  float               *out_data;
  void                *inp_list;
  void                *out_list;

  ZitaResamplerTable * table;
  unsigned int         nchan;
  unsigned int         inmax;
  unsigned int         index;
  unsigned int         nread;
  unsigned int         nzero;
  unsigned int         phase;
  unsigned int         pstep;
  float               *buff;
  void                *dummy [8];
} ZitaResampler;

ZitaResampler *
zita_resampler_new (void);

void
zita_resampler_free (
  ZitaResampler * self);

int
zita_resampler_setup (
  ZitaResampler * self,
  unsigned int fs_inp,
  unsigned int fs_out,
  unsigned int nchan,
  unsigned int hlen);

int
zita_resampler_setup_with_frel (
  ZitaResampler * self,
  unsigned int fs_inp,
  unsigned int fs_out,
  unsigned int nchan,
  unsigned int hlen,
  double       frel);

void
zita_resampler_clear (
  ZitaResampler * self);

int
zita_resampler_reset (
  ZitaResampler * self);

int
zita_resampler_inpsize (
  ZitaResampler * self);

double
zita_resampler_inpdist (
  ZitaResampler * self);

int
zita_resampler_process (
  ZitaResampler * self);

#endif
