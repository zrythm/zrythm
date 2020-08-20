/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include <math.h>

#include "utils/dsp.h"
#include "utils/math.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#ifdef HAVE_LSP_DSP
#include <lsp-plug.in/dsp/dsp.h>
#endif

/**
 * Fill the buffer with the given value.
 */
void
dsp_fill (
  float * buf,
  float   val,
  size_t  size)
{
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_fill (buf, val, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          buf[i] = val;
        }
#ifdef HAVE_LSP_DSP
    }
#endif
}

/**
 * Clamp the buffer to min/max.
 */
void
dsp_clamp (
  float * buf,
  float   minf,
  float   maxf,
  size_t  size)
{
  for (size_t i = 0; i < size; i++)
    {
      buf[i] = CLAMP (buf[i], minf, maxf);
    }
}

/**
 * Gets the absolute peak of the buffer.
 *
 * @return Whether the peak changed.
 */
bool
dsp_abs_peak (
  float * buf,
  float * cur_peak,
  size_t  size)
{
  float new_peak = *cur_peak;

  for (size_t i = 0; i < size; i++)
    {
      float val = fabsf (buf[i]);
      if (val > new_peak)
        {
          new_peak = val;
        }
    }

  bool changed =
    !math_floats_equal (new_peak, *cur_peak);
  *cur_peak = new_peak;

  return changed;
}

/**
 * Calculate dest[i] = dest[i] * k1 + src[i] * k2.
 */
void
dsp_mix2 (
  float *       dest,
  const float * src,
  float         k1,
  float         k2,
  size_t        size)
{
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_mix2 (dest, src, k1, k2, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] = dest[i] * k1 + src[i] * k2;
        }
#ifdef HAVE_LSP_DSP
    }
#endif
}

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
  size_t        size)
{
#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_mix_add2 (
        dest, src1, src2, k1, k2, size);
    }
  else
    {
#endif
      for (size_t i = 0; i < size; i++)
        {
          dest[i] =
            dest[i] + src1[i] * k1 + src2[i] * k2;
        }
#ifdef HAVE_LSP_DSP
    }
#endif
}
