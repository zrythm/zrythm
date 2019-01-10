/*
 * gui/widgets/browser.h - The plugin, etc., browser on the right
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

#ifndef __GUI_WIDGETS_BROWSER_H__
#define __GUI_WIDGETS_BROWSER_H__

#include <gtk/gtk.h>

#define BROWSER_WIDGET_TYPE \
  (browser_widget_get_type ())
G_DECLARE_FINAL_TYPE (BrowserWidget,
                      browser_widget,
                      Z,
                      BROWSER_WIDGET,
                      GtkPaned)

#define MW_BROWSER MW_RIGHT_DOCK_EDGE->browser

typedef struct _BrowserWidget
{
  GtkPaned                 parent_instance;
  GtkGrid                  * browser_top;
  GtkSearchEntry           * browser_search;
  GtkExpander              * collections_exp;
  GtkExpander              * types_exp;
  GtkExpander              * cat_exp;
  GtkBox                   * browser_bot;
  GtkLabel                 * plugin_info;
  const char *             selected_category; ///< selected category
  GtkTreeModel             * category_tree_model;
  GtkTreeModelFilter       * plugins_tree_model;
  GtkTreeView *            plugins_tree_view;
  GtkScrolledWindow *      plugin_scroll_window;
} BrowserWidget;

BrowserWidget *
browser_widget_new ();

#endif
