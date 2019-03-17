/*
 * gui/widgets/right_dock_edge.c - Main window widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "gui/widgets/file_browser.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "utils/resources.h"

G_DEFINE_TYPE (RightDockEdgeWidget,
               right_dock_edge_widget,
               GTK_TYPE_BOX)

static void
right_dock_edge_widget_init (RightDockEdgeWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* setup browser */
  self->plugin_browser = plugin_browser_widget_new ();
  gtk_notebook_prepend_page (
    self->right_notebook,
    GTK_WIDGET (self->plugin_browser),
    /*resources_get_icon (ICON_TYPE_ZRYTHM,*/
                        /*"plugins.svg"));*/
    gtk_image_new_from_icon_name (
      "z-plugins",
      GTK_ICON_SIZE_SMALL_TOOLBAR));
  self->file_browser = file_browser_widget_new ();
  gtk_notebook_append_page (
    self->right_notebook,
    GTK_WIDGET (self->file_browser),
    gtk_image_new_from_icon_name (
      "z-media-optical-audio",
      GTK_ICON_SIZE_SMALL_TOOLBAR));
  gtk_widget_show_all (GTK_WIDGET (self->right_notebook));
}

static void
right_dock_edge_widget_class_init (
  RightDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "right_dock_edge.ui");

  gtk_widget_class_set_css_name (klass,
                                 "right-dock-edge");

  gtk_widget_class_bind_template_child (
    klass,
    RightDockEdgeWidget,
    right_notebook);
}



