// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include "zrythm-config.h"

#include <cstdio>
#include <optional>
#include <string>

#include "utils/string_array.h"

#include <glib.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Gets directory part of filename.
 *
 * @param filename Filename containing directory.
 */
std::string
io_get_dir (const std::string &filename);

/**
 * @brief Makes directory with parents if doesn't exist.
 *
 * @throw ZrythmException Failed to make directory with parents.
 */
void
io_mkdir (const std::string &dir);

/**
 * @brief Touches a file similar to UNIX touch.
 *
 * @return Whether successful.
 */
bool
io_touch_file (const std::string &file_path);

#if 0
ATTR_NONNULL char *
io_path_get_parent_dir (const char * path);
#endif

/**
 * Strips extensions from given filename.
 */
std::string
io_file_strip_ext (const std::string &filename);

/**
 * Returns file extension or NULL.
 */
ATTR_NONNULL const char *
io_file_get_ext (const char * file);

#define io_path_get_basename(filename) g_path_get_basename (filename)

/**
 * Strips path from given filename.
 */
std::string
io_path_get_basename_without_ext (const std::string &filename);

ATTR_NONNULL char *
io_file_get_creation_datetime (const char * filename);

/**
 * Returns the number of seconds since the epoch, or
 * -1 if failed.
 */
ATTR_NONNULL gint64
io_file_get_last_modified_datetime (const char * filename);

ATTR_NONNULL std::string
             io_file_get_last_modified_datetime_as_str (const char * filename);

/**
 * Removes the given file.
 *
 * @param path The path to the file to remove.
 *
 * @throw ZrythmException If an error occurred while attempting to remove @ref
 * path.
 *
 * @return True if the file was deleted, false if it didn't exist.
 */
bool
io_remove (const std::string &path);

/**
 * Removes a dir, optionally forcing deletion.
 *
 * For safety reasons, this only accepts an absolute path with length greater
 * than 20 if forced.
 */
int
io_rmdir (const std::string &path, bool force);

/**
 * @brief
 *
 * @param _dir
 * @throw ZrythmException If @ref dir cannot be opened.
 * @return StringArray
 */
StringArray
io_get_files_in_dir_as_basenames (const std::string &_dir);

/**
 * Returns a list of the files in the given directory as absolute paths.
 *
 * @param dir The directory to look for.
 *
 * @throw ZrythmException If @ref dir cannot be opened.
 *
 * @return a StringArray.
 */
StringArray
io_get_files_in_dir_ending_in (
  const std::string                &_dir,
  bool                              recursive,
  const std::optional<std::string> &end_string);

std::string
io_create_tmp_dir (const std::string template_name = "zrythm_generic_XXXXXX");

/**
 * Returns a list of the files in the given directory.
 *
 * @see io_get_files_in_dir_ending_in().
 * @throw ZrythmException If @ref dir cannot be opened.
 */
StringArray
io_get_files_in_dir (const std::string &_dir);

/**
 * Copies a directory.
 *
 * @note This will not work if @p destdir_str has a file with the same filename
 * as a directory in @p srcdir_str.
 *
 * @see
 * https://stackoverflow.com/questions/16453739/how-do-i-recursively-copy-a-directory-using-vala
 *
 * @throw ZrythmException on error.
 */
void
io_copy_dir (
  const std::string_view destdir_str,
  const std::string_view srcdir_str,
  bool                   follow_symlinks,
  bool                   recursive);

/**
 * Returns a newly allocated path that is either
 * a copy of the original path if the path does
 * not exist, or the original path appended with
 * (n), where n is a number.
 *
 * Example: "myfile" -> "myfile (1)"
 */
std::string
io_get_next_available_filepath (const std::string &filepath);

/**
 * Opens the given directory using the default
 * program.
 */
ATTR_NONNULL void
io_open_directory (const char * path);

/**
 * Returns the given file name with any illegal characters removed.
 *
 * @note This will remove path separators like slashes.
 *
 * @see io_get_legal_path_name().
 */
std::string
io_get_legal_file_name (const std::string &file_name);

/**
 * Returns the given path with any illegal characters removed.
 */
std::string
io_get_legal_path_name (const std::string &path);

/**
 * @brief Writes the data to @p file_path atomically.
 *
 * @param file_path
 * @param data
 * @throw ZrythmException On error.
 */
void
io_write_file_atomic (const std::string &file_path, const std::string &data);

#ifdef _WIN32
char *
io_get_registry_string_val (const char * path);
#endif

#if defined(__APPLE__) && defined(INSTALLER_VER)
/**
 * Gets the bundle path on MacOS.
 *
 * @return Non-zero on fail.
 */
ATTR_NONNULL int
io_get_bundle_path (char * bundle_path);
#endif

/**
 * Returns the new path after traversing any symlinks (using
 * readlink()).
 */
ATTR_NONNULL char *
io_traverse_path (const char * abs_path);

/**
 * @}
 */

#endif
