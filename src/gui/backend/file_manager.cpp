// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cctype>
#include <cstdlib>
#include <cstring>

#include "gui/backend/file_manager.h"
#include "io/file_descriptor.h"
#include "settings/g_settings_manager.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/types.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

void
FileManager::add_volume (GVolume * vol)
{
  GMount * mount = g_volume_get_mount (vol);
  char *   name = g_volume_get_name (vol);
  GFile *  root = mount ? g_mount_get_default_location (mount) : NULL;
  char *   path = NULL;
  if (root)
    {
      path = g_file_get_path (root);
      g_object_unref (root);
    }

  g_debug ("vol: %s [%s]", name, path);

  if (path && (!mount || !g_mount_is_shadowed (mount)))
    {
      FileBrowserLocation fl = FileBrowserLocation (
        name, path, FileManagerSpecialLocation::FILE_MANAGER_DRIVE);
      locations.push_back (fl);

      g_debug ("  added location: %s", fl.path_.c_str ());
    }

  object_free_w_func_and_null (g_object_unref, mount);
  g_free_and_null (name);
  g_free_and_null (path);
}

FileManager::FileManager ()
{
  /* add standard locations */
  FileBrowserLocation fl = FileBrowserLocation (
    /* TRANSLATORS: Home directory */
    _ ("Home"), g_get_home_dir (),
    FileManagerSpecialLocation::FILE_MANAGER_HOME);
  locations.push_back (fl);

  set_selection (fl, false, false);

#if 0
  fl = file_browser_location_new ();
  /* TRANSLATORS: Desktop directory */
  fl->label = g_strdup (_ ("Desktop"));
  fl->path = g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP));
  fl->special_location = FileManagerSpecialLocation::FILE_MANAGER_DESKTOP;
  g_ptr_array_add (self->locations, fl);
#endif

  /* drives */
  g_message ("adding drives...");
  GVolumeMonitor * vol_monitor = g_volume_monitor_get ();
  GList *          drives = g_volume_monitor_get_connected_drives (vol_monitor);
  GList *          dl = drives;
  while (dl != NULL)
    {
      GList * dn = dl->next;

      GDrive * drive = G_DRIVE (dl->data);

      char * drive_name = g_drive_get_name (drive);
      g_debug ("drive: %s", drive_name);
      g_free (drive_name);

      GList * vols = g_drive_get_volumes (drive);
      GList * vl = vols;
      while (vl != NULL)
        {
          GList *   vn = vl->next;
          GVolume * vol = G_VOLUME (vl->data);
          add_volume (vol);

          vl = vn;
        }
      g_list_free_full (vols, g_object_unref);

      dl = dn;
    }
  g_list_free_full (drives, g_object_unref);

  /* volumes without an associated drive */
  /* from nautilusgtkplacesview.c */
  g_message ("adding volumes without an associated drive...");
  GList * volumes = g_volume_monitor_get_volumes (vol_monitor);
  for (GList * l = volumes; l != NULL; l = l->next)
    {
      GVolume * vol = G_VOLUME (l->data);
      GDrive *  drive = g_volume_get_drive (vol);

      if (drive)
        {
          g_object_unref (drive);
          continue;
        }

      add_volume (vol);
    }
  g_list_free_full (volumes, g_object_unref);
  g_object_unref (vol_monitor);

  if (!ZRYTHM_TESTING)
    {
      /* add bookmarks */
      char ** bookmarks =
        g_settings_get_strv (S_UI_FILE_BROWSER, "file-browser-bookmarks");
      for (size_t i = 0; bookmarks[i] != NULL; i++)
        {
          char * bookmark = bookmarks[i];
          char * basename = g_path_get_basename (bookmark);
          fl = FileBrowserLocation (
            basename, bookmark, FileManagerSpecialLocation::FILE_MANAGER_NONE);
          g_free (basename);
          locations.push_back (fl);
        }
      g_strfreev (bookmarks);

      /* set remembered location */
      FileBrowserLocation loc = FileBrowserLocation ();
      char *              last_location =
        g_settings_get_string (S_UI_FILE_BROWSER, "last-location");
      loc.path_ = last_location;
      if (
        loc.path_.length () > 0
        && g_file_test (last_location, G_FILE_TEST_IS_DIR))
        {
          set_selection (loc, true, false);
        }
      g_free (last_location);
    }
}

void
FileManager::load_files_from_location (FileBrowserLocation &location)
{
  locations.clear ();

  GDir * dir = g_dir_open (location.path_.c_str (), 0, NULL);
  if (!dir)
    {
      g_warning ("Could not open dir %s", location.path_.c_str ());
      return;
    }

  /* create special parent dir entry */
  {
    char * parent_dir = io_path_get_parent_dir (location.path_.c_str ());
    auto   fd = FileDescriptor ();
    fd.abs_path = parent_dir;
    g_free (parent_dir);
    fd.type = ZFileType::FILE_TYPE_PARENT_DIR;
    fd.hidden = false;
    fd.label = "..";
    if (fd.abs_path.length () > 1)
      {
        files.push_back (fd);
      }
  }

  const gchar * file;
  while ((file = g_dir_read_name (dir)) != nullptr)
    {
      FileDescriptor fd = FileDescriptor ();

      /* set absolute path & label */
      char * absolute_path = g_strdup_printf (
        "%s%s%s", location.path_.length () == 1 ? "" : location.path_.c_str (),
        G_DIR_SEPARATOR_S, file);
      fd.abs_path = absolute_path;
      fd.label = file;

      GError *    err = NULL;
      GFile *     gfile = g_file_new_for_path (absolute_path);
      GFileInfo * info = g_file_query_info (
        gfile, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN, G_FILE_QUERY_INFO_NONE,
        NULL, &err);
      if (err)
        {
          g_warning ("failed to query file info for %s", absolute_path);
        }
      else
        {
          fd.hidden = g_file_info_get_is_hidden (info);
          g_object_unref (info);
        }
      g_object_unref (gfile);

      /* set type */
      if (g_file_test (absolute_path, G_FILE_TEST_IS_DIR))
        {
          fd.type = ZFileType::FILE_TYPE_DIR;
        }
      else
        {
          fd.type = FileDescriptor::get_type_from_path (file);
        }

      /* force hidden if starts with . */
      if (file[0] == '.')
        fd.hidden = true;

      /* add to list */
      files.push_back (fd);

      g_free (absolute_path);
    }
  g_dir_close (dir);

  /* sort alphabetically */
  std::sort (
    files.begin (), files.end (),
    [] (const FileDescriptor &a, const FileDescriptor &b) {
      return a.label < b.label;
    });
  g_message ("Total files: %zu", files.size ());
}

void
FileManager::load_files ()
{
  if (selection != nullptr)
    {
      load_files_from_location (*selection);
    }
  else
    {
      locations.clear ();
    }
}

void
FileManager::set_selection (
  FileBrowserLocation &sel,
  bool                 _load_files,
  bool                 save_to_settings)
{
  g_debug ("setting selection to %s", sel.path_.c_str ());

  selection.reset (new FileBrowserLocation (sel));
  if (_load_files)
    {
      load_files ();
    }
  if (save_to_settings)
    {
      g_settings_set_string (
        S_UI_FILE_BROWSER, "last-location", selection->path_.c_str ());
    }
}

void
FileManager::save_locations ()
{
  GStrvBuilder * strv_builder = g_strv_builder_new ();
  for (auto &loc : locations)
    {
      if (loc.special_location_ > FileManagerSpecialLocation::FILE_MANAGER_NONE)
        continue;

      g_strv_builder_add (strv_builder, loc.path_.c_str ());
    }

  char ** strings = g_strv_builder_end (strv_builder);
  g_settings_set_strv (
    S_UI_FILE_BROWSER, "file-browser-bookmarks", (const char * const *) strings);
  g_strfreev (strings);
}

void
FileManager::add_location_and_save (const char * abs_path)
{
  auto loc = FileBrowserLocation ();
  loc.path_ = abs_path;
  char * tmp = g_path_get_basename (abs_path);
  loc.label_ = tmp;
  g_free (tmp);

  locations.push_back (loc);
  save_locations ();
}

void
FileManager::remove_location_and_save (
  const char * location,
  bool         skip_if_standard)
{
  auto it = std::find_if (locations.begin (), locations.end (), [&] (auto &loc) {
    return loc.path_ == location;
  });

  if (it != locations.end ())
    {
      const auto &existing_loc = *it;
      if (
        !skip_if_standard
        || existing_loc.special_location_
             == FileManagerSpecialLocation::FILE_MANAGER_NONE)
        {
          locations.erase (it);
        }
    }
  else
    {
      g_warning ("%s not found", location);
    }

  save_locations ();
}

void
FileBrowserLocation::print () const
{
  g_message (
    "[FileBrowserLocation] %s: '%s', special: %s", label_.c_str (),
    path_.c_str (), ENUM_NAME (special_location_));
}

GMenuModel *
FileBrowserLocation::generate_context_menu () const
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    _ ("Delete"), "edit-delete", "app.panel-file-browser-delete-bookmark");
  g_menu_append_item (menu, menuitem);

  return G_MENU_MODEL (menu);
}