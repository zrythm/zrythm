/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Port connections popover.
 */

#ifndef __GUI_WIDGETS_PORT_CONNECTIONS_POPOVER_H__
#define __GUI_WIDGETS_PORT_CONNECTIONS_POPOVER_H__

#include <gtk/gtk.h>

#define PORT_CONNECTIONS_POPOVER_WIDGET_TYPE \
  (port_connections_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionsPopoverWidget,
  port_connections_popover_widget,
  Z,
  PORT_CONNECTIONS_POPOVER_WIDGET,
  GtkPopover)

typedef struct Port                       Port;
typedef struct _PortSelectorPopoverWidget PortSelectorPopoverWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _PortConnectionsPopoverWidget
{
  GtkPopover parent_instance;

  /** The port. */
  Port * port;

  /** The main vertical box containing everything. */
  GtkBox * main_box;

  /** Title to show at the top, e.g. "INPUTS". */
  GtkLabel * title;

  /** Box containing the ports. */
  GtkBox * ports_box;

  /** Button to add connection. */
  GtkButton * add;

  // PortSelectorPopoverWidget * port_selector_popover;
} PortConnectionsPopoverWidget;

/**
 * Creates the popover.
 *
 * @param owner Owner widget to pop up at.
 * @param port Owner port.
 */
PortConnectionsPopoverWidget *
port_connections_popover_widget_new (GtkWidget * owner);

/**
 * Refreshes the popover.
 *
 * Removes all children of ports_box and readds them
 * based on the current connections.
 */
void
port_connections_popover_widget_refresh (
  PortConnectionsPopoverWidget * self,
  Port *                         port);

/**
 * @}
 */

#endif
