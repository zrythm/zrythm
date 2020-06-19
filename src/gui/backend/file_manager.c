/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>
#include <string.h>

#include "audio/supported_file.h"
#include "gui/backend/file_manager.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "zrythm.h"

#include "gtk/gtk.h"

/**
 * Creates the file manager.
 */
FileManager *
file_manager_new (void)
{
  FileManager * self = object_new (FileManager);

  self->num_files = 0;
  self->num_collections = 0;

  /* add locations */
  FileBrowserLocation * fl =
    object_new (FileBrowserLocation);
  fl->label = g_strdup ("Home");
  fl->path = g_strdup (g_get_home_dir ());
  self->locations[0] = fl;
  self->num_locations = 1;

  file_manager_set_selection (
    self, fl, FB_SELECTION_TYPE_LOCATIONS, 0);

  return self;
}

static int
alphaBetize (const void * _a,
             const void * _b)
{
  SupportedFile * a = *(SupportedFile * const *) _a;
  SupportedFile * b = *(SupportedFile * const *) _b;
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
  SupportedFile * fd;
  self->num_files = 0;
  GDir * dir =
    g_dir_open (
      location->path, 0, NULL);
  if (!dir)
    {
      g_warning ("Could not open dir %s",
                 location->path);
      return;
    }

  /* create special parent dir entry */
  fd = calloc (1, sizeof (SupportedFile));
  /*g_message ("pre path %s",*/
             /*location->path);*/
  fd->abs_path =
    io_path_get_parent_dir (location->path);
  /*g_message ("after path %s",*/
             /*fd->abs_path);*/
  fd->type = FILE_TYPE_PARENT_DIR;
  fd->hidden = 0;
  fd->label = g_strdup ("..");
  if (strlen (location->path) > 1)
    {
      self->files[self->num_files++] = fd;
    }
  else
    {
      supported_file_free (fd);
    }

  while ((file = g_dir_read_name (dir)))
    {
      fd = calloc (1, sizeof (SupportedFile));
      /*fd->dnd_type = UI_DND_TYPE_FILE_DESCRIPTOR;*/

      /* set absolute path & label */
      char * absolute_path =
        g_strdup_printf (
          "%s%s%s",
          strlen (location->path) == 1 ?
            "" : location->path,
          G_DIR_SEPARATOR_S, file);
      fd->abs_path = absolute_path;
      fd->label = g_strdup (file);

      /* set type */
      if (g_file_test (
            absolute_path, G_FILE_TEST_IS_DIR))
        {
          fd->type = FILE_TYPE_DIR;
        }
      else
        {
          fd->type = supported_file_get_type (file);
        }

      /* set hidden */
      if (file[0] == '.')
        fd->hidden = 1;
      else
        fd->hidden = 0;

      self->files[self->num_files++] = fd;
      /*g_message ("File found: %s (%d - %d)",*/
                 /*fd->abs_path,*/
                 /*fd->type,*/
                 /*fd->hidden);*/
    }
  qsort (self->files,
         (size_t) self->num_files,
         sizeof (SupportedFile *),
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
      if (self->selection_type ==
            FB_SELECTION_TYPE_LOCATIONS)
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
file_manager_set_selection (
  FileManager *            self,
  void *                   sel,
  FileBrowserSelectionType sel_type,
  bool                     load_files)
{
  self->selection = sel;
  self->selection_type = sel_type;
  if (load_files)
    {
      file_manager_load_files (self);
    }
}

/**
 * Frees the file manager.
 */
void
file_manager_free (
  FileManager * self)
{
  /* TODO */

  object_zero_and_free (self);
}
