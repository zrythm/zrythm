/*
 * SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
