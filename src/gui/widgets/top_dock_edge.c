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
#include "gui/widgets/quantize_mb.h"
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
    ZRYTHM->snap_grid_timeline);
  quantize_mb_widget_setup (
    self->quantize_mb,
    ZRYTHM->quantize_timeline);

  gtk_toggle_tool_button_set_active (
    self->snap_to_grid,
    ZRYTHM->snap_grid_timeline->snap_to_grid);
  gtk_toggle_tool_button_set_active (
    self->snap_to_grid_keep_offset,
    ZRYTHM->snap_grid_timeline->snap_to_grid_keep_offset);
  gtk_toggle_tool_button_set_active (
    self->snap_to_events,
    ZRYTHM->snap_grid_timeline->snap_to_events);

  gtk_tool_button_set_icon_widget (
    GTK_TOOL_BUTTON (self->snap_to_grid),
    resources_get_icon (
      ICON_TYPE_GNOME_BUILDER,
      "ui-packing-symbolic-light.svg"));
  gtk_tool_button_set_icon_widget (
    GTK_TOOL_BUTTON (self->snap_to_grid_keep_offset),
    resources_get_icon (
      ICON_TYPE_GNOME_BUILDER,
      "widget-layout-symbolic-light.svg"));
  gtk_tool_button_set_icon_widget (
    GTK_TOOL_BUTTON (self->snap_to_events),
    resources_get_icon (
      ICON_TYPE_GNOME_BUILDER,
      "builder-split-tab-right-symbolic-light.svg"));
  gtk_tool_button_set_icon_widget (
    GTK_TOOL_BUTTON (self->zoom_in),
    gtk_image_new_from_icon_name (
      "zoom-in",
      GTK_ICON_SIZE_SMALL_TOOLBAR));
  gtk_tool_button_set_icon_widget (
    GTK_TOOL_BUTTON (self->zoom_out),
    gtk_image_new_from_icon_name (
      "zoom-out",
      GTK_ICON_SIZE_SMALL_TOOLBAR));
  gtk_tool_button_set_icon_widget (
    GTK_TOOL_BUTTON (self->best_fit),
    gtk_image_new_from_icon_name (
      "zoom-fit-best",
      GTK_ICON_SIZE_SMALL_TOOLBAR));
  gtk_tool_button_set_icon_widget (
    GTK_TOOL_BUTTON (self->original_size),
    gtk_image_new_from_icon_name (
      "zoom-original",
      GTK_ICON_SIZE_SMALL_TOOLBAR));
  gtk_widget_show_all (GTK_WIDGET (self->zoom_in));
  gtk_widget_show_all (GTK_WIDGET (self->zoom_out));
  gtk_widget_show_all (GTK_WIDGET (self->best_fit));
  gtk_widget_show_all (GTK_WIDGET (self->original_size));
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
    snap_grid_timeline);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    snap_to_grid);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    snap_to_grid_keep_offset);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    snap_to_events);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    original_size);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    best_fit);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    zoom_out);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    zoom_in);
  gtk_widget_class_bind_template_child (
    klass,
    TopDockEdgeWidget,
    quantize_mb);
}

