/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils/color.h"
#include "utils/math.h"

/**
 * Brightens the color by the given amount.
 */
void
color_brighten (
  GdkRGBA * src, double val)
{
  src->red = MIN (src->red + val, 1.0);
  src->green = MIN (src->green + val, 1.0);
  src->blue = MIN (src->blue + val, 1.0);
}

/**
 * Brightens the color by the default amount.
 */
void
color_brighten_default (
  GdkRGBA * src)
{
  color_brighten (src, COLOR_DEFAULT_BRIGHTEN_VAL);
}

/**
 * Darkens the color by the given amount.
 */
void
color_darken (
  GdkRGBA * src, double val)
{
  src->red = MAX (src->red - val, 0.0);
  src->green = MAX (src->green - val, 0.0);
  src->blue = MAX (src->blue - val, 0.0);
}

/**
 * Darkens the color by the default amount.
 */
void
color_darken_default (
  GdkRGBA * src)
{
  color_darken (src, COLOR_DEFAULT_BRIGHTEN_VAL);
}

/**
 * Returns whether the color is the same.
 */
bool
color_is_same (
  GdkRGBA * src,
  GdkRGBA * dest)
{
  return
    math_doubles_equal (src->red, dest->red) &&
    math_doubles_equal (src->green, dest->green) &&
    math_doubles_equal (src->blue, dest->blue) &&
    math_doubles_equal (src->alpha, dest->alpha);
}

/**
 * Returns if the color is bright or not.
 */
bool
color_is_bright (
  GdkRGBA * src)
{
  return src->red + src->green + src->blue >= 1.5;
}

/**
 * Returns if the color is very bright or not.
 */
bool
color_is_very_bright (
  GdkRGBA * src)
{
  return src->red + src->green + src->blue >= 2.0;
}
