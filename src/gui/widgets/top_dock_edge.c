/*
 * gui/widgets/top_dock_edge.c - Main window widget
 *
 * Copytop (C) 2018 Alexandros Theodotou
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

#include "zrythm.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/top_dock_edge.h"
#include "utils/gtk.h"
#include "utils/resources.h"

G_DEFINE_TYPE (TopDockEdgeWidget,
               top_dock_edge_widget,
               GTK_TYPE_BOX)

static void
top_dock_edge_widget_init (TopDockEdgeWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* setup top toolbar */
  snap_grid_widget_setup (
    self->snap_grid_timeline,
    &ZRYTHM->snap_grid_timeline);

}

static void
top_dock_edge_widget_class_init (
  TopDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "top_dock_edge.ui");

  gtk_widget_class_set_css_name (klass,
                                 "top-dock-edge");

  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    top_toolbar);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    snap_grid_timeline);
}

