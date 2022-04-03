/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/cc_bindings.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/inspector_plugin.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/port_connections.h"
#include "gui/widgets/visibility.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  LeftDockEdgeWidget,
  left_dock_edge_widget,
  GTK_TYPE_WIDGET)

static void
on_notebook_switch_page (
  GtkNotebook *        notebook,
  GtkWidget *          page,
  guint                page_num,
  LeftDockEdgeWidget * self)
{
  g_debug ("setting left dock page to %u", page_num);

  g_settings_set_int (
    S_UI, "left-panel-tab", (int) page_num);
}

/**
 * Refreshes the widget and switches to the given
 * page.
 */
void
left_dock_edge_widget_refresh_with_page (
  LeftDockEdgeWidget * self,
  LeftDockEdgeTab      page)
{
  g_settings_set_int (
    S_UI, "left-panel-tab", (int) page);
  left_dock_edge_widget_refresh (self);
}

void
left_dock_edge_widget_refresh (
  LeftDockEdgeWidget * self)
{
  g_debug ("refreshing left dock edge...");

  inspector_track_widget_show_tracks (
    self->track_inspector, TRACKLIST_SELECTIONS,
    false);
  inspector_plugin_widget_show (
    self->plugin_inspector, MIXER_SELECTIONS, false);

  int page_num =
    g_settings_get_int (S_UI, "left-panel-tab");
  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->inspector_notebook);
  gtk_notebook_set_current_page (notebook, page_num);
}

void
left_dock_edge_widget_setup (
  LeftDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->inspector_notebook,
    MW_CENTER_DOCK->left_rest_paned, GTK_POS_LEFT,
    false);

  inspector_track_widget_setup (
    self->track_inspector, TRACKLIST_SELECTIONS);

  visibility_widget_refresh (self->visibility);

  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->inspector_notebook);
  g_signal_connect (
    G_OBJECT (notebook), "switch-page",
    G_CALLBACK (on_notebook_switch_page), self);
}

static GtkScrolledWindow *
wrap_inspector_in_scrolled_window (
  LeftDockEdgeWidget * self,
  GtkWidget *          widget)
{
  GtkScrolledWindow * scroll;
  GtkViewport *       viewport;
  scroll = GTK_SCROLLED_WINDOW (
    gtk_scrolled_window_new ());
  /*gtk_scrolled_window_set_overlay_scrolling (*/
  /*scroll, false);*/
  gtk_scrolled_window_set_policy (
    scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  viewport =
    GTK_VIEWPORT (gtk_viewport_new (NULL, NULL));
  gtk_viewport_set_child (
    GTK_VIEWPORT (viewport), widget);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll),
    GTK_WIDGET (viewport));

  return scroll;
}

/**
 * Prepare for finalization.
 */
void
left_dock_edge_widget_tear_down (
  LeftDockEdgeWidget * self)
{
  g_message ("tearing down %p...", self);

  if (self->track_inspector)
    {
      inspector_track_widget_tear_down (
        self->track_inspector);
    }

  g_message ("done");
}

static void
dispose (LeftDockEdgeWidget * self)
{
  gtk_widget_unparent (
    GTK_WIDGET (self->inspector_notebook));

  G_OBJECT_CLASS (left_dock_edge_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
left_dock_edge_widget_init (
  LeftDockEdgeWidget * self)
{
  g_type_ensure (FOLDABLE_NOTEBOOK_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkBoxLayout * box_layout =
    GTK_BOX_LAYOUT (gtk_widget_get_layout_manager (
      GTK_WIDGET (self)));
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (box_layout),
    GTK_ORIENTATION_VERTICAL);

  const int           min_width = 160;
  GtkBox *            inspector_wrap;
  GtkScrolledWindow * scroll;

  /* setup track inspector */
  self->track_inspector =
    inspector_track_widget_new ();
  gtk_widget_set_size_request (
    GTK_WIDGET (self->track_inspector), min_width,
    -1);
  inspector_wrap = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    GTK_BOX (inspector_wrap),
    GTK_WIDGET (self->track_inspector));
  scroll = wrap_inspector_in_scrolled_window (
    self, GTK_WIDGET (inspector_wrap));
  self->track_inspector_scroll = scroll;
  foldable_notebook_widget_add_page (
    self->inspector_notebook, GTK_WIDGET (scroll),
    "track-inspector", _ ("Track"),
    _ ("Track inspector"));

  /* setup plugin inspector */
  self->plugin_inspector =
    inspector_plugin_widget_new ();
  gtk_widget_set_size_request (
    GTK_WIDGET (self->plugin_inspector), min_width,
    -1);
  inspector_wrap = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    inspector_wrap,
    GTK_WIDGET (self->plugin_inspector));
  scroll = wrap_inspector_in_scrolled_window (
    self, GTK_WIDGET (inspector_wrap));
  self->plugin_inspector_scroll = scroll;
  foldable_notebook_widget_add_page (
    self->inspector_notebook, GTK_WIDGET (scroll),
    "plug", _ ("Plugin"), _ ("Plugin inspector"));

  /* setup visibility */
  self->visibility = visibility_widget_new ();
  gtk_widget_set_size_request (
    GTK_WIDGET (self->visibility), min_width, -1);
  self->visibility_box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    self->visibility_box,
    GTK_WIDGET (self->visibility));
  foldable_notebook_widget_add_page (
    self->inspector_notebook,
    GTK_WIDGET (self->visibility_box),
    "view-visible", _ ("Visibility"),
    _ ("Track visibility"));
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

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, LeftDockEdgeWidget, x)

  BIND_CHILD (inspector_notebook);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BOX_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
