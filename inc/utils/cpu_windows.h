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
 * CPU usage on windows.
 */

#ifndef __UTILS_CPU_WINDOWS_H__
#define __UTILS_CPU_WINDOWS_H__

#ifdef _WOE32

#  ifdef __cplusplus
extern "C" {
#  endif

/**
 * Return the CPU usage as a percentage/
 *
 * @param pid The process ID to check, or -1 to check
 *   the current process.
 */
int
cpu_windows_get_usage (int pid);

#  ifdef __cplusplus
}
#  endif

#endif // _WOE32

#endif
