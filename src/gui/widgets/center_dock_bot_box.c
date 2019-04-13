/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/center_dock.h"
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_minimap.h"
#include "utils/resources.h"

G_DEFINE_TYPE (CenterDockBotBoxWidget,
               center_dock_bot_box_widget,
               GTK_TYPE_BOX)

static void
center_dock_bot_box_widget_init (
  CenterDockBotBoxWidget * self)
{
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      SNAP_GRID_WIDGET_TYPE, NULL)));
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      TIMELINE_MINIMAP_WIDGET_TYPE, NULL)));

  gtk_widget_init_template (GTK_WIDGET (self));

  // set icons
  gtk_tool_button_set_icon_widget (
    self->instrument_add,
    resources_get_icon (ICON_TYPE_ZRYTHM,
                        "plus.svg"));
  gtk_tool_button_set_icon_widget (
    self->toggle_left_dock,
    resources_get_icon (ICON_TYPE_GNOME_BUILDER,
                        "builder-view-left-pane-symbolic-light.svg"));
  gtk_tool_button_set_icon_widget (
    self->toggle_bot_dock,
    resources_get_icon (ICON_TYPE_GNOME_BUILDER,
                        "builder-view-bottom-pane-symbolic-light.svg"));
  gtk_tool_button_set_icon_widget (
    self->toggle_right_dock,
    resources_get_icon (ICON_TYPE_GNOME_BUILDER,
                        "builder-view-right-pane-symbolic-light.svg"));
}


static void
center_dock_bot_box_widget_class_init (
  CenterDockBotBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "center_dock_bot_box.ui");

  gtk_widget_class_set_css_name (
    klass, "center-dock-bot-box");

  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    left_tb);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    right_tb);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    snap_grid_midi);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    instrument_add);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    toggle_left_dock);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    toggle_bot_dock);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    toggle_right_dock);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockBotBoxWidget,
    timeline_minimap);
}

