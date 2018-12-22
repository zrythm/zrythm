/*
 * gui/widgets/center_dock.c - Main window widget
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

#include "audio/transport.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/tracklist.h"
#include "utils/resources.h"

G_DEFINE_TYPE (CenterDockWidget,
               center_dock_widget,
               DZL_TYPE_DOCK_BIN)

static gboolean
key_release_cb (GtkWidget      * widget,
                 GdkEventKey * event,
                 gpointer       data)
{
  if (event && event->keyval == GDK_KEY_space)
    {
      if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
        transport_request_pause (TRANSPORT);
      else if (TRANSPORT->play_state == PLAYSTATE_PAUSED)
        transport_request_roll (TRANSPORT);
    }

  return FALSE;
}

static void
on_toggle_left_dock_clicked (GtkToolButton * tb,
                             gpointer        user_data)
{
  CenterDockWidget * self = CENTER_DOCK_WIDGET (user_data);
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (self),
                         "left-visible",
                         &a);
  int val = g_value_get_boolean (&a);
  g_value_set_boolean (&a,
                       val == 1 ? 0 : 1);
  g_object_set_property (G_OBJECT (self),
                         "left-visible",
                         &a);
}

static void
on_toggle_bottom_dock_clicked (GtkToolButton * tb,
                             gpointer        user_data)
{
  CenterDockWidget * self = CENTER_DOCK_WIDGET (user_data);
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (self),
                         "bottom-visible",
                         &a);
  int val = g_value_get_boolean (&a);
  g_value_set_boolean (&a,
                       val == 1 ? 0 : 1);
  g_object_set_property (G_OBJECT (self),
                         "bottom-visible",
                         &a);
}

static void
on_toggle_right_dock_clicked (GtkToolButton * tb,
                             gpointer        user_data)
{
  CenterDockWidget * self = CENTER_DOCK_WIDGET (user_data);
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (self),
                         "right-visible",
                         &a);
  int val = g_value_get_boolean (&a);
  g_value_set_boolean (&a,
                       val == 1 ? 0 : 1);
  g_object_set_property (G_OBJECT (self),
                         "right-visible",
                         &a);
}

static void
center_dock_widget_init (CenterDockWidget * self)
{
  g_message ("initing center dock...");
  gtk_widget_init_template (GTK_WIDGET (self));

  g_message ("center dock template initialized");

  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_value_set_boolean (&a,
                       1);
  g_object_set_property (G_OBJECT (self),
                         "left-visible",
                         &a);
  g_object_set_property (G_OBJECT (self),
                         "right-visible",
                         &a);
  g_object_set_property (G_OBJECT (self),
                         "bottom-visible",
                         &a);

  // set icons
  gtk_tool_button_set_icon_widget (
    self->instrument_add,
    resources_get_icon ("plus.svg"));
  gtk_tool_button_set_icon_widget (
    self->toggle_left_dock,
    resources_get_icon ("gnome-builder/builder-view-left-pane-symbolic-light.svg"));
  gtk_tool_button_set_icon_widget (
    self->toggle_bot_dock,
    resources_get_icon ("gnome-builder/builder-view-bottom-pane-symbolic-light.svg"));
  gtk_tool_button_set_icon_widget (
    self->toggle_right_dock,
    resources_get_icon ("gnome-builder/builder-view-right-pane-symbolic-light.svg"));

  /* set events */
  g_signal_connect (G_OBJECT (self), "key_release_event",
                    G_CALLBACK (key_release_cb), self);
}


static void
center_dock_widget_class_init (CenterDockWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);

  resources_set_class_template (
    GTK_WIDGET_CLASS (klass),
    "center_dock.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    editor_top);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_timeline);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_top);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_scroll);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_viewport);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_header);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    ruler);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    ruler_scroll);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    ruler_viewport);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    timeline_scroll);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    timeline_viewport);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    timeline);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    ruler);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    left_tb);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    right_tb);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    snap_grid_midi);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    instrument_add);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    toggle_left_dock);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    toggle_bot_dock);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    toggle_right_dock);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    left_dock_edge);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    bot_dock_edge);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    right_dock_edge);
  gtk_widget_class_bind_template_callback (
    klass,
    on_toggle_left_dock_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_toggle_bottom_dock_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_toggle_right_dock_clicked);
}
