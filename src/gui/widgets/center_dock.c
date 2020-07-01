/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  CenterDockWidget,
  center_dock_widget,
  GTK_TYPE_BOX)

static gboolean
key_release_cb (
  GtkWidget *   widget,
  GdkEventKey * event,
  gpointer      data)
{
  return FALSE;
}

static void
on_divider_pos_changed (
  GObject    *gobject,
  GParamSpec *pspec,
  GtkPaned * paned)
{
  g_settings_set_int (
    S_UI, "bot-panel-divider-position",
    gtk_paned_get_position (paned));
}

void
center_dock_widget_setup (
  CenterDockWidget * self)
{
  gtk_paned_set_position (
    self->center_paned,
    2000);

  timeline_panel_widget_setup (
    self->timeline_panel);
  event_viewer_widget_setup (
    self->event_viewer,
    EVENT_VIEWER_TYPE_TIMELINE);

  /* set event viewer visibility */
  gtk_widget_set_visible (
    GTK_WIDGET (self->event_viewer),
    g_settings_get_boolean (
      S_UI, "timeline-event-viewer-visible"));

  bot_dock_edge_widget_setup (
    self->bot_dock_edge);
  left_dock_edge_widget_setup (
    self->left_dock_edge);
  right_dock_edge_widget_setup (
    self->right_dock_edge);

  /* remember divider position */
  gtk_paned_set_position (
    self->center_paned,
    g_settings_get_int (
      S_UI, "bot-panel-divider-position"));
  g_signal_connect (
    G_OBJECT (self->center_paned), "notify::position",
    G_CALLBACK (on_divider_pos_changed),
    self->center_paned);
}

/**
 * Prepare for finalization.
 */
void
center_dock_widget_tear_down (
  CenterDockWidget * self)
{
  if (self->left_dock_edge)
    {
      left_dock_edge_widget_tear_down (
        self->left_dock_edge);
    }
  if (self->timeline_panel)
    {
      timeline_panel_widget_tear_down (
        self->timeline_panel);
    }
}

static void
center_dock_widget_init (CenterDockWidget * self)
{
  g_type_ensure (BOT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (RIGHT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (LEFT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (TIMELINE_PANEL_WIDGET_TYPE);
  g_type_ensure (EVENT_VIEWER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_value_set_boolean (&a, 1);
  /*g_object_set_property (*/
    /*G_OBJECT (self), "left-visible", &a);*/
  /*g_object_set_property (*/
    /*G_OBJECT (self), "right-visible", &a);*/
  /*g_object_set_property (*/
    /*G_OBJECT (self), "bottom-visible", &a);*/
  /*g_object_set_property (*/
    /*G_OBJECT (self), "top-visible", &a);*/

  /* set events */
  g_signal_connect (
    G_OBJECT (self), "key_release_event",
    G_CALLBACK (key_release_cb), self);
}


static void
center_dock_widget_class_init (CenterDockWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "center_dock.ui");

  gtk_widget_class_set_css_name (
    klass, "center-dock");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, CenterDockWidget, x)

  BIND_CHILD (center_paned);
  BIND_CHILD (left_rest_paned);
  BIND_CHILD (center_right_paned);
  BIND_CHILD (event_viewer);
  BIND_CHILD (timeline_panel);
  BIND_CHILD (left_dock_edge);
  BIND_CHILD (bot_dock_edge);
  BIND_CHILD (right_dock_edge);
  BIND_CHILD (timeline_plus_event_viewer_paned);
}
