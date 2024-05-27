// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_PANEL_FILE_BROWSER_H__
#define __GUI_WIDGETS_PANEL_FILE_BROWSER_H__

#include "dsp/supported_file.h"
#include "utils/types.h"

#include "gtk_wrapper.h"

TYPEDEF_STRUCT_UNDERSCORED (FileAuditionerControlsWidget);
TYPEDEF_STRUCT (FileBrowserLocation);
TYPEDEF_STRUCT_UNDERSCORED (FileBrowserFiltersWidget);
TYPEDEF_STRUCT (ItemFactory);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define PANEL_FILE_BROWSER_WIDGET_TYPE (panel_file_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PanelFileBrowserWidget,
  panel_file_browser_widget,
  Z,
  PANEL_FILE_BROWSER_WIDGET,
  GtkWidget);

#define MW_PANEL_FILE_BROWSER MW_RIGHT_DOCK_EDGE->file_browser

typedef struct _PanelFileBrowserWidget
{
  GtkWidget parent_instance;

  GtkPaned * paned;

  GtkBox * browser_top;
  GtkBox * browser_bot;

  GtkListView * bookmarks_list_view;
  ItemFactory * bookmarks_item_factory;

  GtkLabel * file_info;
  ZFileType  selected_type;

  GtkSearchEntry * file_search_entry;

  GtkCustomFilter *    files_filter;
  GtkFilterListModel * files_filter_model;
  GtkSingleSelection * files_selection_model;
  ItemFactory *        files_item_factory;
  GtkListView *        files_list_view;

  /** Array of SupportedFile. */
  GPtrArray * selected_locations;
  GPtrArray * selected_files;

  FileBrowserFiltersWidget * filters_toolbar;

  FileAuditionerControlsWidget * auditioner_controls;

  /** Temp. */
  const SupportedFile * cur_file;

  bool first_draw;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} PanelFileBrowserWidget;

void
panel_file_browser_refresh_bookmarks (PanelFileBrowserWidget * self);

FileBrowserLocation *
panel_file_browser_widget_get_selected_bookmark (PanelFileBrowserWidget * self);

PanelFileBrowserWidget *
panel_file_browser_widget_new (void);

/**
 * @}
 */

#endif
