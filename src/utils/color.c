/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/color.h"
#include "utils/math.h"

/**
 * Brightens the color by the given amount.
 */
void
color_brighten (
  GdkRGBA * src, float val)
{
  src->red = MIN (src->red + val, 1.f);
  src->green = MIN (src->green + val, 1.f);
  src->blue = MIN (src->blue + val, 1.f);
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
  GdkRGBA * src, float val)
{
  src->red = MAX (src->red - val, 0.f);
  src->green = MAX (src->green - val, 0.f);
  src->blue = MAX (src->blue - val, 0.f);
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
  float epsilon = 0.000001f;
  return
    math_floats_equal_epsilon (
      src->red, dest->red, epsilon) &&
    math_floats_equal_epsilon (
      src->green, dest->green, epsilon) &&
    math_floats_equal_epsilon (
      src->blue, dest->blue, epsilon) &&
    math_floats_equal_epsilon (
      src->alpha, dest->alpha, epsilon);
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

/**
 * Returns if the color is very very bright or not.
 */
bool
color_is_very_very_bright (
  GdkRGBA * src)
{
  return src->red + src->green + src->blue >= 2.5;
}

/**
 * Returns if the color is very dark or not.
 */
bool
color_is_very_dark (
  GdkRGBA * src)
{
  return src->red + src->green + src->blue < 1.f;
}

/**
 * Returns if the color is very very dark or not.
 */
bool
color_is_very_very_dark (
  GdkRGBA * src)
{
  return src->red + src->green + src->blue < 0.5f;
}

float
color_get_brightness (
  GdkRGBA * src)
{
  return (src->red + src->green + src->blue) / 3.f;
}

float
color_get_darkness (
  GdkRGBA * color)
{
  return 1.f - color_get_brightness (color);
}

void
color_get_opposite (
  GdkRGBA * src,
  GdkRGBA * dest)
{
  dest->red = 1.f - src->red;
  dest->blue = 1.f - src->blue;
  dest->green = 1.f - src->green;
}

/**
 * Morphs from a to b, depending on the given amount.
 *
 * Eg, if @a amt is 0, the resulting color will be
 * @a a. If @a amt is 1, the resulting color will be
 * @b.
 */
void
color_morph (
  GdkRGBA * a,
  GdkRGBA * b,
  float     amt,
  GdkRGBA * result)
{
  g_return_if_fail (amt >= 0.f);
  g_return_if_fail (amt <= 1.f);
  g_return_if_fail (a && b && result);

  float amt_inv = 1.f - amt;
  result->red = amt_inv * a->red + amt * b->red;
  result->green = amt_inv * a->green + amt * b->green;
  result->blue = amt_inv * a->blue + amt * b->blue;
}
