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

/**
 * \file
 *
 * Terminal utilities.
 */

#ifndef __UTILS_TERMINAL_H__
#define __UTILS_TERMINAL_H__

/* ANSI color codes */
#define TERMINAL_COLOR_RED "\x1b[31m"
#define TERMINAL_COLOR_GREEN "\x1b[32m"
#define TERMINAL_COLOR_YELLOW "\x1b[33m"
#define TERMINAL_COLOR_LIGHT_YELLOW "\x1b[93m"
#define TERMINAL_COLOR_BLUE "\x1b[34m"
#define TERMINAL_COLOR_LIGHTBLUE "\x1b[94m"
#define TERMINAL_COLOR_MAGENTA "\x1b[35m"
#define TERMINAL_COLOR_LIGHT_PURPLE "\x1b[95m"
#define TERMINAL_COLOR_CYAN "\x1b[36m"

#define TERMINAL_BOLD "\x1b[1m"
#define TERMINAL_ITALIC "\x1b[3m"
#define TERMINAL_UNDERLINE "\x1b[4m"

#define TERMINAL_RESET "\x1b[0m"

#endif
