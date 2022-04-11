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

#include "actions/port_connection_action.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "audio/port.h"
#include "audio/port_connection.h"
#include "audio/port_connections_manager.h"
#include "gui/widgets/bar_slider.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/port_connection_row.h"
#include "gui/widgets/port_connections_popover.h"
#include "project.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/objects.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  PortConnectionRowWidget,
  port_connection_row_widget,
  GTK_TYPE_BOX)

static void
on_enable_toggled (
  GtkToggleButton *         btn,
  PortConnectionRowWidget * self)
{
  GError * err = NULL;
  bool ret = port_connection_action_perform_enable (
    self->connection->src_id,
    self->connection->dest_id,
    gtk_toggle_button_get_active (btn), &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _ ("Failed to enable connection"));
    }

  g_return_if_fail (
    IS_PORT_AND_NONNULL (self->parent->port));
  port_connections_popover_widget_refresh (
    self->parent, self->parent->port);
}

static void
on_del_clicked (
  GtkButton *               btn,
  PortConnectionRowWidget * self)
{
  GError * err = NULL;
  bool     ret =
    port_connection_action_perform_disconnect (
      self->connection->src_id,
      self->connection->dest_id, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _ ("Failed to disconnect"));
    }

  g_return_if_fail (
    IS_PORT_AND_NONNULL (self->parent->port));
  port_connections_popover_widget_refresh (
    self->parent, self->parent->port);
}

static void
finalize (PortConnectionRowWidget * self)
{
  object_free_w_func_and_null (
    port_connection_free, self->connection);

  G_OBJECT_CLASS (
    port_connection_row_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

/**
 * Creates the popover.
 */
PortConnectionRowWidget *
port_connection_row_widget_new (
  PortConnectionsPopoverWidget * parent,
  const PortConnection *         connection,
  bool                           is_input)
{
  PortConnectionRowWidget * self = g_object_new (
    PORT_CONNECTION_ROW_WIDGET_TYPE, NULL);

  self->connection =
    port_connection_clone (connection);

  self->is_input = is_input;
  self->parent = parent;

  /* create the widgets and pack */
  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_visible (box, 1);

  /* power button */
  GtkToggleButton * btn =
    z_gtk_toggle_button_new_with_icon (
      "network-connect");
  gtk_toggle_button_set_active (
    btn, connection->enabled);
  gtk_widget_set_visible (GTK_WIDGET (btn), 1);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (btn));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (btn),
    _ ("Enable/disable connection"));
  g_signal_connect (
    G_OBJECT (btn), "toggled",
    G_CALLBACK (on_enable_toggled), self);

  /* create overlay */
  self->overlay = GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->overlay), 1);
  gtk_box_append (
    GTK_BOX (box), GTK_WIDGET (self->overlay));

  /* bar slider */
  char                   designation[600];
  const PortIdentifier * port_id =
    is_input
      ? connection->dest_id
      : connection->src_id;
  Port * port = port_find_from_identifier (port_id);
  if (!IS_PORT_AND_NONNULL (port))
    {
      g_critical (
        "failed to find port for '%s'",
        port_id->label);
      return NULL;
    }
  port_get_full_designation (port, designation);
  strcat (designation, " ");
  self->slider =
    bar_slider_widget_new_port_connection (
      connection, designation);
  gtk_overlay_set_child (
    GTK_OVERLAY (self->overlay),
    GTK_WIDGET (self->slider));

  /* delete connection button */
  self->delete_btn = GTK_BUTTON (
    gtk_button_new_from_icon_name ("edit-delete"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->delete_btn), 1);
  gtk_box_append (
    GTK_BOX (box), GTK_WIDGET (self->delete_btn));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->delete_btn),
    _ ("Delete connection"));
  g_signal_connect (
    G_OBJECT (self->delete_btn), "clicked",
    G_CALLBACK (on_del_clicked), self);

  gtk_box_append (GTK_BOX (self), box);

  gtk_widget_set_sensitive (
    box, !connection->locked);

  return self;
}

static void
port_connection_row_widget_class_init (
  PortConnectionRowWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
port_connection_row_widget_init (
  PortConnectionRowWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), 1);
}
