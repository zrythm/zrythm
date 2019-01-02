/*
 * utils/io.h - IO utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */
#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include <stdio.h>

/**
 * Gets directory part of filename. MUST be freed.
 */
char *
io_get_dir (const char * filename); ///< filename containing directory

/**
 * Makes directory if doesn't exist.
 */
void
io_mkdir (const char * dir);

/**
 * Gets home dir. MUST be freed.
 */
char *
io_get_home_dir ();

/**
 * Creates the file if doesn't exist
 */
FILE *
io_touch_file (const char * filename);

/**
 * Strips extensions from given filename.
 */
char *
io_file_strip_ext (const char * filename);

/**
 * Strips path from given filename.
 *
 * MUST be freed.
 */
char *
io_path_get_basename (const char * filename);

char *
io_file_get_creation_datetime (const char * filename);

char *
io_file_get_last_modified_datetime (const char * filename);
#endif

