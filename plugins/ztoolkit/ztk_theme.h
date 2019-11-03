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

#ifndef __Z_TOOLKIT_ZTK_THEME_H__
#define __Z_TOOLKIT_ZTK_THEME_H__

#include "ztoolkit/ztk_color.h"

/**
 * Theme colors.
 */
typedef struct ZtkTheme
{
  ZtkColor    bg_color;
  ZtkColor    bg_dark_variant2;
  ZtkColor    bright_green;
  ZtkColor    darkish_green;
  ZtkColor    dark_orange;
  ZtkColor    z_yellow;
  ZtkColor    bright_orange;
  ZtkColor    z_purple;
  ZtkColor    matcha_green;
} ZtkTheme;

/**
 * Inits the theme to the default colors.
 */
void
ztk_theme_init (
  ZtkTheme * self);

#endif
