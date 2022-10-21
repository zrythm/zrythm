// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 */

#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include "zrythm-config.h"

#include <stdbool.h>
#include <stdio.h>

#include <glib.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Gets directory part of filename. MUST be freed.
 *
 * @param filename Filename containing directory.
 */
NONNULL
char *
io_get_dir (const char * filename);

/**
 * Makes directory if doesn't exist.
 *
 * @return 0 if the directory exists or was
 *   successfully created, -1 if error was occurred
 *   and errno is set.
 */
NONNULL
int
io_mkdir (const char * dir);

/**
 * Creates the file if doesn't exist
 */
NONNULL
FILE *
io_create_file (const char * filename);

/**
 * Touches a file similar to UNIX touch.
 */
void
io_touch_file (const char * filename);

NONNULL
char *
io_path_get_parent_dir (const char * path);

/**
 * Strips extensions from given filename.
 */
NONNULL
char *
io_file_strip_ext (const char * filename);

/**
 * Returns file extension or NULL.
 */
NONNULL
const char *
io_file_get_ext (const char * file);

#define io_path_get_basename(filename) \
  g_path_get_basename (filename)

/**
 * Strips path from given filename.
 *
 * MUST be freed.
 */
NONNULL
char *
io_path_get_basename_without_ext (const char * filename);

NONNULL
char *
io_file_get_creation_datetime (const char * filename);

/**
 * Returns the number of seconds since the epoch, or
 * -1 if failed.
 */
NONNULL
gint64
io_file_get_last_modified_datetime (const char * filename);

NONNULL
char *
io_file_get_last_modified_datetime_as_str (
  const char * filename);

/**
 * Removes the given file.
 */
NONNULL
int
io_remove (const char * path);

/**
 * Removes a dir, optionally forcing deletion.
 *
 * For safety reasons, this only accepts an
 * absolute path with length greater than 20 if
 * forced.
 */
NONNULL
int
io_rmdir (const char * path, bool force);

char **
io_get_files_in_dir_as_basenames (
  const char * dir,
  bool         allow_empty,
  bool         with_ext);

/**
 * Returns a list of the files in the given
 * directory.
 *
 * @return a NULL terminated array of strings that
 *   must be free'd with g_strfreev(), or NULL if
 *   no files were found.
 */
#define io_get_files_in_dir(dir, allow_empty) \
  io_get_files_in_dir_ending_in (dir, 0, NULL, allow_empty)

/**
 * Returns a list of the files in the given
 * directory.
 *
 * @param dir The directory to look for.
 * @param allow_empty Whether to allow returning
 *   an empty array that has only NULL, otherwise
 *   return NULL if empty.
 *
 * @return a NULL terminated array of strings that
 *   must be free'd with g_strfreev() or NULL.
 */
char **
io_get_files_in_dir_ending_in (
  const char * _dir,
  const int    recursive,
  const char * end_string,
  bool         allow_empty);

/**
 * Copies a directory.
 *
 * @note This will not work if \ref destdir_str has
 *   a file with the same filename as a directory
 *   in \ref srcdir_str.
 *
 * @see https://stackoverflow.com/questions/16453739/how-do-i-recursively-copy-a-directory-using-vala
 */
void
io_copy_dir (
  const char * destdir_str,
  const char * srcdir_str,
  bool         follow_symlinks,
  bool         recursive);

/**
 * Returns a newly allocated path that is either
 * a copy of the original path if the path does
 * not exist, or the original path appended with
 * (n), where n is a number.
 *
 * Example: "myfile" -> "myfile (1)"
 */
NONNULL
char *
io_get_next_available_filepath (const char * filepath);

/**
 * Opens the given directory using the default
 * program.
 */
NONNULL
void
io_open_directory (const char * path);

/**
 * Returns a clone of the given string after
 * removing forbidden characters.
 */
NONNULL
void
io_escape_dir_name (char * dest, const char * dir);

#define io_write_file g_file_set_contents

#ifdef _WOE32
char *
io_get_registry_string_val (const char * path);
#endif

#if defined(__APPLE__) && defined(INSTALLER_VER)
/**
 * Gets the bundle path on MacOS.
 *
 * @return Non-zero on fail.
 */
NONNULL
int
io_get_bundle_path (char * bundle_path);
#endif

/**
 * @}
 */

#endif
