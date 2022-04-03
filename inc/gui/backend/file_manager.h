/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_BACKEND_FILE_MANAGER_H__
#define __GUI_BACKEND_FILE_MANAGER_H__

#include <stdbool.h>

typedef struct SupportedFile SupportedFile;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

/**
 * Special location type.
 */
typedef enum FileManagerSpecialLocation
{
  FILE_MANAGER_NONE,
  FILE_MANAGER_HOME,
  FILE_MANAGER_DESKTOP,
} FileManagerSpecialLocation;

#define FILE_MANAGER (ZRYTHM->file_manager)

/**
 * Locations to be used in the file browser.
 *
 * These are to be saved/updated every time there
 * are changes and reloaded when Zrythm starts.
 *
 * Label is the visible part and path is the actual
 * path.
 * Eg. "Home" -> /home/user
 * "Documents" -> /home/user/Documents
 * "Samples" -> (some arbitrary path)
 */
typedef struct FileBrowserLocation
{
  /** Human readable label. */
  char * label;

  /** Absolute path. */
  char * path;

  /** Whether this is a standard (undeletable)
   * location. */
  FileManagerSpecialLocation special_location;
} FileBrowserLocation;

/**
 * Current selection in the top window.
 */
typedef enum FileBrowserSelectionType
{
  FB_SELECTION_TYPE_COLLECTIONS,
  FB_SELECTION_TYPE_LOCATIONS,
} FileBrowserSelectionType;

typedef struct FileManager
{
  /**
   * Descriptors for files under the current
   * collection / location.
   *
   * To be updated every time location / collection
   * selection changes.
   *
   * Array of SupportedFile.
   */
  GPtrArray * files;

#if 0
  /**
   * User collections.
   */
  char *                   collections[50];
  int                      num_collections;
#endif

  /**
   * Default locations & user defined locations.
   *
   * Array of FileBrowserLocation.
   */
  GPtrArray * locations;

  /**
   * Current selection in the top window.
   */
  FileBrowserLocation * selection;

} FileManager;

/**
 * Creates the file manager.
 */
FileManager *
file_manager_new (void);

/**
 * Loads the files under the current selection.
 */
void
file_manager_load_files (FileManager * self);

/**
 * @param save_to_settings Whether to save this
 *   location to GSettings.
 */
NONNULL
void
file_manager_set_selection (
  FileManager *         self,
  FileBrowserLocation * sel,
  bool                  load_files,
  bool                  save_to_settings);

/**
 * Frees the file manager.
 */
void
file_manager_free (FileManager * self);

FileBrowserLocation *
file_browser_location_new (void);

FileBrowserLocation *
file_browser_location_clone (
  FileBrowserLocation * loc);

/**
 * Adds a location and saves the settings.
 */
void
file_manager_add_location_and_save (
  FileManager * self,
  const char *  abs_path);

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
  bool          skip_if_standard);

NONNULL
void
file_browser_location_free (
  FileBrowserLocation * loc);

/**
 * @}
 */

#endif
