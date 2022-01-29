/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * File browser widget.
 */

#ifndef __GUI_WIDGETS_FILE_BROWSER_H__
#define __GUI_WIDGETS_FILE_BROWSER_H__

#include "audio/supported_file.h"

#include <gtk/gtk.h>

#define FILE_BROWSER_WIDGET_TYPE \
  (file_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FileBrowserWidget,
  file_browser_widget,
  Z, FILE_BROWSER_WIDGET,
  GtkBox)

typedef struct _FileAuditionerControlsWidget
  FileAuditionerControlsWidget;
typedef struct _FileBrowserFiltersWidget
  FileBrowserFiltersWidget;
typedef struct _WrappedObjectWithChangeSignal
  WrappedObjectWithChangeSignal;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_FILE_BROWSER \
  MW_RIGHT_DOCK_EDGE->file_browser

/**
 * The file browser for samples, MIDI files, etc.
 */
typedef struct _FileBrowserWidget
{
  GtkBox               parent_instance;
  GtkBox *             browser_top;
  GtkBox *             browser_bot;
  GtkLabel *           file_info;
  ZFileType            selected_type;

  FileBrowserFiltersWidget * filters_toolbar;

  /** The file chooser. */
  GtkBox *              file_chooser_box;
  GtkFileChooserWidget * file_chooser;

  FileAuditionerControlsWidget * auditioner_controls;

  /** Currently selected file. */
  WrappedObjectWithChangeSignal * selected_file;

  /**
   * A little hack to get the paned position to
   * get set from the gsettings when first created.
   *
   * Had problems where the position was quickly
   * overwritten by something random within 300 ms
   * of widget creation.
   */
  int                  start_saving_pos;
  int                  first_time_position_set;
  gint64               first_time_position_set_time;
} FileBrowserWidget;

/**
 * Creates a new FileBrowserWidget.
 */
FileBrowserWidget *
file_browser_widget_new (void);

/**
 * @}
 */

#endif
