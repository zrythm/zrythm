/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/main_window.h"
#include "gui/widgets/inspector_plugin.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/port_connections.h"
#include "gui/widgets/visibility.h"
#include "settings/settings.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  LeftDockEdgeWidget, left_dock_edge_widget,
  GTK_TYPE_BOX)

static void
on_notebook_switch_page (
  GtkNotebook * notebook,
  GtkWidget *   page,
  guint         page_num,
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
    self->track_inspector,
    TRACKLIST_SELECTIONS, false);
  inspector_plugin_widget_show (
    self->plugin_inspector, MIXER_SELECTIONS,
    false);

  cc_bindings_widget_refresh (self->cc_bindings);
  port_connections_widget_refresh (
    self->port_connections);

  int page_num =
    g_settings_get_int (S_UI, "left-panel-tab");
  gtk_notebook_set_current_page (
    GTK_NOTEBOOK (self->inspector_notebook),
    page_num);
}

void
left_dock_edge_widget_setup (
  LeftDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->inspector_notebook,
    MW_CENTER_DOCK->left_rest_paned,
    GTK_POS_LEFT);

  inspector_track_widget_setup (
    self->track_inspector, TRACKLIST_SELECTIONS);

  visibility_widget_refresh (
    self->visibility);

  g_signal_connect (
    G_OBJECT (self->inspector_notebook),
    "switch-page",
    G_CALLBACK (on_notebook_switch_page), self);
}

static GtkScrolledWindow *
wrap_inspector_in_scrolled_window (
  LeftDockEdgeWidget * self,
  GtkWidget *          widget)
{
  GtkScrolledWindow * scroll;
  GtkViewport * viewport;
  scroll =
    GTK_SCROLLED_WINDOW (
      gtk_scrolled_window_new (NULL, NULL));
  gtk_widget_set_visible (
    GTK_WIDGET (scroll), true);
  /*gtk_scrolled_window_set_overlay_scrolling (*/
    /*scroll, false);*/
  gtk_scrolled_window_set_policy (
    scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  viewport =
    GTK_VIEWPORT (
      gtk_viewport_new (NULL, NULL));
  gtk_widget_set_visible (
    GTK_WIDGET (viewport), true);
  gtk_container_add (
    GTK_CONTAINER (viewport), widget);
  gtk_container_add (
    GTK_CONTAINER (scroll),
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
left_dock_edge_widget_init (
  LeftDockEdgeWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  const int min_width = 160;
  GtkWidget * img;
  GtkBox * inspector_wrap;
  GtkScrolledWindow * scroll;

  /* setup track inspector */
  self->track_inspector =
    inspector_track_widget_new ();
  img =
    gtk_image_new_from_icon_name (
      "track-inspector",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Track inspector"));
  inspector_wrap =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (inspector_wrap),
    GTK_WIDGET (self->track_inspector));
  gtk_widget_set_visible (
    GTK_WIDGET (inspector_wrap), 1);
  scroll =
    wrap_inspector_in_scrolled_window (
      self, GTK_WIDGET (inspector_wrap));
  gtk_notebook_prepend_page (
    GTK_NOTEBOOK (self->inspector_notebook),
    GTK_WIDGET (scroll), img);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->track_inspector),
    min_width, -1);

  /* setup plugin inspector */
  self->plugin_inspector =
    inspector_plugin_widget_new ();
  img =
    gtk_image_new_from_icon_name (
      "plug",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Plugin inspector"));
  inspector_wrap =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (inspector_wrap),
    GTK_WIDGET (self->plugin_inspector));
  gtk_widget_set_visible (
    GTK_WIDGET (inspector_wrap), 1);
  gtk_notebook_append_page (
    GTK_NOTEBOOK (self->inspector_notebook),
    GTK_WIDGET (inspector_wrap), img);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->plugin_inspector),
    min_width, -1);

  /* setup visibility */
  img =
    gtk_image_new_from_icon_name (
      "view-visible",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Visibility"));
  self->visibility = visibility_widget_new ();
  gtk_widget_set_visible (
    GTK_WIDGET (self->visibility), 1);
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
  gtk_widget_set_size_request (
    GTK_WIDGET (self->visibility),
    min_width, -1);

  /* setup CC bindings */
  img =
    gtk_image_new_from_icon_name (
      "labplot-matrix",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("CC Bindings"));
  self->cc_bindings = cc_bindings_widget_new ();
  gtk_widget_set_visible (
    GTK_WIDGET (self->cc_bindings), 1);
  GtkBox * cc_bindings_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (cc_bindings_box),
    GTK_WIDGET (self->cc_bindings));
  gtk_widget_set_visible (
    GTK_WIDGET (cc_bindings_box), 1);
  gtk_notebook_append_page (
    GTK_NOTEBOOK (self->inspector_notebook),
    GTK_WIDGET (cc_bindings_box),
    img);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->cc_bindings),
    min_width, -1);

  /* setup port connections */
  img =
    gtk_image_new_from_icon_name (
      /*"network-connect",*/
      "standard-connector",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Port connections"));
  self->port_connections =
    port_connections_widget_new ();
  gtk_widget_set_visible (
    GTK_WIDGET (self->port_connections), 1);
  GtkBox * port_connections_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (port_connections_box),
    GTK_WIDGET (self->port_connections));
  gtk_widget_set_visible (
    GTK_WIDGET (port_connections_box), 1);
  gtk_notebook_append_page (
    GTK_NOTEBOOK (self->inspector_notebook),
    GTK_WIDGET (port_connections_box),
    img);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->port_connections),
    min_width, -1);
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
