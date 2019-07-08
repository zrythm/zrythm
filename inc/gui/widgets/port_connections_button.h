/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_PORT_CONNECTIONS_BUTTON_H__
#define __GUI_WIDGETS_PORT_CONNECTIONS_BUTTON_H__

#include "utils/resources.h"

#include <gtk/gtk.h>

#define PORT_CONNECTIONS_BUTTON_WIDGET_TYPE \
  (port_connections_button_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionsButtonWidget,
  port_connections_button_widget,
  Z,
  PORT_CONNECTIONS_BUTTON_WIDGET,
  GtkMenuButton)

/**
 * An expander box is a base widget with a button that
 * when clicked expands the contents.
 */
typedef struct _PortConnectionsButtonWidget
{
  GtkMenuButton  parent_instance;

  /** Port this is for. */
  Port *         port;

} PortConnectionsButtonWidget;

void
port_connections_button_widget_refresh (
  PortConnectionsButtonWidget * self);

PortConnectionsButtonWidget *
port_connections_button_widget_new (
  Port * port);

#endif
