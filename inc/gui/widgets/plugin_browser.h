// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
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

#define PLUGIN_BROWSER_WIDGET_TYPE (plugin_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PluginBrowserWidget,
  plugin_browser_widget,
  Z,
  PLUGIN_BROWSER_WIDGET,
  GtkWidget);

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
 * The plugin browser allows to browse and filter available
 * Plugin's on the system.
 *
 * It contains references to PluginDescriptor's, which it uses
 * to initialize Plugin's on row activation or drag-n-drop.
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

  /* The stack pages for each list view */
  GtkBox *            collection_box;
  GtkBox *            author_box;
  GtkBox *            category_box;
  GtkBox *            protocol_box;
  GtkScrolledWindow * plugin_scroll;

  /* The tree views */
  GtkListView * collection_list_view;
  GtkListView * author_list_view;
  GtkListView * category_list_view;
  GtkListView * protocol_list_view;

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
  GtkLabel * plugin_author_label;
  GtkLabel * plugin_type_label;
  GtkLabel * plugin_audio_label;
  GtkLabel * plugin_midi_label;
  GtkLabel * plugin_ctrl_label;
  GtkLabel * plugin_cv_label;

  /** Symbol IDs (for quick comparison) of selected authors. */
  GArray * selected_authors;

  /** Selected categories (ZPluginCategory). */
  GArray * selected_categories;

  /** Selected protocols (ZPluginProtocol). */
  GArray * selected_protocols;

  /** Pointers to the collections (PluginCollection instances)
   * from PluginManager.collections that must not be free'd. */
  GPtrArray * selected_collections;

  /** List view -> selection model -> filter model */
  GtkCustomFilter *    plugin_filter;
  GtkFilterListModel * plugin_filter_model;
  GtkCustomSorter *    plugin_sorter;
  GtkSortListModel *   plugin_sort_model;

  /** Array of ItemFactory. */
  GPtrArray * item_factories;

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
plugin_browser_widget_refresh_collections (PluginBrowserWidget * self);

/**
 * @}
 */

#endif
