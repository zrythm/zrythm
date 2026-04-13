// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <filesystem>
#include <optional>

#include "utils/utf8_string.h"

#include <QTemporaryDir>
#include <QTemporaryFile>

namespace zrythm::utils::io
{
namespace fs = std::filesystem;

/**
 * @brief Get the path list separator as a string (":" or ";" on Windows).
 */
utils::Utf8String
get_path_separator_string ();

/**
 * @brief Get the path to the user's home directory.
 */
std::filesystem::path
get_home_path ();

std::filesystem::path
get_temp_path ();

/**
 * @brief Get the directory of the given file path.
 *
 * This function takes a file path as input and returns the directory
 * portion of the path.
 *
 * If the file path is a relative path, the directory portion will be
 * relative to the current working directory.
 *
 * @param filename The file path to get the directory for.
 * @return The directory portion of the file path.
 */
std::filesystem::path
get_dir (const std::filesystem::path &filename);

/**
 * @brief Makes directory with parents if doesn't exist.
 *
 * @throw ZrythmException Failed to make directory with parents.
 */
void
mkdir (const std::filesystem::path &dir);

/**
 * @brief Touches a file similar to UNIX touch.
 *
 * @return Whether successful.
 */
bool
touch_file (const std::filesystem::path &file_path);

std::filesystem::path
uri_to_file (const utils::Utf8String &uri);

/**
 * Strips extensions from given filename.
 */
std::filesystem::path
file_strip_ext (const std::filesystem::path &filename);

/**
 * Returns file extension or empty string if none.
 */
std::filesystem::path
file_get_ext (const std::filesystem::path &file);

/**
 * @brief Get the base name of a file path with the file extension.
 *
 * This function takes a file path as input and returns the base name of the
 * file with the file extension. For example, if the input is
 * "path/to/file.tar.gz", the output will be "file.tar.gz".
 *
 * @param filename The file path to get the base name for.
 * @return The base name of the file with the extension.
 */
std::filesystem::path
path_get_basename (const std::filesystem::path &filename);

/**
 * @brief Returns the base name of the given file path, without the last file
 * extension.
 *
 * This function takes a file path as input and returns the base name of the
 * file, excluding the last file extension (if any). For example, if the input
 * is "path/to/file.tar.gz", the function will return "file.tar".
 *
 * @param filename The file path to extract the base name from.
 * @return The base name of the file, without the last file extension.
 */
std::filesystem::path
path_get_basename_without_ext (const std::filesystem::path &filename);

/**
 * Returns the number of seconds since the epoch, or
 * -1 if failed.
 */
qint64
file_get_last_modified_datetime (const std::filesystem::path &filename);

Utf8String
file_get_last_modified_datetime_as_str (const std::filesystem::path &filename);

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
remove (const std::filesystem::path &path);

/**
 * @brief Creates a temporary directory in the system's default temp directory.
 *
 * This function creates a new temporary directory using QTemporaryDir. If the
 * directory cannot be created, a ZrythmException is thrown.
 *
 * The directory will be auto-deleted when the QTemporaryDir object is
 * destroyed.
 *
 * Use path() to get the path of the temporary directory.
 *
 * @param template_path An optional template string ending in XXXXXX to use for
 * the temporary directory.
 * @param in_temp_dir Whether to create the directory in the system's default
 * temp directory. If false, @p template_path must be absolute.
 * @return The QTemporaryDir object.
 * @throw ZrythmException if the temporary directory cannot be created.
 */
std::unique_ptr<QTemporaryDir>
make_tmp_dir (
  std::optional<QString> template_path = std::nullopt,
  bool                   in_temp_dir = true);

/**
 * @see make_tmp_dir
 */
inline std::unique_ptr<QTemporaryDir>
make_tmp_dir_at_path (const std::filesystem::path &absolute_template_path)
{
  return make_tmp_dir (
    Utf8String::from_path (absolute_template_path).to_qstring (), false);
}

/**
 * @brief
 * @see make_tmp_dir
 *
 * @param template_path
 * @param in_temp_dir
 * @return std::unique_ptr<QTemporaryFile>
 * @throw ZrythmException If the temporary directory cannot be created.
 */
std::unique_ptr<QTemporaryFile>
make_tmp_file (
  std::optional<utils::Utf8String> template_path = std::nullopt,
  bool                             in_temp_dir = true);

/**
 * Removes a dir, optionally forcing deletion.
 *
 * For safety reasons, this only accepts an absolute path with length greater
 * than 20 if forced.
 *
 * @return Whether successful.
 */
bool
rmdir (const std::filesystem::path &path, bool force);

/**
 * @brief
 *
 * @param _dir
 * @throw ZrythmException If @ref dir cannot be opened.
 */
std::vector<std::filesystem::path>
get_files_in_dir_as_basenames (const std::filesystem::path &_dir);

/**
 * Returns a list of the files in the given directory as absolute paths.
 *
 * @param dir The directory to look for.
 *
 * @throw ZrythmException If @ref dir cannot be opened.
 */
std::vector<std::filesystem::path>
get_files_in_dir_ending_in (
  const std::filesystem::path            &_dir,
  bool                                    recursive,
  const std::optional<utils::Utf8String> &end_string);

/**

* Returns a list of the files in the given directory.
 *
 * @see io_get_files_in_dir_ending_in().
 * @throw ZrythmException If @ref dir cannot be opened.
 */
std::vector<std::filesystem::path>
get_files_in_dir (const std::filesystem::path &_dir);

/**
 * Copies a directory.
 *
 * @note This will not work if @p destdir has a file with the same filename
 * as a directory in @p srcdir.
 *
 * @see
 * https://stackoverflow.com/questions/16453739/how-do-i-recursively-copy-a-directory-using-vala
 *
 * @throw ZrythmException on error.
 */
void
copy_dir (
  const std::filesystem::path &destdir,
  const std::filesystem::path &srcdir,
  bool                         follow_symlinks,
  bool                         recursive);

/**
 * @brief Copies a file from a source path to a destination path.
 *
 * This function copies the contents of the file at the `srcfile` path to the
 * file at the `destfile` path. If the copy operation fails, a `ZrythmException`
 * is thrown with a detailed error message.
 *
 * @param destfile The destination file path.
 * @param srcfile The source file path.
 * @throw ZrythmException If the file copy operation fails.
 */
void
copy_file (
  const std::filesystem::path &destfile,
  const std::filesystem::path &srcfile);

/**
 * @brief Moves (renames) a file from a source path to a destination path.
 *
 * @param destfile The destination file path.
 * @param srcfile The source file path.
 * @param force If true, overwrites the destination file if it exists.
 * @throw ZrythmException If the file rename operation fails.
 */
void
move_file (
  const std::filesystem::path &destfile,
  const std::filesystem::path &srcfile,
  bool                         force = false);

/**
 * Returns a newly allocated path that is either
 * a copy of the original path if the path does
 * not exist, or the original path appended with
 * (n), where n is a number.
 *
 * Example: "myfile" -> "myfile (1)"
 */
std::filesystem::path
get_next_available_filepath (const std::filesystem::path &filepath);

/**
 * Returns the given file name with any illegal characters removed.
 *
 * @note This will remove path separators like slashes.
 *
 * @see io_get_legal_path_name().
 */
Utf8String
get_legal_file_name (const Utf8String &file_name);

/**
 * Returns the given path with any illegal characters removed.
 */
Utf8String
get_legal_path_name (const Utf8String &path);

#ifdef _WIN32
/**
 * @brief Returns the registry value for the given key in Zrythm's registry path.
 */
utils::Utf8String
get_registry_string_val (const utils::Utf8String &key);
#endif

#if defined(__APPLE__) && ZRYTHM_IS_INSTALLER_VER
/**
 * Gets the bundle path on MacOS.
 *
 * @return Non-zero on fail.
 */
[[gnu::nonnull]] int
get_bundle_path (char * bundle_path);
#endif

/**
 * Returns whether the file/dir exists.
 */
bool
path_exists (const std::filesystem::path &path);

bool
is_file_hidden (const std::filesystem::path &file);

/**
 * Do cp --reflink from @ref src to @ref dest.
 *
 * @return Whether successful.
 */
[[nodiscard]] bool
reflink_file (
  const std::filesystem::path &dest,
  const std::filesystem::path &src);

/**
 * @brief Reads the contents of a file into a QByteArray.
 *
 * @param path
 * @return QByteArray
 * @throw ZrythmException on error.
 */
[[nodiscard]] QByteArray
read_file_contents (const std::filesystem::path &path);

/**
 * @brief Write arbitrary data to a file atomically (all at once).
 *
 * @param path
 * @param contents
 * @param size
 */
void
set_file_contents (
  const std::filesystem::path &path,
  const char *                 contents,
  size_t                       size);

/**
 * @brief Writes the string to the file.
 *
 * @param file_path
 * @param data
 * @throw ZrythmException On error.
 */
void
set_file_contents (
  const std::filesystem::path &file_path,
  const utils::Utf8String     &data);

/**
 * @brief Splits a string of paths separated by the system's list separator
 * character.
 *
 * This function takes a string of paths separated by the system's list
 * separator character (e.g. ';' on Windows, ':' on Unix-like systems) and
 * splits it into a
 */
QStringList
split_paths (const QString &paths);

}; // namespace zrythm::utils::io
