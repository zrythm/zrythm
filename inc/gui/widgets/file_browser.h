/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
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
  GtkPaned)

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
  GtkPaned               parent_instance;
  GtkGrid *              browser_top;
  GtkSearchEntry *       browser_search;
  GtkExpander *          collections_exp;
  GtkExpander *          types_exp;
  GtkExpander *          locations_exp;
  GtkBox *               browser_bot;
  GtkLabel *             file_info;
  ZFileType               selected_type;
  GtkTreeModel *         type_tree_model;
  GtkTreeModel *         locations_tree_model;
  GtkTreeModelFilter *   files_tree_model;
  GtkTreeView *          files_tree_view;

  /** The file chooser. */
  GtkFileChooserWidget * file_chooser;

  GtkScrolledWindow *    file_scroll_window;

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
