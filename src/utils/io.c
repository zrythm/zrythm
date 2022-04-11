/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
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
 */

#include "zrythm-config.h"

#define _XOPEN_SOURCE 500
#include <stdio.h>

#include <ftw.h>
#include <unistd.h>

#ifdef _WOE32
#  include <windows.h>
#endif

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils/datetime.h"
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
#  include <unistd.h>
#endif

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
 * @return 0 if the directory exists or was
 *   successfully created, -1 if error was occurred
 *   and errno is set.
 */
int
io_mkdir (const char * dir)
{
  g_message ("Creating directory: %s", dir);
  struct stat st = { 0 };
  if (stat (dir, &st) == -1)
    {
      return g_mkdir_with_parents (dir, 0700);
    }
  return 0;
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
io_touch_file (const char * filename)
{
  return fopen (filename, "ab+");
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
  char * new_str =
    g_strndup (filename, (gsize) size);

  return new_str;
}

/**
 * Strips path from given filename.
 *
 * MUST be freed.
 */
char *
io_path_get_basename_without_ext (
  const char * filename)
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
  char regex[] =
    "(" ROOT_REGEX ".*)" PATH_SEP "[^" PATH_SEP
    "]+";
  char * parent =
    string_get_regex_group (path, regex, 1);
#if 0
  g_message ("[%s]\npath: %s\nregex: %s\nparent: %s",
    __func__, path, regex, parent);
#endif

  if (!parent)
    {
      strcpy (
        regex, "(" ROOT_REGEX ")[^" PATH_SEP "]*");
      parent =
        string_get_regex_group (path, regex, 1);
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
io_file_get_creation_datetime (
  const char * filename)
{
  /* TODO */
  return NULL;
}

/**
 * Returns the number of seconds since the epoch, or
 * -1 if failed.
 */
gint64
io_file_get_last_modified_datetime (
  const char * filename)
{
  struct stat result;
  if (stat (filename, &result) == 0)
    {
      return result.st_mtime;
    }
  g_message (
    "Failed to get last modified for %s", filename);
  return -1;
}

char *
io_file_get_last_modified_datetime_as_str (
  const char * filename)
{
  gint64 secs =
    io_file_get_last_modified_datetime (filename);
  if (secs == -1)
    return NULL;

  return datetime_epoch_to_str (
    secs, "%Y-%m-%d %H:%M:%S");
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
      g_return_val_if_fail (
        g_path_is_absolute (path)
          && strlen (path) > 20,
        -1);
      nftw (
        path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
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
          g_warning (
            "Failed opening directory %s: %s",
            _dir, error->message);
        }
      return;
    }

  while ((filename = g_dir_read_name (dir)))
    {
      full_path =
        g_build_filename (_dir, filename, NULL);

      /* recurse if necessary */
      if (
        recursive
        && g_file_test (
          full_path, G_FILE_TEST_IS_DIR))
        {
          append_files_from_dir_ending_in (
            files, num_files, recursive, full_path,
            end_string);
        }

      if (
        !end_string
        || (end_string && g_str_has_suffix (full_path, end_string)))
        {
          *files = realloc (
            *files,
            sizeof (char *)
              * (size_t) (*num_files + 2));
          (*files)[(*num_files)] =
            g_strdup (full_path);
          (*num_files)++;
        }

      g_free (full_path);
    }
  g_dir_close (dir);

  /* NULL terminate */
  (*files)[*num_files] = NULL;
}

void
io_copy_dir (
  const char * destdir_str,
  const char * srcdir_str,
  bool         follow_symlinks,
  bool         recursive)
{
  GDir *        srcdir;
  GError *      error = NULL;
  const gchar * filename;

  g_debug (
    "attempting to copy dir '%s' to '%s' "
    "(recursive: %d)",
    srcdir_str, destdir_str, recursive);

  srcdir = g_dir_open (srcdir_str, 0, &error);
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
          g_warning (
            "Failed opening directory %s: %s",
            srcdir_str, error->message);
        }
      return;
    }

  io_mkdir (destdir_str);

  while ((filename = g_dir_read_name (srcdir)))
    {
      char * src_full_path = g_build_filename (
        srcdir_str, filename, NULL);
      char * dest_full_path = g_build_filename (
        destdir_str, filename, NULL);

      bool is_dir = g_file_test (
        src_full_path, G_FILE_TEST_IS_DIR);

      /* recurse if necessary */
      if (recursive && is_dir)
        {
          io_copy_dir (
            dest_full_path, src_full_path,
            follow_symlinks, recursive);
        }
      /* otherwise if not dir, copy file */
      else if (!is_dir)
        {
          GFile * src_file =
            g_file_new_for_path (src_full_path);
          GFile * dest_file =
            g_file_new_for_path (dest_full_path);
          g_return_if_fail (src_file && dest_file);
          error = NULL;
          GFileCopyFlags flags =
            G_FILE_COPY_OVERWRITE;
          if (!follow_symlinks)
            {
              flags |=
                G_FILE_COPY_NOFOLLOW_SYMLINKS;
            }
          bool ret = g_file_copy (
            src_file, dest_file, flags, NULL, NULL,
            NULL, &error);
          if (!ret)
            {
              g_warning (
                "Failed copying file %s to %s: %s",
                src_full_path, dest_full_path,
                error->message);
              return;
            }

          g_object_unref (src_file);
          g_object_unref (dest_file);
        }

      g_free (src_full_path);
      g_free (dest_full_path);
    }
  g_dir_close (srcdir);
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

  append_files_from_dir_ending_in (
    &arr, &count, recursive, _dir, end_string);

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
io_get_next_available_filepath (
  const char * filepath)
{
  int    i = 1;
  char * file_without_ext =
    io_file_strip_ext (filepath);
  const char * file_ext =
    io_file_get_ext (filepath);
  char * new_path = g_strdup (filepath);
  while (file_exists (new_path))
    {
      if (g_file_test (new_path, G_FILE_TEST_IS_DIR))
        {
          g_free (new_path);
          new_path = g_strdup_printf (
            "%s (%d)", filepath, i++);
        }
      else
        {
          g_free (new_path);
          new_path = g_strdup_printf (
            "%s (%d).%s", file_without_ext, i++,
            file_ext);
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
  g_return_if_fail (
    g_file_test (path, G_FILE_TEST_IS_DIR));

  char command[800];
#ifdef _WOE32
  char * canonical_path =
    g_canonicalize_filename (path, NULL);
  char * new_path =
    string_replace (canonical_path, "\\", "\\\\");
  g_free (canonical_path);
  sprintf (
    command, OPEN_DIR_CMD " \"%s\"", new_path);
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
        dest[i] == '/' || dest[i] == '>'
        || dest[i] == '<' || dest[i] == '|'
        || dest[i] == ':' || dest[i] == '&'
        || dest[i] == '(' || dest[i] == ')'
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
  sprintf (
    prefix, "Software\\%s\\%s\\Settings",
    PROGRAM_NAME, PROGRAM_NAME);
  RegGetValue (
    HKEY_LOCAL_MACHINE, prefix, path, RRF_RT_ANY,
    NULL, (PVOID) &value, &BufferSize);
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
  CFURLRef    bundleURL =
    CFBundleCopyBundleURL (bundle);
  Boolean success = CFURLGetFileSystemRepresentation (
    bundleURL, TRUE, (UInt8 *) bundle_path,
    PATH_MAX);
  g_return_val_if_fail (success, -1);
  CFRelease (bundleURL);
  g_message ("bundle path: %s", bundle_path);

  return 0;
}
#endif
