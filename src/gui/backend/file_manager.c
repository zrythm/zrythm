// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>
#include <string.h>

#include "audio/supported_file.h"
#include "gui/backend/file_manager.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * Creates the file manager.
 */
FileManager *
file_manager_new (void)
{
  FileManager * self = object_new (FileManager);

  self->files = g_ptr_array_new_full (
    400, (GDestroyNotify) supported_file_free);
  self->locations = g_ptr_array_new_with_free_func (
    (GDestroyNotify) file_browser_location_free);

  /* add standard locations */
  FileBrowserLocation * fl = file_browser_location_new ();
  /* TRANSLATORS: Home directory */
  fl->label = g_strdup (_ ("Home"));
  fl->path = g_strdup (g_get_home_dir ());
  fl->special_location = FILE_MANAGER_HOME;
  g_ptr_array_add (self->locations, fl);

  file_manager_set_selection (self, fl, false, false);

  fl = file_browser_location_new ();
  /* TRANSLATORS: Desktop directory */
  fl->label = g_strdup (_ ("Desktop"));
  fl->path = g_strdup (
    g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP));
  fl->special_location = FILE_MANAGER_DESKTOP;
  g_ptr_array_add (self->locations, fl);

  /* drives */
  GVolumeMonitor * vol_monitor = g_volume_monitor_get ();
  GList *          drives =
    g_volume_monitor_get_connected_drives (vol_monitor);
  GList * dl = drives;
  while (dl != NULL)
    {
      GList * dn = dl->next;

      GDrive * drive = G_DRIVE (dl->data);

      g_debug ("drive: %s", g_drive_get_name (drive));

      GList * vols = g_drive_get_volumes (drive);
      GList * vl = vols;
      while (vl != NULL)
        {
          GList *   vn = vl->next;
          GVolume * vol = G_VOLUME (vl->data);
          g_debug ("  vol: %s", g_volume_get_name (vol));
          GMount * mount = g_volume_get_mount (vol);

          if (mount)
            {
              GFile * loc =
                g_mount_get_default_location (mount);

              fl = file_browser_location_new ();
              fl->label = g_volume_get_name (vol);
              fl->path = g_file_get_path (loc);
              fl->special_location = FILE_MANAGER_DRIVE;
              file_browser_location_print (fl);
              g_ptr_array_add (self->locations, fl);

              g_object_unref (loc);
              g_object_unref (mount);
            }
          g_object_unref (vol);

          vl = vn;
        }
      g_list_free (vols);
      g_object_unref (drive);

      dl = dn;
    }
  g_list_free (drives);
  g_object_unref (vol_monitor);

  if (!ZRYTHM_TESTING)
    {
      /* add bookmarks */
      char ** bookmarks = g_settings_get_strv (
        S_UI_FILE_BROWSER, "file-browser-bookmarks");
      for (size_t i = 0; bookmarks[i] != NULL; i++)
        {
          char * bookmark = bookmarks[i];
          fl = file_browser_location_new ();
          fl->label = g_path_get_basename (bookmark);
          fl->path = g_strdup (bookmark);
          fl->special_location = FILE_MANAGER_NONE;
          g_ptr_array_add (self->locations, fl);
        }
      g_strfreev (bookmarks);

      /* set remembered location */
      FileBrowserLocation * loc = file_browser_location_new ();
      loc->path = g_settings_get_string (
        S_UI_FILE_BROWSER, "last-location");
      if (
        strlen (loc->path) > 0
        && g_file_test (loc->path, G_FILE_TEST_IS_DIR))
        {
          file_manager_set_selection (self, loc, true, false);
        }
      file_browser_location_free (loc);
    }

  return self;
}

static int
alphaBetize (const void * _a, const void * _b)
{
  SupportedFile * a = *(SupportedFile * const *) _a;
  SupportedFile * b = *(SupportedFile * const *) _b;
  int             r = strcasecmp (a->label, b->label);
  if (r)
    return r;
  /* if equal ignoring case, use opposite of strcmp()
   * result to get lower before upper */
  return -strcmp (
    a->label, b->label); /* aka: return strcmp(b, a); */
}

static void
load_files_from_location (
  FileManager *         self,
  FileBrowserLocation * location)
{
  const gchar *   file;
  SupportedFile * fd;

  g_ptr_array_remove_range (self->files, 0, self->files->len);

  GDir * dir = g_dir_open (location->path, 0, NULL);
  if (!dir)
    {
      g_warning ("Could not open dir %s", location->path);
      return;
    }

  /* create special parent dir entry */
  fd = object_new (SupportedFile);
  /*g_message ("pre path %s",*/
  /*location->path);*/
  fd->abs_path = io_path_get_parent_dir (location->path);
  /*g_message ("after path %s",*/
  /*fd->abs_path);*/
  fd->type = FILE_TYPE_PARENT_DIR;
  fd->hidden = 0;
  fd->label = g_strdup ("..");
  if (strlen (location->path) > 1)
    {
      g_ptr_array_add (self->files, fd);
    }
  else
    {
      supported_file_free (fd);
      fd = NULL;
    }

  while ((file = g_dir_read_name (dir)))
    {
      fd = object_new (SupportedFile);
      /*fd->dnd_type = UI_DND_TYPE_FILE_DESCRIPTOR;*/

      /* set absolute path & label */
      char * absolute_path = g_strdup_printf (
        "%s%s%s",
        strlen (location->path) == 1 ? "" : location->path,
        G_DIR_SEPARATOR_S, file);
      fd->abs_path = absolute_path;
      fd->label = g_strdup (file);

      GError *    err = NULL;
      GFile *     gfile = g_file_new_for_path (absolute_path);
      GFileInfo * info = g_file_query_info (
        gfile, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
        G_FILE_QUERY_INFO_NONE, NULL, &err);
      if (err)
        {
          g_warning (
            "failed to query file info for %s", absolute_path);
        }
      else
        {
          fd->hidden = g_file_info_get_is_hidden (info);
          g_object_unref (info);
        }
      g_object_unref (gfile);

      /* set type */
      if (g_file_test (absolute_path, G_FILE_TEST_IS_DIR))
        {
          fd->type = FILE_TYPE_DIR;
        }
      else
        {
          fd->type = supported_file_get_type (file);
        }

      /* force hidden if starts with . */
      if (file[0] == '.')
        fd->hidden = true;

      g_ptr_array_add (self->files, fd);
      /*g_message ("File found: %s (%d - %d)",*/
      /*fd->abs_path,*/
      /*fd->type,*/
      /*fd->hidden);*/
    }
  g_dir_close (dir);

  g_ptr_array_sort (self->files, (GCompareFunc) alphaBetize);
  g_message ("Total files: %d", self->files->len);
}

/**
 * Loads the files under the current selection.
 */
void
file_manager_load_files (FileManager * self)
{
  if (self->selection)
    {
      load_files_from_location (
        self, (FileBrowserLocation *) self->selection);
    }
  else
    {
      g_ptr_array_remove_range (
        self->files, 0, self->files->len);
    }
}

/**
 * @param save_to_settings Whether to save this
 *   location to GSettings.
 */
void
file_manager_set_selection (
  FileManager *         self,
  FileBrowserLocation * sel,
  bool                  load_files,
  bool                  save_to_settings)
{
  g_debug ("setting selection to %s", sel->path);

  if (self->selection)
    file_browser_location_free (self->selection);

  self->selection = file_browser_location_clone (sel);
  if (load_files)
    {
      file_manager_load_files (self);
    }
  if (save_to_settings)
    {
      g_settings_set_string (
        S_UI_FILE_BROWSER, "last-location",
        self->selection->path);
    }
}

static bool
file_browser_location_equal_func (
  const FileBrowserLocation * a,
  const FileBrowserLocation * b)
{
  bool ret = string_is_equal (a->path, b->path);
  return ret;
}

static void
save_locations (FileManager * self)
{
  GStrvBuilder * strv_builder = g_strv_builder_new ();
  for (guint i = 0; i < FILE_MANAGER->locations->len; i++)
    {
      FileBrowserLocation * loc =
        g_ptr_array_index (FILE_MANAGER->locations, i);
      if (loc->special_location > FILE_MANAGER_NONE)
        continue;

      g_strv_builder_add (strv_builder, loc->path);
    }

  char ** strings = g_strv_builder_end (strv_builder);
  g_settings_set_strv (
    S_UI_FILE_BROWSER, "file-browser-bookmarks",
    (const char * const *) strings);
  g_strfreev (strings);
}

/**
 * Adds a location and saves the settings.
 */
void
file_manager_add_location_and_save (
  FileManager * self,
  const char *  abs_path)
{
  FileBrowserLocation * loc = file_browser_location_new ();
  loc->path = g_strdup (abs_path);
  loc->label = g_path_get_basename (loc->path);

  g_ptr_array_add (self->locations, loc);

  save_locations (self);
}

/**
 * Removes the given location (bookmark) from the
 * saved locations.
 *
 * @param skip_if_standard Skip removal if the
 *   given location is a standard location.
 */
void
file_manager_remove_location_and_save (
  FileManager * self,
  const char *  location,
  bool          skip_if_standard)
{
  FileBrowserLocation * loc = file_browser_location_new ();
  loc->path = g_strdup (location);

  unsigned int idx;
  bool         ret = g_ptr_array_find_with_equal_func (
            self->locations, loc,
            (GEqualFunc) file_browser_location_equal_func, &idx);
  if (ret)
    {
      FileBrowserLocation * existing_loc =
        g_ptr_array_index (self->locations, idx);
      if (
        !skip_if_standard
        || existing_loc->special_location == FILE_MANAGER_NONE)
        {
          g_ptr_array_remove_index (self->locations, idx);
        }
    }
  else
    {
      g_warning ("%s not found", location);
    }

  file_browser_location_free (loc);

  save_locations (self);
}

/**
 * Frees the file manager.
 */
void
file_manager_free (FileManager * self)
{
  g_ptr_array_free (self->files, true);
  g_ptr_array_free (self->locations, true);

  object_zero_and_free (self);
}

FileBrowserLocation *
file_browser_location_new (void)
{
  return object_new (FileBrowserLocation);
}

FileBrowserLocation *
file_browser_location_clone (FileBrowserLocation * loc)
{
  FileBrowserLocation * self = file_browser_location_new ();
  self->path = g_strdup (loc->path);
  self->label = g_strdup (loc->label);

  return self;
}

void
file_browser_location_print (const FileBrowserLocation * loc)
{
  g_message (
    "[FileBrowserLocation] %s: '%s', special: %d", loc->label,
    loc->path, loc->special_location);
}

void
file_browser_location_free (FileBrowserLocation * loc)
{
  g_free_and_null (loc->label);
  g_free_and_null (loc->path);
  object_zero_and_free (loc);
}
