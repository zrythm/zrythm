/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * CC Bindings tree.
 */

#ifndef __GUI_WIDGETS_CC_BINDINGS_TREE_H__
#define __GUI_WIDGETS_CC_BINDINGS_TREE_H__

#include <gtk/gtk.h>

#define CC_BINDINGS_TREE_WIDGET_TYPE \
  (cc_bindings_tree_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CcBindingsTreeWidget,
  cc_bindings_tree_widget,
  Z,
  CC_BINDINGS_TREE_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _CcBindingsTreeWidget
{
  GtkBox parent_instance;

  GtkScrolledWindow * scroll;

  /* The column view */
  GtkColumnView * column_view;

  /** Array of ItemFactory pointers for each
   * column. */
  GPtrArray * item_factories;
} CcBindingsTreeWidget;

/**
 * Refreshes the tree model.
 */
void
cc_bindings_tree_widget_refresh (
  CcBindingsTreeWidget * self);

/**
 * Instantiates a new CcBindingsTreeWidget.
 */
CcBindingsTreeWidget *
cc_bindings_tree_widget_new (void);

/**
 * @}
 */

#endif
