/*
 * gui/widgets/plugin_browser.h - The plugin, etc., plugin_browser on the right
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

/**
 * \file
 *
 * Plugin browser.
 */

#ifndef __GUI_WIDGETS_PLUGIN_BROWSER_H__
#define __GUI_WIDGETS_PLUGIN_BROWSER_H__

#include <gtk/gtk.h>

#define PLUGIN_BROWSER_WIDGET_TYPE \
  (plugin_browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PluginBrowserWidget,
  plugin_browser_widget,
  Z, PLUGIN_BROWSER_WIDGET,
  GtkPaned)

typedef struct _ExpanderBoxWidget ExpanderBoxWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PLUGIN_BROWSER \
  MW_RIGHT_DOCK_EDGE->plugin_browser

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
  GtkPaned             parent_instance;

  /** The stack switcher. */
  GtkStackSwitcher *   stack_switcher;
  GtkBox *             stack_switcher_box;

  /** The stack containing collection/category/
   * protocol. */
  GtkStack *           stack;

  /* The scrolls for each tree view */
  GtkScrolledWindow *  collection_scroll;
  GtkScrolledWindow *  category_scroll;
  GtkScrolledWindow *  protocol_scroll;
  GtkScrolledWindow *  plugin_scroll;

  /* The tree views */
  GtkTreeView *        collection_tree_view;
  GtkTreeView *        category_tree_view;
  GtkTreeView *        protocol_tree_view;
  GtkTreeView *        plugin_tree_view;

  /** Browser bot. */
  GtkBox *             browser_bot;

  /* The toolbar to toggle visibility of
   * modulators/effects/instruments/midi modifiers */
  GtkToolbar *         plugin_toolbar;

  GtkToggleToolButton * toggle_instruments;
  GtkToggleToolButton * toggle_effects;
  GtkToggleToolButton * toggle_modulators;
  GtkToggleToolButton * toggle_midi_modifiers;

  /** A label to show info about the currently
   * selected Plugin. */
  GtkLabel *           plugin_info;

  /** The selected category. */
  const char *         selected_category;

  GtkTreeModel *       collection_tree_model;
  GtkTreeModel *       protocol_tree_model;
  GtkTreeModel *       category_tree_model;
  GtkTreeModelFilter * plugin_tree_model;
} PluginBrowserWidget;

/**
 * Instantiates a new PluginBrowserWidget.
 */
PluginBrowserWidget *
plugin_browser_widget_new ();

/**
 * @}
 */

#endif
