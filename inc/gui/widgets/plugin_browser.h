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
 * Plugin browser.
 */

#ifndef __GUI_WIDGETS_PLUGIN_BROWSER_H__
#define __GUI_WIDGETS_PLUGIN_BROWSER_H__

#include "plugins/plugin.h"
#include "utils/symap.h"

#include <gtk/gtk.h>

#define PLUGIN_BROWSER_WIDGET_TYPE \
  (plugin_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PluginBrowserWidget,
  plugin_browser_widget,
  Z,
  PLUGIN_BROWSER_WIDGET,
  GtkBox)

typedef struct _ExpanderBoxWidget ExpanderBoxWidget;
typedef struct PluginCollection   PluginCollection;
typedef struct ItemFactory        ItemFactory;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PLUGIN_BROWSER \
  MW_RIGHT_DOCK_EDGE->plugin_browser

typedef enum
{
  PLUGIN_BROWSER_TAB_COLLECTION,
  PLUGIN_BROWSER_TAB_AUTHOR,
  PLUGIN_BROWSER_TAB_CATEGORY,
  PLUGIN_BROWSER_TAB_PROTOCOL,
} PluginBrowserTab;

typedef enum
{
  PLUGIN_BROWSER_FILTER_NONE,
  PLUGIN_BROWSER_FILTER_INSTRUMENT,
  PLUGIN_BROWSER_FILTER_EFFECT,
  PLUGIN_BROWSER_FILTER_MODULATOR,
  PLUGIN_BROWSER_FILTER_MIDI_EFFECT,
} PluginBrowserFilter;

/**
 * The plugin browser allows to browse and filter
 * available Plugin's on the system.
 *
 * It contains references to PluginDescriptor's,
 * which it uses to initialize Plugin's on row
 * activation or drag-n-drop.
 */
typedef struct _PluginBrowserWidget
{
  GtkBox parent_instance;

  GtkPaned * paned;

  /** The stack switcher. */
  AdwViewSwitcher * stack_switcher;
  GtkBox *          stack_switcher_box;

  /** The stack containing collection/category/
   * protocol. */
  AdwViewStack * stack;

  /* The scrolls for each tree view */
  GtkScrolledWindow * collection_scroll;
  GtkScrolledWindow * author_scroll;
  GtkScrolledWindow * category_scroll;
  GtkScrolledWindow * protocol_scroll;
  GtkScrolledWindow * plugin_scroll;

  /* The tree views */
  GtkTreeView * collection_tree_view;
  GtkTreeView * author_tree_view;
  GtkTreeView * category_tree_view;
  GtkTreeView * protocol_tree_view;

  GtkSearchEntry * plugin_search_entry;
  GtkListView *    plugin_list_view;

  /** Browser bot. */
  GtkBox * browser_bot;

  /* The toolbar to toggle visibility of
   * modulators/effects/instruments/midi modifiers */
  GtkBox * plugin_toolbar;

  GtkToggleButton * toggle_instruments;
  GtkToggleButton * toggle_effects;
  GtkToggleButton * toggle_modulators;
  GtkToggleButton * toggle_midi_modifiers;

  /** A label to show info about the currently
   * selected Plugin. */
  GtkLabel * plugin_info;

  /** Symbol IDs of selected authors. */
  uint32_t selected_authors[600];
  int      num_selected_authors;

  /** Selected categories. */
  ZPluginCategory selected_categories[600];
  int             num_selected_categories;

  /** Selected protocols. */
  PluginProtocol selected_protocols[60];
  int            num_selected_protocols;

  /** The selected collection. */
  PluginCollection * selected_collection;

  GtkTreeModel *     collection_tree_model;
  GtkTreeModel *     author_tree_model;
  GtkTreeModelSort * protocol_tree_model;
  GtkTreeModel *     category_tree_model;

  /** List view -> selection model -> filter model */
  GtkCustomFilter *    plugin_filter;
  GtkFilterListModel * plugin_filter_model;
  GtkSingleSelection * plugin_selection_model;
  ItemFactory *        plugin_item_factory;

  /**
   * The currently selected collections.
   *
   * Used temporarily when right-clicking on
   * collections.
   *
   * These are pointers to the actual collections and
   * must not be deleted.
   */
  PluginCollection ** current_collections;
  int                 num_current_collections;

  /**
   * The currently selected plugin descriptors.
   *
   * Used temporarily when right-clicking on
   * plugins.
   *
   * These are pointers to the actual descriptors and
   * must not be deleted.
   */
  PluginDescriptor ** current_descriptors;
  int                 num_current_descriptors;

  /**
   * A little hack to get the paned position to
   * get set from the gsettings when first created.
   *
   * Had problems where the position was quickly
   * overwritten by something random within 300 ms
   * of widget creation.
   */
  int    start_saving_pos;
  int    first_time_position_set;
  gint64 first_time_position_set_time;

  /** Current search string. */
  char * current_search;

  /** Symbol map for string interning. */
  Symap * symap;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} PluginBrowserWidget;

/**
 * Instantiates a new PluginBrowserWidget.
 */
PluginBrowserWidget *
plugin_browser_widget_new (void);

void
plugin_browser_widget_refresh_collections (
  PluginBrowserWidget * self);

/**
 * @}
 */

#endif
