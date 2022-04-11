/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/port_connections.h"
#include "gui/widgets/port_connections_tree.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  PortConnectionsWidget,
  port_connections_widget,
  GTK_TYPE_BOX)

/**
 * Refreshes the port_connections widget.
 */
void
port_connections_widget_refresh (
  PortConnectionsWidget * self)
{
  port_connections_tree_widget_refresh (
    self->bindings_tree);
}

PortConnectionsWidget *
port_connections_widget_new ()
{
  PortConnectionsWidget * self = g_object_new (
    PORT_CONNECTIONS_WIDGET_TYPE, NULL);

  self->bindings_tree =
    port_connections_tree_widget_new ();
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->bindings_tree));
  gtk_widget_set_vexpand (
    GTK_WIDGET (self->bindings_tree), true);

  return self;
}

static void
port_connections_widget_class_init (
  PortConnectionsWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "port-connections");
}

static void
port_connections_widget_init (
  PortConnectionsWidget * self)
{
}
