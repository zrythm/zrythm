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
 * File utilities.
 */

#ifndef __UTILS_FILE_H__
#define __UTILS_FILE_H__

#include <stdio.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns 1 if the file/dir exists.
 */
#define file_exists(file) \
  g_file_test (file, G_FILE_TEST_EXISTS)

char *
file_path_relative_to (const char * path, const char * base);

int
file_symlink (const char * old_path, const char * new_path);

/**
 * Do cp --reflink from \ref src to \ref dest.
 *
 * @return Non-zero on error.
 */
int
file_reflink (const char * dest, const char * src);

/**
 * @}
 */

#endif
