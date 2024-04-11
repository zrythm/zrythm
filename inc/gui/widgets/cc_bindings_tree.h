// clang-format off
// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * CC Bindings tree.
 */

#ifndef __GUI_WIDGETS_CC_BINDINGS_TREE_H__
#define __GUI_WIDGETS_CC_BINDINGS_TREE_H__

#include <gtk/gtk.h>

#define CC_BINDINGS_TREE_WIDGET_TYPE (cc_bindings_tree_widget_get_type ())
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

  /** Array of ItemFactory pointers for each column. */
  GPtrArray * item_factories;

  GtkBox *    toolbar;
  GtkButton * delete_btn;
} CcBindingsTreeWidget;

/**
 * Refreshes the tree model.
 */
void
cc_bindings_tree_widget_refresh (CcBindingsTreeWidget * self);

/**
 * Instantiates a new CcBindingsTreeWidget.
 */
CcBindingsTreeWidget *
cc_bindings_tree_widget_new (void);

/**
 * @}
 */

#endif
