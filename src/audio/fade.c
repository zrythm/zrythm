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

#include "audio/curve.h"
#include "audio/fade.h"

/**
 * Gets the normalized Y for a normalized X.
 *
 * @param x Normalized x.
 * @param fade_in 1 for in, 0 for out.
 */
double
fade_get_y_normalized (
  double         x,
  CurveOptions * opts,
  int            fade_in)
{
  return curve_get_normalized_y (x, opts, !fade_in);
}
