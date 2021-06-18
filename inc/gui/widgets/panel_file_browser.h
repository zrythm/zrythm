/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_PANEL_FILE_BROWSER_H__
#define __GUI_WIDGETS_PANEL_FILE_BROWSER_H__

#include "audio/supported_file.h"

#include <gtk/gtk.h>

typedef struct _VolumeWidget VolumeWidget;
typedef struct FileBrowserLocation
  FileBrowserLocation;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define PANEL_FILE_BROWSER_WIDGET_TYPE \
  (panel_file_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PanelFileBrowserWidget, panel_file_browser_widget,
  Z, PANEL_FILE_BROWSER_WIDGET, GtkPaned)

#define MW_PANEL_FILE_BROWSER \
  MW_RIGHT_DOCK_EDGE->file_browser

typedef enum PanelFileBrowserFilter
{
  FILE_BROWSER_FILTER_NONE,
  FILE_BROWSER_FILTER_AUDIO,
  FILE_BROWSER_FILTER_MIDI,
  FILE_BROWSER_FILTER_PRESET,
} PanelFileBrowserFilter;

typedef struct _PanelFileBrowserWidget
{
  GtkPaned             parent_instance;
  GtkBox *             browser_top;
  GtkBox *             browser_bot;

  GtkTreeView *        bookmarks_tree_view;
  GtkTreeModel *       bookmarks_tree_model;

  GtkLabel *           file_info;
  ZFileType            selected_type;
  GtkTreeModelFilter * files_tree_model;
  GtkTreeView *        files_tree_view;

  /** Array of SupportedFile. */
  GPtrArray *          selected_locations;
  GPtrArray *          selected_files;

  GtkToggleToolButton * toggle_audio;
  GtkToggleToolButton * toggle_midi;
  GtkToggleToolButton * toggle_presets;

  GtkToolButton *      play_btn;
  GtkToolButton *      stop_btn;
  GtkMenuButton *      file_settings_btn;
  VolumeWidget *       volume;

  GtkComboBoxText *    instrument_cb;

  /** Temp. */
  const FileBrowserLocation * cur_loc;
  const SupportedFile * cur_file;

  bool                 first_draw;
} PanelFileBrowserWidget;

PanelFileBrowserWidget *
panel_file_browser_widget_new (void);

/**
 * @}
 */

#endif
