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

/**
 * \file
 *
 * Color utils.
 *
 */

#ifndef __UTILS_COLOR_H__
#define __UTILS_COLOR_H__

#include <stdbool.h>

#include <gtk/gtk.h>

/**
 * @addtogroup utils
 *
 * @{
 */

#define COLOR_DEFAULT_BRIGHTEN_VAL 0.1

/**
 * Brightens the color by the given amount.
 */
void
color_brighten (
  GdkRGBA * src, double val);

/**
 * Brightens the color by the default amount.
 */
void
color_brighten_default (
  GdkRGBA * src);

/**
 * Darkens the color by the given amount.
 */
void
color_darken (
  GdkRGBA * src, double val);

/**
 * Darkens the color by the default amount.
 */
void
color_darken_default (
  GdkRGBA * src);

/**
 * Returns whether the color is the same.
 */
bool
color_is_same (
  GdkRGBA * src,
  GdkRGBA * dest);

/**
 * Returns if the color is bright or not.
 */
bool
color_is_bright (
  GdkRGBA * src);

/**
 * Returns if the color is very bright or not.
 */
bool
color_is_very_bright (
  GdkRGBA * src);

/**
 * @}
 */

#endif
