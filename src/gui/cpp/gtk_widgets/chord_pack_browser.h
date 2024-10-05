// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_CHORD_PACK_BROWSER_H__
#define __GUI_WIDGETS_CHORD_PACK_BROWSER_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/utils/types.h"

class ChordPreset;
class ChordPresetPack;
TYPEDEF_STRUCT_UNDERSCORED (FileAuditionerControlsWidget);
class ItemFactory;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CHORD_PACK_BROWSER_WIDGET_TYPE (chord_pack_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordPackBrowserWidget,
  chord_pack_browser_widget,
  Z,
  CHORD_PACK_BROWSER_WIDGET,
  GtkBox)

#define MW_CHORD_PACK_BROWSER MW_RIGHT_DOCK_EDGE->chord_pack_browser

using ChordPackBrowserWidget = struct _ChordPackBrowserWidget
{
  GtkBox parent_instance;

  GtkPaned * paned;

  GtkBox * browser_top;
  GtkBox * browser_bot;

  GtkSingleSelection *           packs_selection_model;
  std::unique_ptr<ItemFactory> * packs_item_factory;
  GtkListView *                  packs_list_view;

  GtkLabel * pset_info;

  GtkCustomFilter *              psets_filter;
  GtkFilterListModel *           psets_filter_model;
  GtkSingleSelection *           psets_selection_model;
  std::unique_ptr<ItemFactory> * psets_item_factory;
  GtkListView *                  psets_list_view;

  std::vector<ChordPresetPack *> * selected_packs;
  std::vector<ChordPreset *> *     selected_psets;

  FileAuditionerControlsWidget * auditioner_controls;

  /** Current preset. */
  const ChordPresetPack * cur_pack;
  const ChordPreset *     cur_pset;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
};

void
chord_pack_browser_widget_refresh_packs (ChordPackBrowserWidget * self);

void
chord_pack_browser_widget_refresh_presets (ChordPackBrowserWidget * self);

ChordPackBrowserWidget *
chord_pack_browser_widget_new ();

/**
 * @}
 */

#endif
