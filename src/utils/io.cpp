// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
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
 * ---
 */

#include "zrythm-config.h"

#define _XOPEN_SOURCE 500
#include <stdio.h>

#include <ftw.h>
#include <limits.h>

#ifdef _WOE32
#  include <windows.h>
#endif

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils/datetime.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/system.h"
#include "zrythm.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include <time.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(INSTALLER_VER)
#  include "CoreFoundation/CoreFoundation.h"
#  include <libgen.h>
#endif

typedef enum
{
  Z_UTILS_IO_ERROR_FAILED,
} ZUtilsIOError;

#define Z_UTILS_IO_ERROR z_utils_io_error_quark ()
GQuark
z_utils_io_error_quark (void);
G_DEFINE_QUARK (z - utils - io - error - quark, z_utils_io_error)

/**
 * Gets directory part of filename. MUST be freed.
 *
 * @param filename Filename containing directory.
 */
char *
io_get_dir (const char * filename)
{
  return g_path_get_dirname (filename);
}

/**
 * Makes directory if doesn't exist.
 *
 * @return True if the directory exists or was successfully
 *   created, false if error was occurred and errno is set.
 */
bool
io_mkdir (const char * dir, GError ** error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, false);

  g_message ("Creating directory: %s", dir);
  struct stat st = {};
  if (stat (dir, &st) == -1)
    {
      int ret = g_mkdir_with_parents (dir, 0700);
      if (ret == 0)
        {
          return true;
        }
      else
        {
          g_set_error (
            error, Z_UTILS_IO_ERROR, Z_UTILS_IO_ERROR_FAILED,
            "Failed to make directory with parents: %s", strerror (errno));
          return false;
        }
    }
  return true;
}

/**
 * Returns file extension or NULL.
 */
const char *
io_file_get_ext (const char * filename)
{
  const char * dot = strrchr (filename, '.');
  if (!dot || dot == filename)
    return "";

  return dot + 1;
}

/**
 * Creates the file if doesn't exist
 */
FILE *
io_create_file (const char * filename)
{
  return fopen (filename, "ab+");
}

/**
 * Touches a file similar to UNIX touch.
 */
void
io_touch_file (const char * filename)
{
  FILE * f = io_create_file (filename);
  if (!f)
    {
      g_warning ("failed to create file: %s", filename);
      return;
    }
  fclose (f);
}

/**
 * Strips extensions from given filename.
 *
 * MUST be freed.
 */
char *
io_file_strip_ext (const char * filename)
{
  /* if last char is a dot, return the string
   * without the dot */
  size_t len = strlen (filename);
  if (filename[len - 1] == '.')
    {
      return g_strndup (filename, len - 1);
    }

  const char * dot = io_file_get_ext (filename);

  /* if no dot, return filename */
  if (string_is_equal (dot, ""))
    {
      return g_strdup (filename);
    }

  long   size = (dot - filename) - 1;
  char * new_str = g_strndup (filename, (gsize) size);

  return new_str;
}

/**
 * Strips path from given filename.
 *
 * MUST be freed.
 */
char *
io_path_get_basename_without_ext (const char * filename)
{
  char * basename = g_path_get_basename (filename);
  char * no_ext = io_file_strip_ext (basename);
  g_free (basename);

  return no_ext;
}

char *
io_path_get_parent_dir (const char * path)
{
#ifdef _WOE32
#  define PATH_SEP "\\\\"
#  define ROOT_REGEX "[A-Z]:" PATH_SEP
#else
#  define PATH_SEP "/"
#  define ROOT_REGEX "/"
#endif
  char   regex[] = "(" ROOT_REGEX ".*)" PATH_SEP "[^" PATH_SEP "]+";
  char * parent = string_get_regex_group (path, regex, 1);
#if 0
  g_message ("[%s]\npath: %s\nregex: %s\nparent: %s",
    __func__, path, regex, parent);
#endif

  if (!parent)
    {
      strcpy (regex, "(" ROOT_REGEX ")[^" PATH_SEP "]*");
      parent = string_get_regex_group (path, regex, 1);
#if 0
      g_message ("path: %s\nregex: %s\nparent: %s",
        path, regex, parent);
#endif
    }

#undef PATH_SEP
#undef ROOT_REGEX

  return parent;
}

char *
io_file_get_creation_datetime (const char * filename)
{
  /* TODO */
  return NULL;
}

/**
 * Returns the number of seconds since the epoch, or
 * -1 if failed.
 */
gint64
io_file_get_last_modified_datetime (const char * filename)
{
  struct stat result;
  if (stat (filename, &result) == 0)
    {
      return result.st_mtime;
    }
  g_message ("Failed to get last modified for %s", filename);
  return -1;
}

char *
io_file_get_last_modified_datetime_as_str (const char * filename)
{
  gint64 secs = io_file_get_last_modified_datetime (filename);
  if (secs == -1)
    return NULL;

  return datetime_epoch_to_str (secs, "%Y-%m-%d %H:%M:%S");
}

/**
 * Removes the given file.
 */
int
io_remove (const char * path)
{
  if (ZRYTHM)
    {
      g_message ("Removing %s...", path);
    }
  return g_remove (path);
}

static int
unlink_cb (
  const char *        fpath,
  const struct stat * sb,
  int                 typeflag,
  struct FTW *        ftwbuf)
{
  int rv = remove (fpath);

  if (rv)
    perror (fpath);

  return rv;
}

/**
 * Removes a dir, optionally forcing deletion.
 *
 * For safety reasons, this only accepts an
 * absolute path with length greater than 20 if
 * forced.
 */
int
io_rmdir (const char * path, bool force)
{
  if (force)
    {
      g_return_val_if_fail (g_path_is_absolute (path) && strlen (path) > 20, -1);
      nftw (path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    }

  g_message ("Removing %s", path);
  return g_rmdir (path);
}

/**
 * Appends files to the given array from the given
 * dir if they end in the given string.
 *
 * @param end_string If empty, appends all files.
 */
static void
append_files_from_dir_ending_in (
  char ***     files,
  int *        num_files,
  const int    recursive,
  const char * _dir,
  const char * end_string)
{
  GDir *        dir;
  GError *      error = NULL;
  const gchar * filename;
  char *        full_path;

  dir = g_dir_open (_dir, 0, &error);
  if (error)
    {
      if (ZRYTHM_TESTING)
        {
          /* this is needed because
           * g_test_expect_message() doesn't work
           * with below */
          g_warn_if_reached ();
        }
      else
        {
          g_warning ("Failed opening directory %s: %s", _dir, error->message);
        }
      return;
    }

  while ((filename = g_dir_read_name (dir)))
    {
      full_path = g_build_filename (_dir, filename, NULL);

      /* recurse if necessary */
      if (recursive && g_file_test (full_path, G_FILE_TEST_IS_DIR))
        {
          append_files_from_dir_ending_in (
            files, num_files, recursive, full_path, end_string);
        }

      if (
        !end_string || (end_string && g_str_has_suffix (full_path, end_string)))
        {
          *files = static_cast<char **> (
            g_realloc (*files, sizeof (char *) * (size_t) (*num_files + 2)));
          (*files)[(*num_files)] = g_strdup (full_path);
          (*num_files)++;
        }

      g_free (full_path);
    }
  g_dir_close (dir);

  /* NULL terminate */
  (*files)[*num_files] = NULL;
}

bool
io_copy_dir (
  const char * destdir_str,
  const char * srcdir_str,
  bool         follow_symlinks,
  bool         recursive,
  GError **    error)
{

  g_debug (
    "attempting to copy dir '%s' to '%s' "
    "(recursive: %d)",
    srcdir_str, destdir_str, recursive);

  GError * err = NULL;
  GDir *   srcdir = g_dir_open (srcdir_str, 0, &err);
  if (!srcdir)
    {
      if (ZRYTHM_TESTING)
        {
          /* this is needed because
           * g_test_expect_message() doesn't work
           * with below */
          g_warn_if_reached ();
        }
      else
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "Failed opening directory %s", srcdir_str);
        }
      return false;
    }

  bool success = io_mkdir (destdir_str, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Failed creating directory %s", destdir_str);
      return false;
    }

  const gchar * filename;
  while ((filename = g_dir_read_name (srcdir)))
    {
      char * src_full_path = g_build_filename (srcdir_str, filename, NULL);
      char * dest_full_path = g_build_filename (destdir_str, filename, NULL);

      bool is_dir = g_file_test (src_full_path, G_FILE_TEST_IS_DIR);

      /* recurse if necessary */
      if (recursive && is_dir)
        {
          success = io_copy_dir (
            dest_full_path, src_full_path, follow_symlinks, recursive, &err);
          if (!success)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, "Failed copying directory %s to %s", src_full_path,
                dest_full_path);
              return false;
            }
        }
      /* otherwise if not dir, copy file */
      else if (!is_dir)
        {
          GFile * src_file = g_file_new_for_path (src_full_path);
          GFile * dest_file = g_file_new_for_path (dest_full_path);
          g_return_val_if_fail (src_file && dest_file, false);
          int flags = (int) G_FILE_COPY_OVERWRITE;
          if (!follow_symlinks)
            {
              flags |= (int) G_FILE_COPY_NOFOLLOW_SYMLINKS;
            }
          success = g_file_copy (
            src_file, dest_file, (GFileCopyFlags) flags, NULL, NULL, NULL, &err);
          if (!success)
            {
              PROPAGATE_PREFIXED_ERROR (
                error, err, "Failed copying file %s to %s", src_full_path,
                dest_full_path);
              return false;
            }

          g_object_unref (src_file);
          g_object_unref (dest_file);
        }

      g_free (src_full_path);
      g_free (dest_full_path);
    }
  g_dir_close (srcdir);

  return true;
}

char **
io_get_files_in_dir_as_basenames (
  const char * dir,
  bool         allow_empty,
  bool         with_ext)
{
  if (!with_ext)
    {
      g_critical ("unimplemented");
      return NULL;
    }

  char ** files = io_get_files_in_dir (dir, allow_empty);
  if (!files)
    return NULL;

  int            i = 0;
  GStrvBuilder * builder = g_strv_builder_new ();
  char *         f;
  while ((f = files[i++]))
    {
      char * new_str = g_path_get_basename (f);
      g_strv_builder_add (builder, new_str);
      g_free (new_str);
    }
  g_strfreev (files);

  return g_strv_builder_end (builder);
}

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
  bool         allow_empty)
{
  char ** arr = object_new_n (1, char *);
  int     count = 0;

  append_files_from_dir_ending_in (&arr, &count, recursive, _dir, end_string);

  if (count == 0 && !allow_empty)
    {
      free (arr);
      return NULL;
    }

  return arr;
}

/**
 * Returns a newly allocated path that is either
 * a copy of the original path if the path does
 * not exist, or the original path appended with
 * (n), where n is a number.
 *
 * Example: "myfile" -> "myfile (1)"
 */
char *
io_get_next_available_filepath (const char * filepath)
{
  int          i = 1;
  char *       file_without_ext = io_file_strip_ext (filepath);
  const char * file_ext = io_file_get_ext (filepath);
  char *       new_path = g_strdup (filepath);
  while (file_exists (new_path))
    {
      if (g_file_test (new_path, G_FILE_TEST_IS_DIR))
        {
          g_free (new_path);
          new_path = g_strdup_printf ("%s (%d)", filepath, i++);
        }
      else
        {
          g_free (new_path);
          new_path =
            g_strdup_printf ("%s (%d).%s", file_without_ext, i++, file_ext);
        }
    }
  g_free (file_without_ext);

  return new_path;
}

/**
 * Opens the given directory using the default
 * program.
 */
void
io_open_directory (const char * path)
{
  g_return_if_fail (g_file_test (path, G_FILE_TEST_IS_DIR));

  char command[800];
#ifdef _WOE32
  char * canonical_path = g_canonicalize_filename (path, NULL);
  char * new_path = string_replace (canonical_path, "\\", "\\\\");
  g_free (canonical_path);
  sprintf (command, OPEN_DIR_CMD " \"%s\"", new_path);
  g_free (new_path);
#else
  sprintf (command, OPEN_DIR_CMD " \"%s\"", path);
#endif
  system (command);
  g_message ("executed: %s", command);
}

/**
 * Returns a clone of the given string after
 * removing forbidden characters.
 */
void
io_escape_dir_name (char * dest, const char * dir)
{
  int len = strlen (dir);
  strcpy (dest, dir);
  for (int i = len - 1; i >= 0; i--)
    {
      if (
        dest[i] == '/' || dest[i] == '>' || dest[i] == '<' || dest[i] == '|'
        || dest[i] == ':' || dest[i] == '&' || dest[i] == '(' || dest[i] == ')'
        || dest[i] == ';' || dest[i] == '\\')
        {
          for (int j = i; j < len; j++)
            {
              dest[j] = dest[j + 1];
            }
        }
    }
}

#ifdef _WOE32
char *
io_get_registry_string_val (const char * path)
{
  char  value[8192];
  DWORD BufferSize = 8192;
  char  prefix[500];
  sprintf (prefix, "Software\\%s\\%s\\Settings", PROGRAM_NAME, PROGRAM_NAME);
  RegGetValue (
    HKEY_LOCAL_MACHINE, prefix, path, RRF_RT_ANY, NULL, (PVOID) &value,
    &BufferSize);
  g_message ("reg value: %s", value);
  return g_strdup (value);
}
#endif

#if defined(__APPLE__) && defined(INSTALLER_VER)
/**
 * Gets the bundle path on MacOS.
 *
 * @return Non-zero on fail.
 */
int
io_get_bundle_path (char * bundle_path)
{
  CFBundleRef bundle = CFBundleGetMainBundle ();
  CFURLRef    bundleURL = CFBundleCopyBundleURL (bundle);
  Boolean     success = CFURLGetFileSystemRepresentation (
    bundleURL, TRUE, (UInt8 *) bundle_path, PATH_MAX);
  g_return_val_if_fail (success, -1);
  CFRelease (bundleURL);
  g_message ("bundle path: %s", bundle_path);

  return 0;
}
#endif

/**
 * Returns the new path after traversing any symlinks (using
 * readlink()).
 */
char *
io_traverse_path (const char * abs_path)
{
  /* TODO handle on other platforms as well */
#if defined(__linux__) || defined(__FreeBSD__)
  char * traversed_path = realpath (abs_path, NULL);
  if (traversed_path)
    {
      if (!string_is_equal (traversed_path, abs_path))
        {
          g_debug ("traversed path: %s => %s", abs_path, traversed_path);
        }
      return traversed_path;
    }
  else
    {
      g_warning ("realpath() failed: %s", strerror (errno));
      return g_strdup (abs_path);
    }
#else
  return g_strdup (abs_path);
#endif
}
