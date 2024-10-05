/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * CC Bindings matrix.
 */

#ifndef __GUI_WIDGETS_PORT_CONNECTIONS_H__
#define __GUI_WIDGETS_PORT_CONNECTIONS_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

typedef struct _PortConnectionsTreeWidget PortConnectionsTreeWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define PORT_CONNECTIONS_WIDGET_TYPE (port_connections_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionsWidget,
  port_connections_widget,
  Z,
  PORT_CONNECTIONS_WIDGET,
  GtkBox)

#define MW_PORT_CONNECTIONS (MW_MAIN_NOTEBOOK->port_connections)

/**
 * Left dock widget.
 */
typedef struct _PortConnectionsWidget
{
  GtkBox parent_instance;

  PortConnectionsTreeWidget * bindings_tree;
} PortConnectionsWidget;

PortConnectionsWidget *
port_connections_widget_new (void);

void
port_connections_widget_refresh (PortConnectionsWidget * self);

/**
 * @}
 */

#endif
