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

/* FIXME don't need a template, add rows
 * dynamically */

#include "audio/port.h"
#include "gui/widgets/dialogs/port_info.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  PortInfoDialogWidget,
  port_info_dialog_widget,
  GTK_TYPE_DIALOG)

static void
set_values (PortInfoDialogWidget * self, Port * port)
{
  self->port = port;

  PortIdentifier * id = &port->id;

  char tmp[600];
  gtk_label_set_text (self->name_lbl, id->label);
  gtk_label_set_text (self->group_lbl, id->port_group);
  port_get_full_designation (port, tmp);
  gtk_label_set_text (self->full_designation_lbl, tmp);
  gtk_label_set_text (
    self->type_lbl, port_type_strings[id->type].str);
  sprintf (
    tmp, _ ("%.1f to %.1f"), (double) port->minf,
    (double) port->maxf);
  gtk_label_set_text (self->range_lbl, tmp);
  sprintf (tmp, "%.1f", (double) port->deff);
  gtk_label_set_text (self->default_value_lbl, tmp);
  if (id->type == TYPE_CONTROL)
    {
      sprintf (tmp, "%f", (double) port->control);
      gtk_label_set_text (self->current_val_lbl, tmp);
    }
  else
    {
      gtk_label_set_text (self->current_val_lbl, _ ("N/A"));
    }
  for (int i = 0;
       i < (int) CYAML_ARRAY_LEN (port_flags_bitvals); i++)
    {
      if (!(id->flags & (1 << i)))
        continue;

      GtkWidget * lbl =
        gtk_label_new (port_flags_bitvals[i].name);
      gtk_widget_set_visible (lbl, 1);
      gtk_widget_set_hexpand (lbl, 1);
      gtk_box_append (GTK_BOX (self->flags_box), lbl);
    }
  for (int i = 0;
       i < (int) CYAML_ARRAY_LEN (port_flags2_bitvals); i++)
    {
      if (!(id->flags2 & (unsigned int) (1 << i)))
        continue;

      GtkWidget * lbl =
        gtk_label_new (port_flags2_bitvals[i].name);
      gtk_widget_set_visible (lbl, 1);
      gtk_widget_set_hexpand (lbl, 1);
      gtk_box_append (GTK_BOX (self->flags_box), lbl);
    }

  /* TODO scale points */
}

/**
 * Creates a new port_info dialog.
 */
PortInfoDialogWidget *
port_info_dialog_widget_new (Port * port)
{
  PortInfoDialogWidget * self =
    g_object_new (PORT_INFO_DIALOG_WIDGET_TYPE, NULL);

  set_values (self, port);

  return self;
}

static void
port_info_dialog_widget_class_init (
  PortInfoDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "port_info_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, PortInfoDialogWidget, x)

  BIND_CHILD (name_lbl);
  BIND_CHILD (group_lbl);
  BIND_CHILD (full_designation_lbl);
  BIND_CHILD (type_lbl);
  BIND_CHILD (range_lbl);
  BIND_CHILD (default_value_lbl);
  BIND_CHILD (current_val_lbl);
  BIND_CHILD (flags_box);
  /* TODO */
  /*BIND_CHILD (scale_points_box);*/
}

static void
port_info_dialog_widget_init (PortInfoDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_window_set_title (GTK_WINDOW (self), _ ("Port info"));
  gtk_window_set_icon_name (GTK_WINDOW (self), "zrythm");
}
