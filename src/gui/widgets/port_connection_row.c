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

#include "audio/port.h"
#include "gui/widgets/port_connection_row.h"
#include "gui/widgets/knob.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (PortConnectionRowWidget,
               port_connection_row_widget,
               GTK_TYPE_EVENT_BOX)

/**
 * Creates the popover.
 */
PortConnectionRowWidget *
port_connection_row_widget_new (
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

  /* create the widgets and pack */
  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_visible (box, 1);
  GtkWidget * label =
    gtk_label_new (is_input ?
                   src->identifier.label :
                   dest->identifier.label);
  gtk_widget_set_visible (label, 1);
  gtk_box_pack_start (
    GTK_BOX (box),
    label,
    0, 0, 0);
  self->knob =
    knob_widget_new_port (
      is_input ? src : dest,
      is_input ? dest : src,
      18);
  gtk_box_pack_end (
    GTK_BOX (box),
    GTK_WIDGET (self->knob),
    0,0,0);

  gtk_container_add (
    GTK_CONTAINER (self),
    box);

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
