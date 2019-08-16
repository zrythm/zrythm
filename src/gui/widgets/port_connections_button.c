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

#include "audio/port.h"
#include "gui/widgets/port_connections_button.h"
#include "gui/widgets/port_connections_popover.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (PortConnectionsButtonWidget,
               port_connections_button_widget,
               GTK_TYPE_MENU_BUTTON)

PortConnectionsButtonWidget *
port_connections_button_widget_new (
  Port * port)
{
  PortConnectionsButtonWidget * self =
    g_object_new (
      PORT_CONNECTIONS_BUTTON_WIDGET_TYPE,
      NULL);

  self->port = port;

  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self),
    GTK_WIDGET (port_connections_popover_widget_new (
                  self)));
  gtk_menu_button_set_direction (
    GTK_MENU_BUTTON (self),
    GTK_ARROW_RIGHT);

  if (!port->identifier.label)
    goto port_connections_button_new_end;

  char * str = NULL;
  if (port->identifier.owner_type ==
        PORT_OWNER_TYPE_PLUGIN ||
      port->identifier.owner_type ==
        PORT_OWNER_TYPE_FADER ||
      port->identifier.owner_type ==
        PORT_OWNER_TYPE_PREFADER)
    {
      if (port->identifier.flow == FLOW_INPUT)
        {
          str =
            g_strdup_printf (
              "%s (%d)",
              port->identifier.label,
              port->num_srcs);
        }
      else if (port->identifier.flow == FLOW_OUTPUT)
        {
          str =
            g_strdup_printf (
              "%s (%d)",
              port->identifier.label,
              port->num_dests);
        }
    }

  if (str)
    {
      gtk_button_set_label (
        GTK_BUTTON (self), str);
      g_free (str);
    }

port_connections_button_new_end:

  return self;
}

static void
port_connections_button_widget_class_init (
  PortConnectionsButtonWidgetClass * klass)
{
}

static void
port_connections_button_widget_init (
  PortConnectionsButtonWidget * self)
{
  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);
}
