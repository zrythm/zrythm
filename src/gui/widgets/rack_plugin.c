/*
 * gui/widgets/rack_plugin.c - A rack_plugin widget
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

#include "gui/widgets/rack_plugin.h"
#include "plugins/plugin.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (RackPluginWidget, rack_plugin_widget, GTK_TYPE_GRID)

static void
rack_plugin_widget_class_init (RackPluginWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/online/alextee/zrythm/ui/rack_plugin.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        name);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        power);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        search_entry);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        in_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        in_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        out_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        out_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackPluginWidget,
                                        automate);
}

static void
rack_plugin_widget_init (RackPluginWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

RackPluginWidget *
rack_plugin_widget_new (Plugin * plugin)
{
  RackPluginWidget * self = g_object_new (RACK_PLUGIN_WIDGET_TYPE, NULL);

  GtkWidget * image;
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/power.svg");
  gtk_button_set_image (GTK_BUTTON (self->power),
                        image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/automate.svg");
  gtk_button_set_image (GTK_BUTTON (self->automate),
                        image);

  return self;
}


