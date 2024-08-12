// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Port connections tree.
 */

#ifndef __GUI_WIDGETS_PORT_CONNECTIONS_TREE_H__
#define __GUI_WIDGETS_PORT_CONNECTIONS_TREE_H__

#include "gtk_wrapper.h"

class Port;

#define MW_PORT_CONNECTIONS_TREE (MW_PORT_CONNECTIONS->bindings_tree)

#define PORT_CONNECTIONS_TREE_WIDGET_TYPE \
  (port_connections_tree_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionsTreeWidget,
  port_connections_tree_widget,
  Z,
  PORT_CONNECTIONS_TREE_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _PortConnectionsTreeWidget
{
  GtkBox parent_instance;

  GtkScrolledWindow * scroll;

  /* The tree views */
  GtkTreeView *  tree;
  GtkTreeModel * tree_model;

  /** Temporary storage. */
  Port * src_port;
  Port * dest_port;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} PortConnectionsTreeWidget;

/**
 * Refreshes the tree model.
 */
void
port_connections_tree_widget_refresh (PortConnectionsTreeWidget * self);

/**
 * Instantiates a new PortConnectionsTreeWidget.
 */
PortConnectionsTreeWidget *
port_connections_tree_widget_new (void);

/**
 * @}
 */

#endif
