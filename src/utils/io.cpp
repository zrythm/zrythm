// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 *  Copyright 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: 2000 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "utils/exceptions.h"
#include "utils/logger.h"
#ifdef __linux__
#  include <sys/ioctl.h>
#  include <sys/stat.h>
#  include <sys/types.h>

#  include <fcntl.h>
#  include <linux/fs.h>
#endif

#ifdef _WIN32
#  include <windows.h>
#endif

#include "utils/datetime.h"
#include "utils/io.h"
#include "utils/string.h"

#include <QUrl>

#include "juce_wrapper.h"
#include <fmt/format.h>

#if defined(__APPLE__) && ZRYTHM_IS_INSTALLER_VER
#  include "CoreFoundation/CoreFoundation.h"
#  include <libgen.h>
#endif

namespace zrythm::utils::io
{

utils::Utf8String
get_path_separator_string ()
{
  return utils::Utf8String::from_utf8_encoded_string (std::string{
    QDir::listSeparator ().toLatin1 () });
}

fs::path
get_home_path ()
{
  return Utf8String::from_qstring (QDir::homePath ()).to_path ();
}

fs::path
get_temp_path ()
{
  return Utf8String::from_qstring (QDir::tempPath ()).to_path ();
}

fs::path
get_dir (const fs::path &filename)
{
  return Utf8String::from_qstring (
           QFileInfo (Utf8String::from_path (filename).to_qstring ())
             .absolutePath ())
    .to_path ();
}

fs::path
uri_to_file (const utils::Utf8String &uri)
{
  auto url = QUrl (uri.to_qstring ());
  if (!url.isValid ())
    {
      throw ZrythmException (fmt::format ("Failed to parse URI '{}'", uri));
    }
  return utils::Utf8String::from_qstring (url.toLocalFile ()).to_path ();
}

void
mkdir (const fs::path &dir)
{
  // this is called during logger instantiation so check if logger exists
  if (LoggerProvider::has_logger ())
    {
      z_debug ("Creating directory: {}", dir);
    }
  std::error_code ec;
  if (!fs::create_directories (dir, ec))
    {
      if (ec.value () == 0) // Directory already exists
        {
          return;
        }
      throw ZrythmException (fmt::format (
        "Failed to make directory {} with parents: {}", dir, ec.message ()));
    }
}

fs::path
file_get_ext (const fs::path &filename)
{
  if (filename.has_extension ())
    {
      auto ret = filename.extension ();
      // remove the dot
      return { ret.string ().substr (1) };
    }
  return "";
}

bool
touch_file (const fs::path &file_path)
{
  juce::File file (utils::Utf8String::from_path (file_path).to_juce_file ());
  if (file.exists ())
    {
      // Update the file's modification time to the current time
      return file.setLastModificationTime (juce::Time::getCurrentTime ());
    }
  else
    {
      // Create a new empty file
      return file.replaceWithData (nullptr, 0);
    }
}

fs::path
file_strip_ext (const fs::path &filename)
{
  return filename.parent_path () / filename.stem ();
}
fs::path
path_get_basename (const fs::path &filename)
{
  return filename.filename ();
}

fs::path
path_get_basename_without_ext (const fs::path &filename)
{
  return filename.stem ();
}

qint64
file_get_last_modified_datetime (const fs::path &filename)
{
  juce::File file (utils::Utf8String::from_path (filename).to_juce_file ());
  if (file.exists ())
    {
      return file.getLastModificationTime ().toMilliseconds () / 1000;
    }
  z_info ("Failed to get last modified for {}", filename);
  return -1;
}

Utf8String
file_get_last_modified_datetime_as_str (const fs::path &filename)
{
  qint64 secs = file_get_last_modified_datetime (filename);
  if (secs == -1)
    return {};

  return datetime::epoch_to_str (secs, u8"%Y-%m-%d %H:%M:%S");
}

bool
remove (const fs::path &path)
{
  z_debug ("Removing {}...", path.string ());

  auto file = utils::Utf8String::from_path (path).to_juce_file ();
  if (!file.exists ())
    return false;

  if (file.isDirectory ())
    throw ZrythmException (fmt::format ("Cannot remove directory {}", path));

  bool success = file.deleteFile ();
  if (success)
    return true;

  throw ZrythmException (fmt::format ("Failed to remove {}", path));
}

bool
rmdir (const fs::path &path, bool force)
{

  if (!path.is_absolute () || !fs::exists (path) || !fs::is_directory (path))
    {
      z_warning ("refusing to remove {}", path);
      return false;
    }
  z_info ("Removing {}{}", path, force ? " recursively" : "");

  if (force)
    {
      const auto path_str = Utf8String::from_path (path);
      z_return_val_if_fail (path_str.str ().length () > 20, false);
      juce::File dir (path_str.to_juce_string ());
      return dir.deleteRecursively ();
    }

  std::error_code ec;
  std::filesystem::remove (path, ec);
  return !ec;
}

/**
 * Appends files to the given array from the given dir if they end in the given
 * string.
 *
 * @param opt_end_string If empty, appends all files.
 */
static void
append_files_from_dir_ending_in (
  std::vector<fs::path>                  &files,
  bool                                    recursive,
  const fs::path                         &_dir,
  const std::optional<utils::Utf8String> &opt_end_string)
{
  juce::File directory (_dir.string ());

  if (!directory.isDirectory ())
    {
      throw ZrythmException (fmt::format (
        "'{}' is not a directory (or doesn't exist)", _dir.string ()));
    }

  for (
    const auto &file : juce::RangedDirectoryIterator (
      directory, recursive,
      opt_end_string
        ? utils::Utf8String (u8"*").to_juce_string ()
            + (*opt_end_string).to_juce_string ()
        : u8"*",
      juce::File::findFiles))
    {
      files.push_back (
        Utf8String ::from_juce_string (file.getFile ().getFullPathName ())
          .to_path ());
    }
}

std::vector<fs::path>
get_files_in_dir (const fs::path &_dir)
{
  return get_files_in_dir_ending_in (_dir, false, std::nullopt);
}

void
copy_dir (
  const fs::path &destdir,
  const fs::path &srcdir,
  bool            follow_symlinks,
  bool            recursive)
{
  z_debug (
    "attempting to copy dir '{}' to '{}' (recursive: {})", srcdir, destdir,
    recursive);

  QDir src_dir (Utf8String::from_path (srcdir).to_qstring ());
  QDir dest_dir (Utf8String::from_path (destdir).to_qstring ());

  if (!src_dir.exists ())
    {
      throw ZrythmException (
        fmt::format ("Failed opening directory '{}'", srcdir));
    }

  if (!dest_dir.exists ())
    {
      mkdir (destdir);
    }

  const auto entries =
    src_dir.entryInfoList (QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

  for (const auto &entry : entries)
    {
      const auto dest_path = dest_dir.filePath (entry.fileName ());
      const auto src_path = entry.filePath ();

      /* recurse if necessary */
      if (entry.isDir ())
        {
          if (recursive)
            {
              copy_dir (
                Utf8String::from_qstring (dest_path).to_path (),
                Utf8String::from_qstring (src_path).to_path (), follow_symlinks,
                recursive);
            }
        }
      /* otherwise if not dir, copy file */
      else
        {
          // not following symlinks unimplemented
          z_return_if_fail (!follow_symlinks);

          if (!QFile::copy (src_path, dest_path))
            {
              throw ZrythmException (fmt::format (
                "Failed copying file {} to {}", src_path, dest_path));
            }
        }
    }
}

void
copy_file (const fs::path &destfile, const fs::path &srcfile)
{
  auto src_file = QFile (srcfile);
  if (!src_file.copy (destfile))
    {
      throw ZrythmException (fmt::format (
        "Failed to copy '{}' to '{}': {}", Utf8String::from_path (srcfile),
        Utf8String::from_path (destfile), src_file.errorString ()));
    }
}

std::vector<fs::path>
get_files_in_dir_as_basenames (const fs::path &_dir)
{
  return get_files_in_dir (_dir)
         | std::views::transform ([] (const fs::path &path) {
             return path_get_basename (path);
           })
         | std::ranges::to<std::vector> ();
}

std::vector<fs::path>
get_files_in_dir_ending_in (
  const fs::path                         &_dir,
  bool                                    recursive,
  const std::optional<utils::Utf8String> &end_string)
{
  std::vector<fs::path> arr;
  append_files_from_dir_ending_in (arr, recursive, _dir, end_string);
  return arr;
}

fs::path
get_next_available_filepath (const fs::path &filepath)
{
  int  i = 1;
  auto file_without_ext = file_strip_ext (filepath);
  auto file_ext = file_get_ext (filepath);
  auto new_path = filepath;
  while (path_exists (new_path))
    {
      if (fs::is_directory (new_path))
        {
          new_path = fs::path (fmt::format ("{} ({})", filepath, i++));
        }
      else
        {
          new_path = fs::path (
            fmt::format ("{} ({}).{}", file_without_ext, i++, file_ext));
        }
    }

  return new_path;
}

std::unique_ptr<QTemporaryDir>
make_tmp_dir (std::optional<QString> template_str, bool in_temp_dir)
{
  std::unique_ptr<QTemporaryDir> ret;
  if (template_str)
    {
      QString path =
        in_temp_dir ? QDir::tempPath () + QDir::separator () : QString ();
      ret = std::make_unique<QTemporaryDir> (path + *template_str);
    }
  else
    {
      ret = std::make_unique<QTemporaryDir> ();
    }
  if (!ret->isValid ())
    {
      throw ZrythmException ("Failed to create temporary directory");
    }
  return ret;
}

std::unique_ptr<QTemporaryFile>
make_tmp_file (std::optional<utils::Utf8String> template_path, bool in_temp_dir)
{
  std::unique_ptr<QTemporaryFile> ret;
  if (template_path)
    {
      QString path =
        in_temp_dir ? QDir::tempPath () + QDir::separator () : QString ();
      ret = std::make_unique<QTemporaryFile> (
        path + (*template_path).to_qstring ());
    }
  else
    {
      ret = std::make_unique<QTemporaryFile> ();
    }
  if (!ret->open ())
    {
      throw ZrythmException ("Failed to create temporary file");
    }
  return ret;
}

Utf8String
get_legal_file_name (const Utf8String &file_name)
{
  return Utf8String::from_juce_string (
    juce::File::createLegalFileName (file_name.to_juce_string ()));
}

Utf8String
get_legal_path_name (const Utf8String &path)
{
  return Utf8String::from_juce_string (
    juce::File::createLegalPathName (path.to_juce_string ()));
}

#ifdef _WIN32
std::string
get_registry_string_val (const std::string &key)
{
  auto full_path = fmt::format (
    "HKEY_LOCAL_MACHINE\\Software\\{}\\{}\\Settings\\{}", PROGRAM_NAME,
    PROGRAM_NAME, key);
  auto value = juce::WindowsRegistry::getValue (juce::String (full_path));

  if (!value.isEmpty ())
    {
      z_info ("reg value: {}", value);
      return juce_string_to_std_string (value);
    }

  z_warning ("reg value not found: {}", full_path);
  return "";
}
#endif

#if defined(__APPLE__) && ZRYTHM_IS_INSTALLER_VER
/**
 * Gets the bundle path on MacOS.
 *
 * @return Non-zero on fail.
 */
int
get_bundle_path (char * bundle_path)
{
  CFBundleRef bundle = CFBundleGetMainBundle ();
  CFURLRef    bundleURL = CFBundleCopyBundleURL (bundle);
  Boolean     success = CFURLGetFileSystemRepresentation (
    bundleURL, TRUE, (UInt8 *) bundle_path, PATH_MAX);
  z_return_val_if_fail (success, -1);
  CFRelease (bundleURL);
  z_info ("bundle path: {}", bundle_path);

  return 0;
}
#endif

bool
path_exists (const fs::path &path)
{
  return fs::exists (path);
}

bool
reflink_file (const fs::path &dest, const fs::path &src)
{
#ifdef __linux__
  int src_fd = open (src.c_str (), O_RDONLY);
  if (src_fd == -1)
    {
      z_warning ("Failed to open source file {}", src);
      return false;
    }
  int dest_fd = open (dest.c_str (), O_RDWR | O_CREAT, 0644);
  if (dest_fd == -1)
    {
      z_warning ("Failed to open destination file {}", dest);
      return false;
    }
  if (ioctl (dest_fd, FICLONE, src_fd) != 0)
    {
      z_warning ("Failed to reflink '{}' to '{}'", src, dest);
      return false;
    }
  return true;
#else
  z_warning ("Reflink not supported on this platform");
  return false;
#endif
}

bool
is_file_hidden (const fs::path &file)
{
  return QFileInfo (file).isHidden ();
}

QByteArray
read_file_contents (const fs::path &path)
{
  QFile file (path);
  if (file.open (QIODevice::ReadOnly))
    {
      return file.readAll ();
    }
  throw ZrythmException (fmt::format (
    "Failed to open file for reading: '{}' ({})", path.string (),
    file.errorString ()));
}

void
set_file_contents (const fs::path &path, const char * contents, size_t size)
{
  QFile file (path);
  if (file.open (QIODevice::WriteOnly))
    {
      file.write (contents, static_cast<qint64> (size));
    }
  else
    {
      throw ZrythmException (fmt::format (
        "Failed to open file for writing: '{}' ({})", path.string (),
        file.errorString ()));
    }
}

void
set_file_contents (const fs::path &file_path, const Utf8String &data)
{
  std::ofstream file (file_path);
  if (!file.is_open ())
    {
      throw ZrythmException (
        fmt::format ("Failed to open file: {}", file_path.string ()));
    }
  file << data;
}

QStringList
split_paths (const QString &paths)
{
  return paths.split (QDir::listSeparator ());
}

}; // namespace zrythm::utils::io
