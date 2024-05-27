/*
 * SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __GUI_WIDGETS_CHORD_PACK_BROWSER_H__
#define __GUI_WIDGETS_CHORD_PACK_BROWSER_H__

#include "gtk_wrapper.h"

typedef struct ChordPreset                   ChordPreset;
typedef struct ChordPresetPack               ChordPresetPack;
typedef struct _FileAuditionerControlsWidget FileAuditionerControlsWidget;
typedef struct ItemFactory                   ItemFactory;

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

typedef struct _ChordPackBrowserWidget
{
  GtkBox parent_instance;

  GtkPaned * paned;

  GtkBox * browser_top;
  GtkBox * browser_bot;

  GtkSingleSelection * packs_selection_model;
  ItemFactory *        packs_item_factory;
  GtkListView *        packs_list_view;

  GtkLabel * pset_info;

  GtkCustomFilter *    psets_filter;
  GtkFilterListModel * psets_filter_model;
  GtkSingleSelection * psets_selection_model;
  ItemFactory *        psets_item_factory;
  GtkListView *        psets_list_view;

  /** Array of ChordPreset. */
  GPtrArray * selected_packs;
  GPtrArray * selected_psets;

  FileAuditionerControlsWidget * auditioner_controls;

  /** Current preset. */
  const ChordPresetPack * cur_pack;
  const ChordPreset *     cur_pset;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} ChordPackBrowserWidget;

void
chord_pack_browser_widget_refresh_packs (ChordPackBrowserWidget * self);

void
chord_pack_browser_widget_refresh_presets (ChordPackBrowserWidget * self);

ChordPackBrowserWidget *
chord_pack_browser_widget_new (void);

/**
 * @}
 */

#endif
