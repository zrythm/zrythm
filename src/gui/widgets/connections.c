/*
 * gui/widgets/connections.c - Manages plugins
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "gui/widgets/connections.h"

G_DEFINE_TYPE (ConnectionsWidget, connections_widget, GTK_TYPE_GRID)

ConnectionsWidget *
connections_widget_new ()
{
  ConnectionsWidget * self = g_object_new (CONNECTIONS_WIDGET_TYPE, NULL);

  return self;
}


static void
connections_widget_class_init (ConnectionsWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
                        GTK_WIDGET_CLASS (klass),
                        "/online/alextee/zrythm/ui/connections.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        ConnectionsWidget,
                                        select_src_combobox);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        ConnectionsWidget,
                                        select_dest_combobox);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        ConnectionsWidget,
                                        src_plugin_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        ConnectionsWidget,
                                        dest_plugin_viewport);
}

static void
connections_widget_init (ConnectionsWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

