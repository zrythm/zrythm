/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

/**
 * \file
 *
 * IO utils.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "utils/io.h"

#include <gtk/gtk.h>

static char * home_dir;

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

static void
setup_home_dir ()
{
  home_dir = (char *) g_get_home_dir ();
}

/**
 * Returns file extension or NULL.
 */
char *
io_file_get_ext (const char * file)
{
  char ** parts = g_strsplit (file, ".", 2);
  char * file_part = parts[0];
  char * ext_part = parts[1];

  g_free (file_part);

  return ext_part;
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
io_path_get_basename (const char * filename)
{
  return g_path_get_basename (filename);
}

char *
io_path_get_parent_dir (const char * path)
{
  char ** parts = g_strsplit (path, G_DIR_SEPARATOR_S, -1);
  char * new_str = NULL;
  char * prev_str = NULL;
  int num_parts = 0;
  while (parts[num_parts])
    {
      num_parts++;
    }

  /* root */
  if (num_parts < 2)
    new_str = g_strdup (G_DIR_SEPARATOR_S);
  else
    {
      for (int i = 0; i < num_parts - 1; i++)
        {
          prev_str = new_str;

          if (i == 1)
            new_str = g_strconcat (new_str,
                                   parts[i],
                                   NULL);
          else if (i == 0)
            new_str = g_strconcat (G_DIR_SEPARATOR_S,
                                   parts[i],
                                   NULL);
          else
            new_str = g_strconcat (new_str,
                                   G_DIR_SEPARATOR_S,
                                   parts[i],
                                   NULL);

          if (prev_str)
            g_free (prev_str);
        }
    }

  g_strfreev (parts);

  g_message ("parent dir for %s is %s",
             path,
             new_str);
  return new_str;
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

  return 0;
}

/**
 * Removes a dir, optionally forcing deletion.
 */
int
io_rmdir (
  const char * path,
  int          force)
{
  /* TODO */
  g_message ("Removing %s", path);

  return 0;
}
