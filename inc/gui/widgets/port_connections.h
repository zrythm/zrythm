/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * @file
 *
 * CC Bindings matrix.
 */

#ifndef __GUI_WIDGETS_PORT_CONNECTIONS_H__
#define __GUI_WIDGETS_PORT_CONNECTIONS_H__

#include <gtk/gtk.h>

typedef struct _PortConnectionsTreeWidget
  PortConnectionsTreeWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define PORT_CONNECTIONS_WIDGET_TYPE \
  (port_connections_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionsWidget,
  port_connections_widget,
  Z,
  PORT_CONNECTIONS_WIDGET,
  GtkBox)

#define MW_PORT_CONNECTIONS \
  (MW_MAIN_NOTEBOOK->port_connections)

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
