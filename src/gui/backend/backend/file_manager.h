// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_FILE_MANAGER_H__
#define __GUI_BACKEND_FILE_MANAGER_H__

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/io/file_descriptor.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

/**
 * Special location type.
 */
enum class FileManagerSpecialLocation
{
  FILE_MANAGER_NONE,
  FILE_MANAGER_HOME,
  FILE_MANAGER_DESKTOP,
  FILE_MANAGER_DRIVE,
};

/**
 * Locations to be used in the file browser.
 *
 * These are to be saved/updated every time there are changes and reloaded when
 * Zrythm starts.
 *
 * Label is the visible part and path is the actual path.
 * Eg. "Home" -> /home/user
 * "Documents" -> /home/user/Documents
 * "Samples" -> (some arbitrary path)
 */
struct FileBrowserLocation
{
  FileBrowserLocation (
    QString                    label,
    fs::path                   path,
    FileManagerSpecialLocation special_location)
      : label_ (std::move (label)), path_ (std::move (path)),
        special_location_ (special_location)
  {
  }
  FileBrowserLocation () = default;

  const char * get_icon_name () const
  {
    switch (special_location_)
      {
      case FileManagerSpecialLocation::FILE_MANAGER_NONE:
        return "folder";
      case FileManagerSpecialLocation::FILE_MANAGER_HOME:
        return "user-home";
      case FileManagerSpecialLocation::FILE_MANAGER_DESKTOP:
        return "desktop";
      case FileManagerSpecialLocation::FILE_MANAGER_DRIVE:
        return "drive-harddisk-symbolic";
      }
    return "folder";
  }

  void print () const;

  // GMenuModel * generate_context_menu () const;

  /** Human readable label. */
  QString label_;

  /** Absolute path. */
  fs::path path_;

  /** Whether this is a standard (undeletable) location. */
  FileManagerSpecialLocation special_location_ = {};
};

/**
 * Current selection in the top window.
 */
enum class FileBrowserSelectionType
{
  FB_SELECTION_TYPE_COLLECTIONS,
  FB_SELECTION_TYPE_LOCATIONS,
};

/**
 * Manages the file browser functionality, including loading files, setting the
 * current selection, adding and removing locations (bookmarks), and saving
 * the locations.
 */
class FileManager
{
public:
  FileManager ();

  /**
   * Loads the files under the current selection.
   */
  void load_files ();

  /**
   * Sets the current selection and optionally loads the files and saves the
   * location to the settings.
   *
   * @param sel The new file browser location to select.
   * @param load_files Whether to load the files under the new selection.
   * @param save_to_settings Whether to save the new selection to the settings.
   */
  void
  set_selection (FileBrowserLocation &sel, bool load_files, bool save_to_settings);

  /**
   * Adds a new location (bookmark) to the saved locations and saves the
   * settings.
   *
   * @param abs_path The absolute path of the new location to add.
   */
  void add_location_and_save (const fs::path &abs_path);

  /**
   * Removes the given location (bookmark) from the saved locations and saves
   * the settings.
   *
   * @param location The location to remove.
   * @param skip_if_standard Whether to skip removal if the location is a
   * standard location.
   */
  void
  remove_location_and_save (const fs::path &location, bool skip_if_standard);

private:
  /**
   * Saves the current locations (bookmarks) to the settings.
   */
  void save_locations ();

  /**
   * Loads the files from the given file browser location.
   *
   * @param location The location to load the files from.
   */
  void load_files_from_location (FileBrowserLocation &location);

  // void add_volume (GVolume * vol);

public:
  /**
   * The file descriptors for the files under the current collection/location.
   * This is updated whenever the location or collection selection changes.
   */
  std::vector<FileDescriptor> files;

  /**
   * The default and user-defined locations (bookmarks).
   */
  std::vector<FileBrowserLocation> locations;

  /**
   * The current selection in the top window.
   */
  std::unique_ptr<FileBrowserLocation> selection;
};

/**
 * @}
 */

#endif
