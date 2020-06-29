/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/port_connection_row.h"
#include "gui/widgets/inspector_port.h"
#include "gui/widgets/port_connections_popover.h"
#include "gui/widgets/port_selector_popover.h"
#include "plugins/plugin.h"
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
      self, self->owner->port);
  gtk_popover_set_relative_to (
    GTK_POPOVER (psp), GTK_WIDGET (btn));
  gtk_popover_set_position (
    GTK_POPOVER (psp), GTK_POS_BOTTOM);
  gtk_widget_show_all (GTK_WIDGET (psp));
}

void
port_connections_popover_widget_refresh (
  PortConnectionsPopoverWidget * self)
{
  InspectorPortWidget * owner =
    self->owner;
  g_return_if_fail (
    Z_IS_INSPECTOR_PORT_WIDGET (self->owner));

  z_gtk_container_destroy_all_children (
    GTK_CONTAINER (self->ports_box));

  /* set title and add ports */
  Port * port;
  PortConnectionRowWidget * pcr;
  /*if (owner->port->identifier.owner_type ==*/
        /*PORT_OWNER_TYPE_PLUGIN)*/
    /*{*/
      if (owner->port->id.flow == FLOW_INPUT)
        {
          if (GTK_IS_LABEL (self->title))
            {
              gtk_label_set_text (
                self->title, _("INPUTS"));
            }

          for (int i = 0;
               i < owner->port->num_srcs; i++)
            {
              port = owner->port->srcs[i];

              if (!port_is_connection_locked (
                     port, owner->port))
                {
                  pcr =
                    port_connection_row_widget_new (
                      self, port, owner->port, 1);
                  gtk_container_add (
                    GTK_CONTAINER (self->ports_box),
                    GTK_WIDGET (pcr));
                }
            }
        }
      else if (owner->port->id.flow ==
                 FLOW_OUTPUT)
        {
          if (GTK_IS_LABEL (self->title))
            {
              gtk_label_set_text (
                self->title, _("OUTPUTS"));
            }

          for (int i = 0;
               i < owner->port->num_dests; i++)
            {
              port = owner->port->dests[i];

              if (!port_is_connection_locked (
                     owner->port, port))
                {
                  pcr =
                    port_connection_row_widget_new (
                      self, owner->port, port, 0);
                  gtk_container_add (
                    GTK_CONTAINER (self->ports_box),
                    GTK_WIDGET (pcr));
                }
            }
        }
    /*}*/
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
PortConnectionsPopoverWidget *
port_connections_popover_widget_new (
  InspectorPortWidget * owner)
{
  PortConnectionsPopoverWidget * self =
    g_object_new (
      PORT_CONNECTIONS_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;
  gtk_popover_set_relative_to (
    GTK_POPOVER (self),
    GTK_WIDGET (owner));

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
  gtk_widget_set_visible (
    GTK_WIDGET (self->add), 1);
  GtkWidget * btn_box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_visible (btn_box, 1);
  GtkWidget * img =
    gtk_image_new_from_resource ("");
  resources_set_image_icon (
    GTK_IMAGE (img), ICON_TYPE_ZRYTHM, "plus.svg");
  gtk_widget_set_visible (img, 1);
  gtk_box_pack_start (
    GTK_BOX (btn_box), img,
    0, 0, 0);
  GtkWidget * lbl =
    gtk_label_new (_("Add"));
  gtk_widget_set_visible (lbl, 1);
  gtk_box_pack_end (
    GTK_BOX (btn_box), lbl,
    1, 1, 0);
  gtk_container_add (
    GTK_CONTAINER (self->add),
    btn_box);
  g_signal_connect (
    G_OBJECT (self->add), "clicked",
    G_CALLBACK (on_add_clicked), self);

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
  /*g_object_ref (self);*/
}
