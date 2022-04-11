// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  CenterDockWidget,
  center_dock_widget,
  GTK_TYPE_WIDGET)

static void
on_divider_pos_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  GtkPaned *   paned)
{
  int new_pos = gtk_paned_get_position (paned);
  g_debug (
    "saving bot panel divider pos: %d", new_pos);
  g_settings_set_int (
    S_UI, "bot-panel-divider-position", new_pos);
}

static bool
on_center_dock_mapped (
  GtkWidget *        widget,
  CenterDockWidget * self)
{
  if (self->first_draw)
    {
      self->first_draw = false;

      /* fetch divider position */
      int pos = g_settings_get_int (
        S_UI, "bot-panel-divider-position");
      g_debug (
        "loading bot panel divider pos: %d", pos);
      gtk_paned_set_position (
        self->center_paned, pos);

      g_signal_connect (
        G_OBJECT (self->center_paned),
        "notify::position",
        G_CALLBACK (on_divider_pos_changed),
        self->center_paned);
    }

  return false;
}

void
center_dock_widget_setup (CenterDockWidget * self)
{
  bot_dock_edge_widget_setup (self->bot_dock_edge);
  left_dock_edge_widget_setup (
    self->left_dock_edge);
  right_dock_edge_widget_setup (
    self->right_dock_edge);
  main_notebook_widget_setup (self->main_notebook);

  g_signal_connect (
    G_OBJECT (self), "map",
    G_CALLBACK (on_center_dock_mapped), self);
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
  if (self->main_notebook)
    {
      main_notebook_widget_tear_down (
        self->main_notebook);
    }
}

static void
dispose (CenterDockWidget * self)
{
  gtk_widget_unparent (
    GTK_WIDGET (self->left_rest_paned));

  G_OBJECT_CLASS (center_dock_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
center_dock_widget_init (CenterDockWidget * self)
{
  self->first_draw = true;

  g_type_ensure (BOT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (RIGHT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (LEFT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (MAIN_NOTEBOOK_WIDGET_TYPE);

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

  gtk_paned_set_shrink_start_child (
    self->left_rest_paned, false);
  gtk_paned_set_shrink_end_child (
    self->left_rest_paned, false);

  gtk_paned_set_shrink_start_child (
    self->center_right_paned, false);
  gtk_paned_set_shrink_end_child (
    self->center_right_paned, false);
  gtk_paned_set_resize_start_child (
    self->center_right_paned, true);
  gtk_paned_set_resize_end_child (
    self->center_right_paned, false);

  gtk_paned_set_shrink_start_child (
    self->center_paned, false);
  gtk_paned_set_shrink_end_child (
    self->center_paned, false);
  gtk_paned_set_resize_start_child (
    self->center_paned, true);
  gtk_paned_set_resize_end_child (
    self->center_paned, true);
}

static void
center_dock_widget_class_init (
  CenterDockWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BOX_LAYOUT);
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
  BIND_CHILD (main_notebook);
  BIND_CHILD (left_dock_edge);
  BIND_CHILD (bot_dock_edge);
  BIND_CHILD (right_dock_edge);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
