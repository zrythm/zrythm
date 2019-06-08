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

#include "gui/widgets/dzl/dzl-dock-revealer.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/left_dock_edge.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (LeftDockEdgeWidget,
               left_dock_edge_widget,
               GTK_TYPE_BOX)

void
left_dock_edge_widget_setup (
  LeftDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->inspector_notebook,
    NULL,
    DZL_DOCK_REVEALER (
      gtk_widget_get_parent (
        gtk_widget_get_parent (
          GTK_WIDGET (self)))),
    GTK_POS_LEFT);
}

static void
left_dock_edge_widget_init (
  LeftDockEdgeWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GtkWidget * img;

  /* setup inspector */
  self->inspector = inspector_widget_new ();
  img =
    gtk_image_new_from_icon_name (
      "z-document-properties",
      GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Inspector"));
  GtkBox * inspector_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (inspector_box),
    GTK_WIDGET (self->inspector));
  gtk_widget_set_visible (
    GTK_WIDGET (inspector_box), 1);
  gtk_notebook_prepend_page (
    GTK_NOTEBOOK (self->inspector_notebook),
    GTK_WIDGET (inspector_box),
    img);

  /* setup visibility */
  img =
    gtk_image_new_from_icon_name (
      "z-visibility",
      GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Visibility"));
  self->visibility = inspector_widget_new ();
  GtkBox * visibility_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (visibility_box),
    GTK_WIDGET (self->visibility));
  gtk_widget_set_visible (
    GTK_WIDGET (visibility_box), 1);
  gtk_notebook_append_page (
    GTK_NOTEBOOK (self->inspector_notebook),
    GTK_WIDGET (visibility_box),
    img);
}

static void
left_dock_edge_widget_class_init (
  LeftDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "left_dock_edge.ui");

  gtk_widget_class_set_css_name (
    klass, "left-dock-edge");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    LeftDockEdgeWidget,
    inspector_notebook);
}


