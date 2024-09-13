// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
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

#ifndef __RESAMPLER_TABLE_H
#define __RESAMPLER_TABLE_H

#include <mutex>

#define ZITA_RESAMPLER_MAJOR_VERSION 1
#define ZITA_RESAMPLER_MINOR_VERSION 11

extern int
zita_resampler_major_version (void);
extern int
zita_resampler_minor_version (void);

class Resampler_mutex
{
private:
  friend class Resampler_table;

  void lock () { _mutex.lock (); }
  void unlock () { _mutex.unlock (); }

  std::mutex _mutex;
};

class Resampler_table
{
public:
  static void print_list (void);

private:
  Resampler_table (double fr, unsigned int hl, unsigned int np);
  ~Resampler_table (void);

  friend class Resampler;
  friend class VResampler;

  Resampler_table * _next;
  unsigned int      _refc;
  float *           _ctab;
  double            _fr;
  unsigned int      _hl;
  unsigned int      _np;

  static Resampler_table * create (double fr, unsigned int hl, unsigned int np);
  static void              destroy (Resampler_table * T);

  static Resampler_table * _list;
  static Resampler_mutex   _mutex;
};

#endif
