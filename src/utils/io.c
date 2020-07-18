/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include <windows.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "utils/file.h"
#include "utils/io.h"
#include "utils/string.h"
#include "utils/system.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#ifdef MAC_RELEASE
#include "CoreFoundation/CoreFoundation.h"
#include <unistd.h>
#include <libgen.h>
#endif

/**
 * Gets directory part of filename. MUST be freed.
 */
char *
io_get_dir (const char * filename) ///< filename containing directory
{
  return g_path_get_dirname (filename);
}

/**
 * Makes directory if doesn't exist.
 */
void
io_mkdir (const char * dir)
{
  struct stat st = {0};
  if (stat(dir, &st) == -1)
    {
      g_mkdir_with_parents (dir, 0700);
    }
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
  return fopen(filename, "ab+");
}

/**
 * Strips extensions from given filename.
 *
 * MUST be freed.
 */
char *
io_file_strip_ext (const char * filename)
{
  char ** parts = g_strsplit (filename, ".", 2);
  char * file_part =
    g_strdup (parts[0]);
  g_strfreev (parts);
  return file_part;
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
  char * no_ext =
    io_file_strip_ext (basename);
  g_free (basename);

  return no_ext;
}

char *
io_path_get_parent_dir (
  const char * path)
{
  g_return_val_if_fail (path, NULL);

#ifdef _WOE32
#define PATH_SEP "\\\\"
#define ROOT_REGEX "[A-Z]:" PATH_SEP
#else
#define PATH_SEP "/"
#define ROOT_REGEX "/"
#endif
  char regex[] =
    "(" ROOT_REGEX ".*)" PATH_SEP "[^" PATH_SEP "]+";
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
io_file_get_creation_datetime (const char * filename)
{
  /* TODO */
  return NULL;
}

char *
io_file_get_last_modified_datetime (
  const char * filename)
{
  struct stat result;
  struct tm *nowtm;
  char tmbuf[64];
  if (stat (filename, &result)==0)
    {
      nowtm = localtime (&result.st_mtime);
      strftime(tmbuf, sizeof (tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
      char * mod = g_strdup (tmbuf);
      return mod;
    }
  g_message ("Failed to get last modified for %s",
             filename);
  return NULL;
}

/**
 * Removes the given file.
 */
int
io_remove (
  const char * path)
{
  g_message ("Removing %s", path);
  return g_remove (path);
}

static int
unlink_cb (
  const char *fpath,
  const struct stat *sb,
  int typeflag,
  struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

/**
 * Removes a dir, optionally forcing deletion.
 *
 * For safety reasons, this only accepts an
 * absolute path with length greater than 25 if
 * forced.
 */
int
io_rmdir (
  const char * path,
  int          force)
{
  if (force)
    {
      g_return_val_if_fail (
        g_path_is_absolute (path) &&
        strlen (path) > 25, -1);
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
  GDir *dir;
  GError *error;
  const gchar *filename;
  char * full_path;

  dir = g_dir_open (_dir, 0, &error);
  while ((filename = g_dir_read_name (dir)))
    {
      full_path =
        g_build_filename (
          _dir, filename, NULL);

      /* recurse if necessary */
      if (recursive &&
          g_file_test (
            full_path, G_FILE_TEST_IS_DIR))
        {
          append_files_from_dir_ending_in (
            files, num_files, recursive, full_path,
            end_string);
        }

      if (!end_string ||
          (end_string &&
             g_str_has_suffix (
               full_path, end_string)))
        {
          *files =
            realloc (
              *files,
              sizeof (char *) *
                (size_t) (*num_files + 2));
          (*files)[(*num_files)] =
            g_strdup (full_path);
          (*num_files)++;
        }

      g_free (full_path);
    }

  /* NULL terminate */
  (*files)[*num_files] = NULL;
}

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
  const char * _dir,
  const int    recursive,
  const char * end_string)
{
  char ** arr =
    calloc (1, sizeof (char *));
  int count = 0;

  append_files_from_dir_ending_in (
    &arr, &count, recursive, _dir, end_string);

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
  int i = 1;
  char * file_without_ext =
    io_file_strip_ext (filepath);
  const char * file_ext = io_file_get_ext (filepath);
  char * new_path = g_strdup (filepath);
  while (file_exists (new_path))
    {
      if (g_file_test (
            new_path, G_FILE_TEST_IS_DIR))
        {
          g_free (new_path);
          new_path =
            g_strdup_printf (
              "%s (%d)", filepath, i++);
        }
      else
        {
          g_free (new_path);
          new_path =
            g_strdup_printf (
              "%s (%d).%s",
              file_without_ext, i++,
              file_ext);
        }
    }
  g_free (file_without_ext);

  return new_path;
}

/* fallback for glib < 2.58 */
#ifndef HAVE_G_CANONICALIZE_FILENAME
/**
 * g_canonicalize_filename:
 * @filename: (type filename): the name of the file
 * @relative_to: (type filename) (nullable): the relative directory, or %NULL
 * to use the current working directory
 *
 * Gets the canonical file name from @filename. All triple slashes are turned into
 * single slashes, and all `..` and `.`s resolved against @relative_to.
 *
 * Symlinks are not followed, and the returned path is guaranteed to be absolute.
 *
 * If @filename is an absolute path, @relative_to is ignored. Otherwise,
 * @relative_to will be prepended to @filename to make it absolute. @relative_to
 * must be an absolute path, or %NULL. If @relative_to is %NULL, it'll fallback
 * to g_get_current_dir().
 *
 * This function never fails, and will canonicalize file paths even if they don't
 * exist.
 *
 * No file system I/O is done.
 *
 * Returns: (type filename) (transfer full): a newly allocated string with the
 * canonical file path
 * Since: 2.58
 */
static gchar *
g_canonicalize_filename (const gchar *filename,
                         const gchar *relative_to)
{
  gchar *canon, *start, *p, *q;
  guint i;

  g_return_val_if_fail (relative_to == NULL || g_path_is_absolute (relative_to), NULL);

  if (!g_path_is_absolute (filename))
    {
      gchar *cwd_allocated = NULL;
      const gchar  *cwd;

      if (relative_to != NULL)
        cwd = relative_to;
      else
        cwd = cwd_allocated = g_get_current_dir ();

      canon = g_build_filename (cwd, filename, NULL);
      g_free (cwd_allocated);
    }
  else
    {
      canon = g_strdup (filename);
    }

  start = (char *)g_path_skip_root (canon);

  if (start == NULL)
    {
      /* This shouldn't really happen, as g_get_current_dir() should
         return an absolute pathname, but bug 573843 shows this is
         not always happening */
      g_free (canon);
      return g_build_filename (G_DIR_SEPARATOR_S, filename, NULL);
    }

  /* POSIX allows double slashes at the start to
   * mean something special (as does windows too).
   * So, "//" != "/", but more than two slashes
   * is treated as "/".
   */
  i = 0;
  for (p = start - 1;
       (p >= canon) &&
         G_IS_DIR_SEPARATOR (*p);
       p--)
    i++;
  if (i > 2)
    {
      i -= 1;
      start -= i;
      memmove (start, start+i, strlen (start+i) + 1);
    }

  /* Make sure we're using the canonical dir separator */
  p++;
  while (p < start && G_IS_DIR_SEPARATOR (*p))
    *p++ = G_DIR_SEPARATOR;

  p = start;
  while (*p != 0)
    {
      if (p[0] == '.' && (p[1] == 0 || G_IS_DIR_SEPARATOR (p[1])))
        {
          memmove (p, p+1, strlen (p+1)+1);
        }
      else if (p[0] == '.' && p[1] == '.' && (p[2] == 0 || G_IS_DIR_SEPARATOR (p[2])))
        {
          q = p + 2;
          /* Skip previous separator */
          p = p - 2;
          if (p < start)
            p = start;
          while (p > start && !G_IS_DIR_SEPARATOR (*p))
            p--;
          if (G_IS_DIR_SEPARATOR (*p))
            *p++ = G_DIR_SEPARATOR;
          memmove (p, q, strlen (q)+1);
        }
      else
        {
          /* Skip until next separator */
          while (*p != 0 && !G_IS_DIR_SEPARATOR (*p))
            p++;

          if (*p != 0)
            {
              /* Canonicalize one separator */
              *p++ = G_DIR_SEPARATOR;
            }
        }

      /* Remove additional separators */
      q = p;
      while (*q && G_IS_DIR_SEPARATOR (*q))
        q++;

      if (p != q)
        memmove (p, q, strlen (q) + 1);
    }

  /* Remove trailing slashes */
  if (p > start && G_IS_DIR_SEPARATOR (*(p-1)))
    *(p-1) = 0;

  return canon;
}
#endif

/**
 * Opens the given directory using the default
 * program.
 */
void
io_open_directory (
  const char * path)
{
  g_return_if_fail (
    g_file_test (path, G_FILE_TEST_IS_DIR));

  char command[800];
#ifdef _WOE32
  char * canonical_path =
    g_canonicalize_filename (path, NULL);
  char * new_path =
    string_replace (
      canonical_path, "\\", "\\\\");
  g_free (canonical_path);
  sprintf (
    command, OPEN_DIR_CMD " \"%s\"",
    new_path);
  g_free (new_path);
#else
  sprintf (
    command, OPEN_DIR_CMD " \"%s\"",
    path);
#endif
  system (command);
  g_message ("executed: %s", command);
}

/**
 * Returns a clone of the given string after
 * removing forbidden characters.
 */
void
io_escape_dir_name (
  char *       dest,
  const char * dir)
{
  int len = strlen (dir);
  strcpy (dest, dir);
  for (int i = len - 1; i >= 0; i--)
    {
      if (dest[i] == '/' ||
          dest[i] == '>' ||
          dest[i] == '<' ||
          dest[i] == '|' ||
          dest[i] == ':' ||
          dest[i] == '&' ||
          dest[i] == '(' ||
          dest[i] == ')' ||
          dest[i] == ';' ||
          dest[i] == '\\')
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
io_get_registry_string_val (
  const char * path)
{
  char value[8192];
  DWORD BufferSize = 8192;
  char prefix[500];
  sprintf (
    prefix, "Software\\%s\\%s\\Settings",
    PROGRAM_NAME, PROGRAM_NAME);
  RegGetValue (
    HKEY_LOCAL_MACHINE, prefix, path,
    RRF_RT_ANY, NULL, (PVOID) &value, &BufferSize);
  g_message ("reg value: %s", value);
  return g_strdup (value);
}
#endif

#ifdef MAC_RELEASE
/**
 * Gets the bundle path on MacOS.
 *
 * @return Non-zero on fail.
 */
int
io_get_bundle_path (
  char * bundle_path)
{
  CFBundleRef bundle =
    CFBundleGetMainBundle ();
  CFURLRef bundleURL =
    CFBundleCopyBundleURL (bundle);
  Boolean success =
    CFURLGetFileSystemRepresentation (
      bundleURL, TRUE,
      (UInt8 *) bundle_path, PATH_MAX);
  g_return_val_if_fail (success, -1);
  CFRelease (bundleURL);
  g_message ("bundle path: %s", bundle_path);

  return 0;
}
#endif
