/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#ifndef __UTILS_DSP_H__
#define __UTILS_DSP_H__

#include <stdbool.h>
#include <stddef.h>

/**
 * Fill the buffer with the given value.
 */
void
dsp_fill (
  float * buf,
  float   val,
  size_t  size);

/**
 * Clamp the buffer to min/max.
 */
void
dsp_clamp (
  float * buf,
  float   minf,
  float   maxf,
  size_t  size);

/**
 * Gets the absolute peak of the buffer.
 *
 * @return Whether the peak changed.
 */
bool
dsp_abs_peak (
  float * buf,
  float * cur_peak,
  size_t  size);

/**
 * Calculate dest[i] = dest[i] * k1 + src[i] * k2.
 */
void
dsp_mix2 (
  float *       dest,
  const float * src,
  float         k1,
  float         k2,
  size_t        size);

/**
 * Calculate
 * dst[i] = dst[i] + src1[i] * k1 + src2[i] * k2.
 */
void
dsp_mix_add2 (
  float *       dest,
  const float * src1,
  const float * src2,
  float         k1,
  float         k2,
  size_t        size);

#endif
