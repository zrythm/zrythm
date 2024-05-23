// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
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

#ifndef __ZITA_RESAMPLER_RESAMPLER_TABLE_H__
#define __ZITA_RESAMPLER_RESAMPLER_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

typedef struct ZitaResamplerTable
{
  struct ZitaResamplerTable * next;
  unsigned int         refc;
  float *              ctab;
  double               fr;
  unsigned int         hl;
  unsigned int         np;

  struct ZitaResamplerTable * list;
  pthread_mutex_t      mutex;
} ZitaResamplerTable;

ZitaResamplerTable *
zita_resampler_table_new (
  double       fr,
  unsigned int hl,
  unsigned int np);

void
zita_resampler_table_free (
  ZitaResamplerTable * self);

ZitaResamplerTable *
zita_resampler_table_create (
  double       fr,
  unsigned int hl,
  unsigned int np);

void
zita_resampler_table_destroy (
  ZitaResamplerTable * self);

void
zita_resampler_table_print_list (void);

#ifdef __cplusplus
}
#endif

#endif
