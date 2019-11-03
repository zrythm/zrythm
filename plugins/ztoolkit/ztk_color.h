/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __Z_TOOLKIT_ZTK_COLOR_H__
#define __Z_TOOLKIT_ZTK_COLOR_H__

#include <cairo.h>

/**
 * Color.
 */
typedef struct ZtkColor
{
  double     red;
  double     green;
  double     blue;
  double     alpha;
} ZtkColor;

void
ztk_color_set_for_cairo (
  ZtkColor * color,
  cairo_t *  cr);

#endif
