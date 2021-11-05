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

#include "audio/automation_track.h"
#include "audio/channel_track.h"
#include "audio/port_connections_manager.h"
#include "gui/widgets/port_connection_row.h"
#include "gui/widgets/inspector_port.h"
#include "gui/widgets/port_connections_popover.h"
#include "gui/widgets/port_selector_popover.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PortConnectionsPopoverWidget,
  port_connections_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_add_clicked (
  GtkButton *                    btn,
  PortConnectionsPopoverWidget * self)
{
  PortSelectorPopoverWidget * psp =
    port_selector_popover_widget_new (
      self, self->port);
  /*gtk_popover_set_relative_to (*/
    /*GTK_POPOVER (psp), GTK_WIDGET (btn));*/
  gtk_popover_set_position (
    GTK_POPOVER (psp), GTK_POS_RIGHT);
  gtk_popover_present (GTK_POPOVER (psp));
}

void
port_connections_popover_widget_refresh (
  PortConnectionsPopoverWidget * self)
{
  z_gtk_widget_destroy_all_children (
    GTK_WIDGET (self->ports_box));

  /* set title and add ports */
  if (self->port->id.flow == FLOW_INPUT)
    {
      if (GTK_IS_LABEL (self->title))
        {
          gtk_label_set_text (
            self->title, _("INPUTS"));
        }

      GPtrArray * srcs = g_ptr_array_new ();
      int num_srcs =
        port_connections_manager_get_sources_or_dests (
          PORT_CONNECTIONS_MGR, srcs,
          &self->port->id, true);
      for (int i = 0; num_srcs; i++)
        {
          PortConnection * conn =
            g_ptr_array_index (srcs, i);
          if (!conn->locked)
            {
              PortConnectionRowWidget * pcr =
                port_connection_row_widget_new (
                  self, conn, true);
              gtk_box_append (
                GTK_BOX (self->ports_box),
                GTK_WIDGET (pcr));
            }
        }
      g_ptr_array_unref (srcs);
    }
  else if (self->port->id.flow ==
             FLOW_OUTPUT)
    {
      if (GTK_IS_LABEL (self->title))
        {
          gtk_label_set_text (
            self->title, _("OUTPUTS"));
        }

      GPtrArray * dests = g_ptr_array_new ();
      int num_dests =
        port_connections_manager_get_sources_or_dests (
          PORT_CONNECTIONS_MGR, dests,
          &self->port->id, false);
      for (int i = 0; i < num_dests; i++)
        {
          PortConnection * conn =
            g_ptr_array_index (dests, i);
          if (!conn->locked)
            {
              PortConnectionRowWidget * pcr =
                port_connection_row_widget_new (
                  self, conn, false);
              gtk_box_append (
                GTK_BOX (self->ports_box),
                GTK_WIDGET (pcr));
            }
        }
      g_ptr_array_unref (dests);
    }
}

/**
 * Creates the popover.
 *
 * @param owner Owner widget to pop up at.
 * @param port Owner port.
 */
PortConnectionsPopoverWidget *
port_connections_popover_widget_new (
  GtkWidget * owner,
  Port *      port)
{
  g_return_val_if_fail (
    GTK_IS_WIDGET (owner) && IS_PORT (port), NULL);

  PortConnectionsPopoverWidget * self =
    g_object_new (
      PORT_CONNECTIONS_POPOVER_WIDGET_TYPE, NULL);

  self->port = port;
  /*gtk_popover_set_relative_to (*/
    /*GTK_POPOVER (self),*/
    /*GTK_WIDGET (owner));*/

  port_connections_popover_widget_refresh (self);

  return self;
}

static void
finalize (
  PortConnectionsPopoverWidget * self)
{
  G_OBJECT_CLASS (
    port_connections_popover_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
port_connections_popover_widget_class_init (
  PortConnectionsPopoverWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
port_connections_popover_widget_init (
  PortConnectionsPopoverWidget * self)
{
  /* create all */
  self->main_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 2));
  gtk_widget_set_visible (
    GTK_WIDGET (self->main_box), 1);
  self->title =
    GTK_LABEL (gtk_label_new (""));
  gtk_widget_set_visible (
    GTK_WIDGET (self->title), 1);
  self->ports_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 1));
  gtk_widget_set_visible (
    GTK_WIDGET (self->ports_box), 1);

  self->add =
    GTK_BUTTON (gtk_button_new ());
  GtkWidget * btn_box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget * img =
    gtk_image_new_from_icon_name ("add");
  gtk_box_append (GTK_BOX (btn_box), img);
  GtkWidget * lbl =
    gtk_label_new (_("Add"));
  gtk_widget_set_visible (lbl, 1);
  gtk_box_append (GTK_BOX (btn_box), lbl);
  gtk_button_set_child (
    GTK_BUTTON (self->add), btn_box);
  g_signal_connect (
    G_OBJECT (self->add), "clicked",
    G_CALLBACK (on_add_clicked), self);

  GtkWidget * separator =
    gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_visible (separator, 1);

  /* add to each other */
  gtk_box_append (
    self->main_box, GTK_WIDGET (self->title));
  gtk_box_append (
    self->main_box, separator);
  gtk_box_append (
    self->main_box, GTK_WIDGET (self->ports_box));
  gtk_box_append (
    self->main_box, GTK_WIDGET (self->add));

  /* add to popover */
  gtk_popover_set_child (
    GTK_POPOVER (self),
    GTK_WIDGET (self->main_box));
  /*g_object_ref (self);*/
}
