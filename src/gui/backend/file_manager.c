/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#include <stdlib.h>
#include <string.h>

#include "gui/backend/file_manager.h"
#include "utils/io.h"
#include "zrythm.h"

#include "gtk/gtk.h"

/**
 * Sets default values.
 */
void
file_manager_init (FileManager * self)
{
  self->num_files = 0;
  self->num_collections = 0;

  /* set file types */
  FileType * ft;
  FileBrowserLocation * fl;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_MIDI;
  ft->label = g_strdup ("MIDI");
  ft->exts[0] = g_strdup (".mid");
  ft->exts[1] = g_strdup (".smf");
  ft->num_exts = 2;
  self->file_types[FILE_TYPE_MIDI] = ft;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_MP3;
  ft->label = g_strdup ("MP3");
  ft->exts[0] = g_strdup (".mp3");
  ft->num_exts = 1;
  self->file_types[FILE_TYPE_MP3] = ft;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_FLAC;
  ft->label = g_strdup ("FLAC");
  ft->exts[0] = g_strdup (".flac");
  ft->num_exts = 1;
  self->file_types[FILE_TYPE_FLAC] = ft;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_OGG;
  ft->label = g_strdup ("OGG");
  ft->exts[0] = g_strdup (".ogg");
  ft->num_exts = 1;
  self->file_types[FILE_TYPE_OGG] = ft;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_WAV;
  ft->label = g_strdup ("Wave");
  ft->exts[0] = g_strdup (".wav");
  ft->num_exts = 1;
  self->file_types[FILE_TYPE_WAV] = ft;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_DIR;
  ft->label = g_strdup ("Directory");
  ft->num_exts = 0;
  self->file_types[FILE_TYPE_DIR] = ft;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_PARENT_DIR;
  ft->label = g_strdup ("Parent directory");
  ft->num_exts = 0;
  self->file_types[FILE_TYPE_PARENT_DIR] = ft;

  ft = calloc (1, sizeof (FileType));
  ft->type = FILE_TYPE_OTHER;
  ft->label = g_strdup ("Other");
  ft->num_exts = 0;
  self->file_types[FILE_TYPE_OTHER] = ft;

  /* add locations */
  fl = calloc (1, sizeof (FileBrowserLocation));
  fl->label = g_strdup ("Home");
  fl->path = g_strdup (g_get_home_dir ());
  self->locations[0] = fl;
  self->num_locations = 1;

  file_manager_set_selection (fl,
                              FB_SELECTION_TYPE_LOCATIONS,
                              0);
}

static int
alphaBetize (const void * _a,
             const void * _b)
{
  FileDescriptor * a = *(FileDescriptor * const *) _a;
  FileDescriptor * b = *(FileDescriptor * const *) _b;
  int r = strcasecmp(a->label, b->label);
  if (r) return r;
  /* if equal ignoring case, use opposite of strcmp()
   * result to get lower before upper */
  return -strcmp(a->label, b->label); /* aka: return strcmp(b, a); */
}

static void
load_files_from_location (
  FileManager *         self,
  FileBrowserLocation * location)
{
  const gchar * file;
  FileDescriptor * fd;
  self->num_files = 0;
  GDir * dir = g_dir_open (location->path,
                           0,
                           NULL);
  if (!dir)
    {
      g_warning ("Could not open dir %s",
                 location->path);
      return;
    }

  /* create special parent dir entry */
  fd = malloc (sizeof (FileDescriptor));
  g_message ("pre path %s",
             location->path);
  fd->absolute_path =
    io_path_get_parent_dir (location->path);
  g_message ("after path %s",
             fd->absolute_path);
  fd->type = FILE_TYPE_PARENT_DIR;
  fd->hidden = 0;
  fd->label = g_strdup ("..");
  if (strlen (location->path) > 1)
    {
      self->files[self->num_files++] = fd;
    }
  else
    {
      file_descr_free (fd);
    }

  while ((file = g_dir_read_name (dir)))
    {
      fd = malloc (sizeof (FileDescriptor));
      /*fd->dnd_type = UI_DND_TYPE_FILE_DESCRIPTOR;*/

      /* set absolute path & label */
      char * absolute_path =
        g_strdup_printf ("%s%s%s",
                         strlen (location->path) == 1 ?
                           "" :
                           location->path,
                         G_DIR_SEPARATOR_S,
                         file);
      fd->absolute_path = absolute_path;
      fd->label = g_strdup (file);

      /* set type */
      if (g_file_test (absolute_path,
                       G_FILE_TEST_IS_DIR))
        {
          fd->type = FILE_TYPE_DIR;
        }
      else
        {
          char * ext = io_file_get_ext (file);

          if (ext)
            {
              if (!g_ascii_strncasecmp (ext, "wav", 3))
                fd->type = FILE_TYPE_WAV;
              else if (!g_ascii_strncasecmp (ext, "ogg", 3))
                fd->type = FILE_TYPE_OGG;
              else if (!g_ascii_strncasecmp (ext, "mid", 3) ||
                       !g_ascii_strncasecmp (ext, "smf", 3))
                fd->type = FILE_TYPE_MIDI;
              else if (!g_ascii_strncasecmp (ext, "flac", 3))
                fd->type = FILE_TYPE_FLAC;
              else if (!g_ascii_strncasecmp (ext, "mp3", 3))
                fd->type = FILE_TYPE_MP3;
              else
                fd->type = FILE_TYPE_OTHER;
            }
          else
            fd->type = FILE_TYPE_OTHER;
        }

      /* set hidden */
      if (file[0] == '.')
        fd->hidden = 1;
      else
        fd->hidden = 0;

      self->files[self->num_files++] = fd;
      /*g_message ("File found: %s (%d - %d)",*/
                 /*fd->absolute_path,*/
                 /*fd->type,*/
                 /*fd->hidden);*/
    }
  qsort (self->files,
         self->num_files,
         sizeof (FileDescriptor *),
         alphaBetize);
  g_message ("Total files: %d",
             self->num_files);
}

/**
 * Loads the files under the current selection.
 */
void
file_manager_load_files (FileManager * self)
{
  if (self->selection)
    {
      if (self->selection_type == FB_SELECTION_TYPE_LOCATIONS)
        {
          load_files_from_location (
            self,
            (FileBrowserLocation *) self->selection);
        }
    }
  else
    {
      self->num_files = 0;
    }
}

void
file_manager_clone_descr (
  FileDescriptor * src,
  FileDescriptor * dest)
{
  dest->absolute_path =
    g_strdup (src->absolute_path);
  dest->label =
    g_strdup (src->label);
  dest->type = src->type;
  dest->hidden = src->hidden;
}

/**
 * Returns the FileType struct from the enum.
 */
FileType *
file_manager_get_file_type_from_enum (
  FileTypeEnum fte)
{
  for (int i = 0; i < NUM_FILE_TYPES; i++)
    {
      if (fte == FILE_MANAGER->file_types[i]->type)
        return FILE_MANAGER->file_types[i];
    }
  g_warn_if_reached ();
  return NULL;
}

void
file_descr_free (FileDescriptor * fd)
{
  if (fd->absolute_path)
    g_free (fd->absolute_path);
  if (fd->label)
    g_free (fd->label);
  free (fd);
}
