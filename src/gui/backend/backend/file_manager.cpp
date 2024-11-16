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
#include <memory>

#include "common/io/file_descriptor.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/io.h"
#include "common/utils/objects.h"
#include "common/utils/types.h"
#include "gui/backend/backend/file_manager.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm;

#if 0
void
FileManager::add_volume (GVolume * vol)
{
  GMount *    mount = g_volume_get_mount (vol);
  auto        _name = g_volume_get_name (vol);
  std::string name = _name;
  g_free (_name);
  GFile *     root = mount ? g_mount_get_default_location (mount) : NULL;
  std::string path;
  if (root)
    {
      auto _path = g_file_get_path (root);
      path = _path;
      g_free (_path);
      g_object_unref (root);
    }

  z_debug ("vol: {} [{}]", name, path);

  if (!path.empty () && (!mount || !g_mount_is_shadowed (mount)))
    {
      FileBrowserLocation fl = FileBrowserLocation (
        name.c_str (), path.c_str (),
        FileManagerSpecialLocation::FILE_MANAGER_DRIVE);
      locations.push_back (fl);

      z_debug ("  added location: {}", fl.path_.c_str ());
    }

  object_free_w_func_and_null (g_object_unref, mount);
}
#endif

FileManager::FileManager ()
{
  /* add standard locations */
  FileBrowserLocation fl = FileBrowserLocation (
    /* TRANSLATORS: Home directory */
    QObject::tr ("Home"), QDir::home ().filesystemAbsolutePath (),
    FileManagerSpecialLocation::FILE_MANAGER_HOME);
  locations.push_back (fl);

  set_selection (fl, false, false);

#if 0
  fl = file_browser_location_new ();
  /* TRANSLATORS: Desktop directory */
  fl->label = g_strdup (QObject::tr ("Desktop"));
  fl->path = g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP));
  fl->special_location = FileManagerSpecialLocation::FILE_MANAGER_DESKTOP;
  g_ptr_array_add (self->locations, fl);
#endif

  /* drives */
#if 0
  z_info ("adding drives...");
  GVolumeMonitor * vol_monitor = g_volume_monitor_get ();
  GList *          drives = g_volume_monitor_get_connected_drives (vol_monitor);
  GList *          dl = drives;
  while (dl != nullptr)
    {
      GList * dn = dl->next;

      GDrive * drive = G_DRIVE (dl->data);

      char * drive_name = g_drive_get_name (drive);
      z_debug ("drive: {}", drive_name);
      g_free (drive_name);

      GList * vols = g_drive_get_volumes (drive);
      GList * vl = vols;
      while (vl != nullptr)
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
  z_info ("adding volumes without an associated drive...");
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
#endif

  if (ZRYTHM_HAVE_UI && !ZRYTHM_TESTING)
    {
      /* add bookmarks */
      {
        z_debug ("adding bookmarks...");
        auto bookmarks =
          zrythm::gui::SettingsManager::get_instance ()
            ->get_fileBrowserBookmarks ();
        for (const auto &bookmark : bookmarks)
          {
            auto basename = QFileInfo (bookmark).baseName ();
            locations.emplace_back (
              basename, QFileInfo (bookmark).filesystemAbsoluteFilePath (),
              FileManagerSpecialLocation::FILE_MANAGER_NONE);
          }
      }

      /* set remembered location */
      FileBrowserLocation loc;
      auto                last_location =
        zrythm::gui::SettingsManager::get_instance ()
          ->get_fileBrowserLastLocation ();
      if (!last_location.isEmpty ())
        {
          loc.path_ = QFileInfo (last_location).filesystemAbsoluteFilePath ();
          if (!loc.path_.empty () && fs::is_directory (loc.path_))
            {
              set_selection (loc, true, false);
            }
        }
    }
}

void
FileManager::load_files_from_location (FileBrowserLocation &location)
{
  files.clear ();

      /* create special parent dir entry */
      {
        auto parent_dir = fs::path (location.path_).parent_path ();
        FileDescriptor fd;
        fd.abs_path_ = parent_dir.string ();
        fd.type_ = FileType::ParentDirectory;
        fd.hidden_ = false;
        fd.label_ = "..";
        if (fd.abs_path_.length () > 1)
          {
            files.push_back (fd);
          }
      }

      for (const auto &file : fs::directory_iterator (location.path_))
        {
          FileDescriptor fd;

          /* set absolute path & label */
          auto absolute_path = file.path ();
          fd.abs_path_ = absolute_path.string ();
          fd.label_ = file.path ().filename ().string ();

          fd.hidden_ = utils::io::is_file_hidden (absolute_path);

          /* set type */
          if (fs::is_directory (absolute_path))
            {
              fd.type_ = FileType::Directory;
            }
          else
            {
              fd.type_ = FileDescriptor::get_type_from_path (file);
            }

          /* add to list */
          files.push_back (fd);
        }

  /* sort alphabetically */
  std::sort (files.begin (), files.end (), [] (const auto &a, const auto &b) {
    return a.label_ < b.label_;
  });
  z_info ("Total files: {}", files.size ());
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
  z_debug ("setting selection to {}", sel.path_.c_str ());

  selection = std::make_unique<FileBrowserLocation> (sel);
  if (_load_files)
    {
      load_files ();
    }
  if (save_to_settings)
    {
      gui::SettingsManager::get_instance ()->set_fileBrowserLastLocation (
        QString::fromStdString (selection->path_.string ()));
    }
}

void
FileManager::save_locations ()
{
  QStringList strv_builder;
  for (auto &loc : locations)
    {
      if (loc.special_location_ > FileManagerSpecialLocation::FILE_MANAGER_NONE)
        continue;

      strv_builder.append (QString::fromStdString (loc.path_.string ()));
    }

  gui::SettingsManager::get_instance ()->set_fileBrowserBookmarks (strv_builder);
}

void
FileManager::add_location_and_save (const fs::path &abs_path)
{
  FileBrowserLocation loc;
  loc.path_ = abs_path;
  loc.label_ = QFileInfo (abs_path).baseName ();
  locations.push_back (loc);
  save_locations ();
}

void
FileManager::remove_location_and_save (
  const fs::path &location,
  bool            skip_if_standard)
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
      z_warning ("{} not found", location);
    }

  save_locations ();
}

void
FileBrowserLocation::print () const
{
  z_info (
    "[FileBrowserLocation] {}: '{}', special: {}", label_, path_.c_str (),
    ENUM_NAME (special_location_));
}

#if 0
GMenuModel *
FileBrowserLocation::generate_context_menu () const
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    QObject::tr ("Delete"), "edit-delete", "app.panel-file-browser-delete-bookmark");
  g_menu_append_item (menu, menuitem);

  return G_MENU_MODEL (menu);
}
#endif