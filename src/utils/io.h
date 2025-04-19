// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include "zrythm-config.h"

#include <optional>

#include "utils/string_array.h"

#include <QTemporaryDir>
#include <QTemporaryFile>

#include "./types.h"

namespace zrythm::utils::io
{

/**
 * @brief Get the path list separator as a string (":" or ";" on Windows).
 */
std::string
get_path_separator_string ();

/**
 * @brief Get the path to the user's home directory.
 */
fs::path
get_home_path ();

fs::path
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
std::string
get_dir (const std::string &filename);

/**
 * @brief Makes directory with parents if doesn't exist.
 *
 * @throw ZrythmException Failed to make directory with parents.
 */
void
mkdir (const std::string &dir);

/**
 * @brief Touches a file similar to UNIX touch.
 *
 * @return Whether successful.
 */
bool
touch_file (const std::string &file_path);

fs::path
uri_to_file (const std::string &uri);

/**
 * Strips extensions from given filename.
 */
std::string
file_strip_ext (const std::string &filename);

/**
 * Returns file extension or empty string if none.
 */
std::string
file_get_ext (const std::string &file);

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
std::string
path_get_basename (const std::string &filename);

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
std::string
path_get_basename_without_ext (const std::string &filename);

/**
 * Returns the number of seconds since the epoch, or
 * -1 if failed.
 */
qint64
file_get_last_modified_datetime (const std::string &filename);

std::string
file_get_last_modified_datetime_as_str (const std::string &filename);

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
remove (const std::string &path);

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
make_tmp_dir_at_path (const fs::path &absolute_template_path)
{
  return make_tmp_dir (
    QString::fromStdString (absolute_template_path.string ()), false);
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
  std::optional<std::string> template_path = std::nullopt,
  bool                       in_temp_dir = true);

/**
 * Removes a dir, optionally forcing deletion.
 *
 * For safety reasons, this only accepts an absolute path with length greater
 * than 20 if forced.
 *
 * @return Whether successful.
 */
bool
rmdir (const fs::path &path, bool force);

/**
 * @brief
 *
 * @param _dir
 * @throw ZrythmException If @ref dir cannot be opened.
 * @return StringArray
 */
StringArray
get_files_in_dir_as_basenames (const std::string &_dir);

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
get_files_in_dir_ending_in (
  const std::string                &_dir,
  bool                              recursive,
  const std::optional<std::string> &end_string);

/**
 * Returns a list of the files in the given directory.
 *
 * @see io_get_files_in_dir_ending_in().
 * @throw ZrythmException If @ref dir cannot be opened.
 */
StringArray
get_files_in_dir (const std::string &_dir);

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
  const fs::path &destdir,
  const fs::path &srcdir,
  bool            follow_symlinks,
  bool            recursive);

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
copy_file (const fs::path &destfile, const fs::path &srcfile);

/**
 * Returns a newly allocated path that is either
 * a copy of the original path if the path does
 * not exist, or the original path appended with
 * (n), where n is a number.
 *
 * Example: "myfile" -> "myfile (1)"
 */
std::string
get_next_available_filepath (const std::string &filepath);

/**
 * Returns the given file name with any illegal characters removed.
 *
 * @note This will remove path separators like slashes.
 *
 * @see io_get_legal_path_name().
 */
std::string
get_legal_file_name (const std::string &file_name);

/**
 * Returns the given path with any illegal characters removed.
 */
std::string
get_legal_path_name (const std::string &path);

#ifdef _WIN32
/**
 * @brief Returns the registry value for the given key in Zrythm's registry path.
 *
 * @param key
 * @return std::string
 */
std::string
get_registry_string_val (const std::string &key);
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
path_exists (const fs::path &path);

bool
is_file_hidden (const fs::path &file);

/**
 * Do cp --reflink from @ref src to @ref dest.
 *
 * @return Whether successful.
 */
[[nodiscard]] bool
reflink_file (const std::string &dest, const std::string &src);

/**
 * @brief Reads the contents of a file into a QByteArray.
 *
 * @param path
 * @return QByteArray
 * @throw ZrythmException on error.
 */
QByteArray
read_file_contents (const fs::path &path);

void
set_file_contents (const fs::path &path, const char * contents, size_t size);

/**
 * @brief Writes the data to @p file_path atomically (all at once).
 *
 * @param file_path
 * @param data
 * @throw ZrythmException On error.
 */
void
set_file_contents (const fs::path &file_path, const std::string &data);

QStringList
split_paths (const QString &paths);

}; // namespace zrythm::utils::io

#endif
