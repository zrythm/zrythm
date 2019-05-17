/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automatable.h"
#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/channel_track.h"
#include "gui/widgets/port_connection_row.h"
#include "gui/widgets/port_connections_button.h"
#include "gui/widgets/port_connections_popover.h"
#include "gui/widgets/automation_lane.h"
#include "plugins/plugin.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (PortConnectionsPopoverWidget,
               port_connections_popover_widget,
               GTK_TYPE_POPOVER)

/**
 * Creates a digital meter with the given type (bpm or position).
 */
PortConnectionsPopoverWidget *
port_connections_popover_widget_new (
  PortConnectionsButtonWidget * owner)
{
  PortConnectionsPopoverWidget * self =
    g_object_new (
      PORT_CONNECTIONS_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;

  /* set title and add ports */
  Port * port;
  PortConnectionRowWidget * pcr;
  if (owner->port->identifier.owner_type ==
        PORT_OWNER_TYPE_PLUGIN)
    {
      if (owner->port->identifier.flow == FLOW_INPUT)
        {
          gtk_label_set_text (
            self->title, _("INPUTS"));

          for (int i = 0;
               i < owner->port->num_srcs; i++)
            {
              port = owner->port->srcs[i];

              pcr =
                port_connection_row_widget_new (
                  port, owner->port, 1);
              gtk_container_add (
                GTK_CONTAINER (self->ports_box),
                GTK_WIDGET (pcr));
            }
        }
      else if (owner->port->identifier.flow ==
                 FLOW_OUTPUT)
        {
          gtk_label_set_text (
            self->title, _("OUTPUTS"));

          for (int i = 0;
               i < owner->port->num_dests; i++)
            {
              port = owner->port->dests[i];

              pcr =
                port_connection_row_widget_new (
                  owner->port, port, 0);
              gtk_container_add (
                GTK_CONTAINER (self->ports_box),
                GTK_WIDGET (pcr));
            }
        }
    }

  return self;
}

static void
port_connections_popover_widget_class_init (
  PortConnectionsPopoverWidgetClass * _klass)
{
}

static void
port_connections_popover_widget_init (
  PortConnectionsPopoverWidget * self)
{
  /* create all */
  self->main_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_visible (
    GTK_WIDGET (self->main_box), 1);
  self->title =
    GTK_LABEL (gtk_label_new (""));
  gtk_widget_set_visible (
    GTK_WIDGET (self->title), 1);
  self->ports_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_visible (
    GTK_WIDGET (self->ports_box), 1);
  self->add =
    GTK_BUTTON (gtk_button_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->add), 1);
  GtkWidget * separator =
    gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_visible (separator, 1);

  /* add to each other */
  gtk_box_pack_start (
    self->main_box,
    GTK_WIDGET (self->title),
    0, // expand
    0, // fill
    0); // extra padding
  gtk_box_pack_start (
    self->main_box,
    separator,
    0, // expand
    0, // fill
    0); // extra padding
  gtk_box_pack_start (
    self->main_box,
    GTK_WIDGET (self->ports_box),
    0, // expand
    0, // fill
    0); // extra padding
  gtk_box_pack_start (
    self->main_box,
    GTK_WIDGET (self->add),
    0, // expand
    0, // fill
    0); // extra padding

  /* add to popover */
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->main_box));
}
