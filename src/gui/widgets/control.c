/*
 * gui/widgets/control.c - A control widget
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

#include "audio/port.h"
#include "gui/widgets/control.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ControlWidget, control_widget, GTK_TYPE_BOX)

static void
control_widget_class_init (ControlWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/online/alextee/zrythm/ui/control.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        ControlWidget,
                                        name);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        ControlWidget,
                                        knob_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        ControlWidget,
                                        toggle);
}

static void
control_widget_init (ControlWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

ControlWidget *
control_widget_new (Port * port)
{
  ControlWidget * self = g_object_new (CONTROL_WIDGET_TYPE, NULL);

  self->port = port;

  return self;
}

