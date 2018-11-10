/*
 * gui/widgets/automator.c - A automator widget
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

#include "gui/widgets/automator.h"
#include "gui/widgets/control.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (AutomatorWidget, automator_widget, GTK_TYPE_GRID)

static void
automator_widget_class_init (AutomatorWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/online/alextee/zrythm/ui/automator.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomatorWidget,
                                        name);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomatorWidget,
                                        power_button);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomatorWidget,
                                        controls_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomatorWidget,
                                        controls_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomatorWidget,
                                        toggle);
}

static void
automator_widget_init (AutomatorWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

AutomatorWidget *
automator_widget_new ()
{
  AutomatorWidget * self = g_object_new (AUTOMATOR_WIDGET_TYPE, NULL);

  self->controls[0] = control_widget_new (NULL);
  self->controls[1] = control_widget_new (NULL);
  self->controls[2] = control_widget_new (NULL);
  gtk_box_pack_start (self->controls_box,
                      GTK_WIDGET (self->controls[0]),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      0);
  gtk_box_pack_start (self->controls_box,
                      GTK_WIDGET (self->controls[1]),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      0);
  gtk_box_pack_start (self->controls_box,
                      GTK_WIDGET (self->controls[2]),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      0);

  return self;
}

