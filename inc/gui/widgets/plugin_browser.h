// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
  GtkWidget)

TYPEDEF_STRUCT_UNDERSCORED (ExpanderBoxWidget);
TYPEDEF_STRUCT (PluginCollection);
TYPEDEF_STRUCT (ItemFactory);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PLUGIN_BROWSER MW_RIGHT_DOCK_EDGE->plugin_browser

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

typedef enum
{
  PLUGIN_BROWSER_SORT_ALPHA,
  PLUGIN_BROWSER_SORT_LAST_USED,
  PLUGIN_BROWSER_SORT_MOST_USED,
} PluginBrowserSortStyle;

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
  GtkWidget parent_instance;

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

  GtkToggleButton * alpha_sort_btn;
  GtkToggleButton * last_used_sort_btn;
  GtkToggleButton * most_used_sort_btn;

  /** A label to show info about the currently
   * selected Plugin. */
  GtkLabel * plugin_info;

  /* FIXME use GPtrArray instead of fixed size
   * arrays */

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
  GtkCustomSorter *    plugin_sorter;
  GtkSortListModel *   plugin_sort_model;
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
  /* FIXME remove this, add a getter instead that gets the
   * selection from the widget directly via GTK API */
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
