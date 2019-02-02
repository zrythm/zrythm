/*
 * gui/backend/file_manager.h - File manager
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_BACKEND_FILE_MANAGER_H__
#define __GUI_BACKEND_FILE_MANAGER_H__

#define FILE_MANAGER (&ZRYTHM->file_manager)
#define file_manager_set_selection(sel, sel_type, load_files) \
  FILE_MANAGER->selection = (void *) sel; \
  FILE_MANAGER->selection_type = sel_type; \
  if (load_files) \
    file_manager_load_files (FILE_MANAGER);

/**
 * Locations to be used in the file browser.
 *
 * These are to be saved/updated every time there are changes
 * and reloaded when Zrythm starts.
 *
 * Label is the visible part and path is the actual path.
 * Eg. "Home" -> /home/user
 * "Documents" -> /home/user/Documents
 * "Samples" -> (some arbitrary path)
 */
typedef struct FileBrowserLocation
{
  char *             label; ///< Human readable label
  char *             path;
} FileBrowserLocation;

/**
 * For easy checking if a file is of a type without string
 * operations.
 */
typedef enum FileTypeEnum
{
  FILE_TYPE_MIDI,
  FILE_TYPE_MP3,
  FILE_TYPE_FLAC,
  FILE_TYPE_OGG,
  FILE_TYPE_WAV,
  FILE_TYPE_DIR,
  FILE_TYPE_PARENT_DIR, ///< special entry ".." at the top
  FILE_TYPE_OTHER,
  NUM_FILE_TYPES,
} FileTypeEnum;

typedef struct FileType
{
  FileTypeEnum       type;
  char *             label; ///< Human readable label
  char *             exts[10]; ///< extensions
  int                num_exts;
} FileType;

typedef struct FileDescriptor
{
  char *              absolute_path; ///< absolute path
  char *              label; ///< human readable label
  FileTypeEnum        type; ///< file type
  int                 hidden; ///< hidden or not

  /* TODO other stuff, sample rate etc. */
} FileDescriptor;

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
   * Descriptors for files under the current collection /
   * location.
   *
   * To be updated every time location / collection selection
   * changes.
   */
  FileDescriptor *         files[10000];
  int                      num_files;

  /**
   * File types.
   *
   * These are not editable.
   */
  FileType *               file_types[NUM_FILE_TYPES];

  /**
   * User collections.
   */
  char *                   collections[50];
  int                      num_collections;

  /**
   * Default locations & user defined locations.
   */
  FileBrowserLocation *    locations[50];
  int                      num_locations;

  /**
   * Current selection in the top window.
   */
  void *                   selection;
  FileBrowserSelectionType selection_type;

} FileManager;

/**
 * Sets default values.
 */
void
file_manager_init (FileManager * self);

/**
 * Loads the files under the current selection.
 */
void
file_manager_load_files (FileManager * self);

FileType *
file_manager_get_file_type_from_enum (
  FileTypeEnum fte);

/**
 * Frees the File Descriptor.
 */
void
file_descr_free (FileDescriptor * fd);

#endif
