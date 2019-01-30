/*
 * gui/widgets/left_dock_edge.c - Main window widget
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

#include "gui/widgets/inspector.h"
#include "gui/widgets/left_dock_edge.h"
#include "utils/resources.h"

G_DEFINE_TYPE (LeftDockEdgeWidget,
               left_dock_edge_widget,
               GTK_TYPE_BOX)

static void
left_dock_edge_widget_init (LeftDockEdgeWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* setup inspector */
  self->inspector = inspector_widget_new ();
  GtkWidget * img =
    /*resources_get_icon (ICON_TYPE_GNOME_BUILDER,*/
                        /*"ui-section-symbolic-light.svg");*/
    gtk_image_new_from_icon_name (
      "document-properties",
      GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_notebook_prepend_page (self->inspector_notebook,
                             GTK_WIDGET (self->inspector),
                             img);
}

static void
left_dock_edge_widget_class_init (
  LeftDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "left_dock_edge.ui");

  gtk_widget_class_set_css_name (klass,
                                 "left-dock-edge");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    LeftDockEdgeWidget,
    inspector_notebook);
}


