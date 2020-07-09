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

#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/port.h"
#include "gui/widgets/bar_slider.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/port_connection_row.h"
#include "gui/widgets/port_connections_popover.h"
#include "project.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PortConnectionRowWidget,
  port_connection_row_widget,
  GTK_TYPE_EVENT_BOX)

static void
on_enable_toggled (
  GtkToggleButton *         btn,
  PortConnectionRowWidget * self)
{
  int dest_idx =
    port_get_dest_index (self->src, self->dest);
  self->src->dest_enabled[dest_idx] =
    gtk_toggle_button_get_active (btn);
  port_connections_popover_widget_refresh (
    self->parent);
}

static void
on_del_clicked (
  GtkButton *               btn,
  PortConnectionRowWidget * self)
{
  UndoableAction * ua =
    port_connection_action_new (
      PORT_CONNECTION_DISCONNECT,
      &self->src->id, &self->dest->id);
  undo_manager_perform (UNDO_MANAGER, ua);

  port_connections_popover_widget_refresh (
    self->parent);
}

/**
 * Creates the popover.
 */
PortConnectionRowWidget *
port_connection_row_widget_new (
  PortConnectionsPopoverWidget * parent,
  Port * src,
  Port * dest,
  int    is_input)
{
  PortConnectionRowWidget * self =
    g_object_new (
      PORT_CONNECTION_ROW_WIDGET_TYPE, NULL);

  self->src = src;
  self->dest = dest;
  self->is_input = is_input;
  self->parent = parent;

  /* get connection locked/enabled */
  int dest_idx = port_get_dest_index (src, dest);
  int enabled = src->dest_enabled[dest_idx];
  int locked = src->dest_locked[dest_idx];

  /* create the widgets and pack */
  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_visible (box, 1);

  /* power button */
  GtkToggleButton * btn =
    z_gtk_toggle_button_new_with_icon (
      "network-connect");
  gtk_toggle_button_set_active (btn, enabled);
  gtk_widget_set_visible (GTK_WIDGET (btn), 1);
  gtk_box_pack_start (
    GTK_BOX (box), GTK_WIDGET (btn), 0, 0, 0);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (btn),
    _("Enable/disable connection"));
  g_signal_connect (
    G_OBJECT (btn), "toggled",
    G_CALLBACK (on_enable_toggled), self);

  /* create overlay */
  self->overlay =
    GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->overlay), 1);
  gtk_box_pack_end (
    GTK_BOX (box),
    GTK_WIDGET (self->overlay),
    1,1,0);

  /* bar slider */
  char designation[600];
  if (is_input)
    port_get_full_designation (src, designation);
  else
    port_get_full_designation (dest, designation);
  strcat (designation, " ");
  self->slider =
    bar_slider_widget_new_port (
      src, dest, designation);
  gtk_container_add (
    GTK_CONTAINER (self->overlay),
    GTK_WIDGET (self->slider));

  /* delete connection button */
  self->delete_btn =
    GTK_BUTTON (
      gtk_button_new_from_icon_name (
        "edit-delete", GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_visible (
    GTK_WIDGET (self->delete_btn), 1);
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->delete_btn));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->delete_btn),
    _("Delete connection"));
  g_signal_connect (
    G_OBJECT (self->delete_btn), "clicked",
    G_CALLBACK (on_del_clicked), self);

  gtk_container_add (GTK_CONTAINER (self), box);

  gtk_widget_set_sensitive (box, !locked);

  return self;
}

static void
port_connection_row_widget_class_init (
  PortConnectionRowWidgetClass * _klass)
{
}

static void
port_connection_row_widget_init (
  PortConnectionRowWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), 1);
}
