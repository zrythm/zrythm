// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_PANEL_FILE_BROWSER_H__
#define __GUI_WIDGETS_PANEL_FILE_BROWSER_H__

#include "io/file_descriptor.h"
#include "utils/types.h"

#include "gtk_wrapper.h"

TYPEDEF_STRUCT_UNDERSCORED (FileAuditionerControlsWidget);
TYPEDEF_STRUCT_UNDERSCORED (FileBrowserFiltersWidget);
struct FileBrowserLocation;
class ItemFactory;

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

using PanelFileBrowserWidget = struct _PanelFileBrowserWidget
{
  GtkWidget parent_instance;

  GtkPaned * paned;

  GtkBox * browser_top;
  GtkBox * browser_bot;

  GtkListView * bookmarks_list_view;
  std::unique_ptr<ItemFactory> * bookmarks_item_factory;

  GtkLabel * file_info;
  FileType   selected_type;

  GtkSearchEntry * file_search_entry;

  GtkCustomFilter *    files_filter;
  GtkFilterListModel * files_filter_model;
  GtkSingleSelection * files_selection_model;
  std::unique_ptr<ItemFactory> * files_item_factory;
  GtkListView *        files_list_view;

  /** Array of FileDescriptor. */
  // GPtrArray * selected_locations;
  std::vector<FileDescriptor *> * selected_files;

  FileBrowserFiltersWidget * filters_toolbar;

  FileAuditionerControlsWidget * auditioner_controls;

  /** Temp. */
  const FileDescriptor * cur_file;

  bool first_draw;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
};

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
