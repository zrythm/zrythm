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

#ifndef __GUI_WIDGETS_PANEL_FILE_BROWSER_H__
#define __GUI_WIDGETS_PANEL_FILE_BROWSER_H__

#include "audio/supported_file.h"

#include <gtk/gtk.h>

#define PANEL_FILE_BROWSER_WIDGET_TYPE \
  (panel_file_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (PanelFileBrowserWidget,
                      panel_file_browser_widget,
                      Z,
                      PANEL_FILE_BROWSER_WIDGET,
                      GtkPaned)

#define MW_PANEL_FILE_BROWSER \
  MW_RIGHT_DOCK_EDGE->file_browser

typedef struct SupportedFile SupportedFile;

typedef struct _PanelFileBrowserWidget
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
  GtkScrolledWindow *    file_scroll_window;
  SupportedFile *        selected_file_descr;

  bool                   first_draw;
} PanelFileBrowserWidget;

PanelFileBrowserWidget *
panel_file_browser_widget_new (void);

#endif
