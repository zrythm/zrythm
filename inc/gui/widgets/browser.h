/*
 * gui/widgets/browser.h - The plugin, etc., browser on the right
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#define BROWSER_WIDGET_TYPE                  (browser_widget_get_type ())
#define BROWSER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BROWSER_WIDGET_TYPE, BrowserWidget))
#define BROWSER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), BROWSER_WIDGET, BrowserWidgetClass))
#define IS_BROWSER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BROWSER_WIDGET_TYPE))
#define IS_BROWSER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), BROWSER_WIDGET_TYPE))
#define BROWSER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), BROWSER_WIDGET_TYPE, BrowserWidgetClass))

typedef struct BrowserWidget
{
  GtkPaned                 parent_instance;
  GtkGrid                  * browser_top;
  GtkSearchEntry           * browser_search;
  GtkExpander              * collections_exp;
  GtkExpander              * types_exp;
  GtkExpander              * cat_exp;
  GtkBox                   * browser_bot;
  GtkLabel                 * plugin_info;
  GtkTreeModel             * plugins_tree_model;
} BrowserWidget;

typedef struct BrowserWidgetClass
{
  GtkPanedClass               parent_class;
} BrowserWidgetClass;

BrowserWidget *
browser_widget_new ();

#endif
