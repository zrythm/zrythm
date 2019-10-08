/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include <stdio.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns 1 if the file/dir exists.
 */
#define io_file_exists(file) \
  g_file_test (file, G_FILE_TEST_EXISTS)

/**
 * Gets directory part of filename. MUST be freed.
 *
 * @param filename Filename containing directory.
 */
char *
io_get_dir (const char * filename);

/**
 * Makes directory if doesn't exist.
 */
void
io_mkdir (const char * dir);

/**
 * Gets home dir. MUST be freed.
 */
const char *
io_get_home_dir (void);

/**
 * Creates the file if doesn't exist
 */
FILE *
io_touch_file (const char * filename);

char *
io_path_get_parent_dir (const char * path);

/**
 * Strips extensions from given filename.
 */
char *
io_file_strip_ext (const char * filename);

/**
 * Returns file extension or NULL.
 */
char *
io_file_get_ext (const char * file);

/**
 * Strips path from given filename.
 *
 * MUST be freed.
 */
char *
io_path_get_basename (const char * filename);

char *
io_file_get_creation_datetime (
  const char * filename);

char *
io_file_get_last_modified_datetime (
  const char * filename);

/**
 * Removes the given file.
 */
int
io_remove (
  const char * path);

/**
 * Removes a dir, optionally forcing deletion.
 */
int
io_rmdir (
  const char * path,
  int          force);

/**
 * Returns a list of the files in the given
 * directory.
 *
 * @return a NULL terminated array of strings that
 *   must be free'd with g_strfreev().
 */
#define io_get_files_in_dir(dir) \
  io_get_files_in_dir_ending_in (dir, 0, NULL)

/**
 * Returns a list of the files in the given
 * directory.
 *
 * @param dir The directory to look for.
 *
 * @return a NULL terminated array of strings that
 *   must be free'd with g_strfreev().
 */
char **
io_get_files_in_dir_ending_in (
  const char * dir,
  const int    recursive,
  const char * end_string);

/**
 * Returns a newly allocated path that is either
 * a copy of the original path if the path does
 * not exist, or the original path appended with
 * (n), where n is a number.
 *
 * Example: "myfile" -> "myfile (1)"
 */
char *
io_get_next_available_filepath (
  const char * filepath);

/**
 * Opens the given directory using the default
 * program.
 */
void
io_open_directory (
  const char * path);

/**
 * @}
 */

#endif
