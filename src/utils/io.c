/*
 * utils/io.c - IO utils
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include "utils/io.h"

#include <gtk/gtk.h>

#ifdef _WIN32
static const char * separator = "\\"
#else
static const char * separator = "/";
#endif

static char * home_dir;

/**
 * Gets system file separator.
 */
char *
io_get_separator () ///< string to write to
{
  return separator;
}

/**
 * Gets directory part of filename. MUST be freed.
 */
char *
io_get_dir (const char * filename) ///< filename containing directory
{
  char * separator = io_get_separator ();
  char ** parts = g_strsplit_set (filename,
                                  separator,
                                  -1);
  int k = 0;
  while (parts[k] != NULL)
    {
      k++;
    }
  parts[k - 1] = NULL;
  char * directory = g_strjoinv (separator, parts);

  g_free (separator);
  g_strfreev (parts);
  return directory;
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
      mkdir(dir, 0700);
    }
}

static void
setup_home_dir ()
{
  if ((home_dir = getenv ("XDG_CONFIG_HOME")) == NULL)
    {
      if ((home_dir = getenv ("HOME")) == NULL)
        {
          home_dir = getpwuid (getuid ())->pw_dir;
        }
    }
}

/**
 * Gets home dir. MUST be freed.
 */
char *
io_get_home_dir ()
{
  if (home_dir == NULL)
    {
      setup_home_dir ();
    }

  return home_dir;
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
  char * file_part = parts[0];
  char * ext_part = parts[1];

  g_free (ext_part);
  return file_part;
}

/**
 * Strips path from given filename.
 *
 * MUST be freed.
 */
char *
io_file_strip_path (const char * filename)
{
  char ** path_file = g_strsplit (filename, io_get_separator (), -1);
  int i = 0;
  while (path_file[i] != NULL)
    i++;
  for (int j = 0; j < i - 1; j++)
    {
      g_free (path_file[j]);
    }

  return path_file[i - 1];
}

char *
io_file_get_creation_datetime (const char * filename)
{
  /* TODO */
  return NULL;
}

char *
io_file_get_last_modified_datetime (const char * filename)
{
  struct stat result;
  struct tm *nowtm;
  char tmbuf[64];
  if (stat (filename, &result)==0)
    {
      struct timespec * ts = &result.st_mtime;
      nowtm = localtime(&ts->tv_sec);
      strftime(tmbuf, sizeof (tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
      char * mod = g_strdup (tmbuf);
      return mod;
    }
  g_warning ("Failed to get last modified");
  return NULL;
}
