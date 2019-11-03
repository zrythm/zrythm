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

#include "ztoolkit/ztk_theme.h"

/**
 * Inits the theme to the default colors.
 */
void
ztk_theme_init (
  ZtkTheme * self)
{
  ZtkColor bg_color = {
    45.0 / 255.0, 45.0 / 255.0, 45.0 / 255.0,
    1.0 };
  self->bg_color = bg_color;
  ZtkColor bg_dark_variant2 = {
    31.0 / 255.0, 31.0 / 255.0, 31.0 / 255.0,
    1.0 };
  self->bg_dark_variant2 = bg_dark_variant2;
  ZtkColor bright_green = {
    29.0 / 255.0, 221.0 / 255.0, 106.0 / 255.0,
    1.0 };
  self->bright_green = bright_green;
  ZtkColor darkish_green = {
    25.0 / 255.0, 102.0 / 255.0, 76.0 / 255.0,
    1.0 };
  self->darkish_green = darkish_green;
  ZtkColor dark_orange = {
    214.0 / 255.0, 138.0 / 255.0, 12.0 / 255.0,
    1.0 };
  self->dark_orange = dark_orange;
  ZtkColor z_yellow = {
    249.0 / 255.0, 202.0 / 255.0, 27.0 / 255.0,
    1.0 };
  self->z_yellow = z_yellow;
  ZtkColor bright_orange = {
    247.0 / 255.0, 150.0 / 255.0, 22.0 / 255.0,
    1.0 };
  self->bright_orange = bright_orange;
  ZtkColor z_purple = {
    157.0 / 255.0, 57.0 / 255.0, 85.0 / 255.0,
    1.0 };
  self->z_purple = z_purple;
  ZtkColor matcha_green = {
    46.0 / 255.0, 179.0 / 255.0, 152.0 / 255.0,
    1.0 };
  self->matcha_green = matcha_green;
}
