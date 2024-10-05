// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/gtk.h"
#include "common/utils/resources.h"
#include "common/utils/ui.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/settings/settings.h"
#include "gui/cpp/backend/tracklist_selections.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/cc_bindings.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/foldable_notebook.h"
#include "gui/cpp/gtk_widgets/inspector_plugin.h"
#include "gui/cpp/gtk_widgets/inspector_track.h"
#include "gui/cpp/gtk_widgets/left_dock_edge.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/port_connections.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (LeftDockEdgeWidget, left_dock_edge_widget, GTK_TYPE_WIDGET)

/* TODO implement after workspaces */
#if 0
static void
on_notebook_switch_page (
  GtkNotebook *        notebook,
  GtkWidget *          page,
  guint                page_num,
  LeftDockEdgeWidget * self)
{
  z_debug (
    "setting left dock page to {}", page_num);

  g_settings_set_int (
    S_UI, "left-panel-tab", (int) page_num);
}
#endif

/**
 * Refreshes the widget and switches to the given
 * page.
 */
void
left_dock_edge_widget_refresh_with_page (
  LeftDockEdgeWidget * self,
  LeftDockEdgeTab      page)
{
  g_settings_set_int (S_UI, "left-panel-tab", (int) page);
  left_dock_edge_widget_refresh (self);

  PanelWidget * panel_widget = NULL;
  switch (page)
    {
    case LeftDockEdgeTab::LEFT_DOCK_EDGE_TAB_TRACK:
      panel_widget = PANEL_WIDGET (gtk_widget_get_ancestor (
        GTK_WIDGET (self->track_inspector), PANEL_TYPE_WIDGET));
      break;
    case LeftDockEdgeTab::LEFT_DOCK_EDGE_TAB_PLUGIN:
      panel_widget = PANEL_WIDGET (gtk_widget_get_ancestor (
        GTK_WIDGET (self->plugin_inspector), PANEL_TYPE_WIDGET));
      break;
    default:
      break;
    }

  if (panel_widget)
    {
      panel_widget_raise (panel_widget);
    }
}

void
left_dock_edge_widget_refresh (LeftDockEdgeWidget * self)
{
  z_debug ("refreshing left dock edge...");

  inspector_track_widget_show_tracks (
    self->track_inspector, TRACKLIST_SELECTIONS.get (), false);
  inspector_plugin_widget_show (
    self->plugin_inspector, MIXER_SELECTIONS.get (), false);

  /* TODO load from workspaces */
#if 0
  LeftDockEdgeTab      page =
    (LeftDockEdge) g_settings_get_int (S_UI, "left-panel-tab");
  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->inspector_notebook);
  gtk_notebook_set_current_page (
    notebook, page_num);
#endif
}

void
left_dock_edge_widget_setup (LeftDockEdgeWidget * self)
{
  inspector_track_widget_setup (
    self->track_inspector, TRACKLIST_SELECTIONS.get ());
}

static GtkScrolledWindow *
wrap_inspector_in_scrolled_window (LeftDockEdgeWidget * self, GtkWidget * widget)
{
  GtkScrolledWindow * scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  /*gtk_scrolled_window_set_overlay_scrolling (*/
  /*scroll, false);*/
  gtk_scrolled_window_set_policy (
    scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (widget));

  /* doesn't allow focus inside it otherwise */
  gtk_widget_set_focusable (GTK_WIDGET (scroll), false);

  return scroll;
}

/**
 * Prepare for finalization.
 */
void
left_dock_edge_widget_tear_down (LeftDockEdgeWidget * self)
{
  z_debug ("tearing down {}...", fmt::ptr (self));

  if (self->track_inspector)
    {
      inspector_track_widget_tear_down (self->track_inspector);
    }

  z_debug ("done");
}

static void
dispose (LeftDockEdgeWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->panel_frame));

  G_OBJECT_CLASS (left_dock_edge_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
left_dock_edge_widget_init (LeftDockEdgeWidget * self)
{
  g_type_ensure (FOLDABLE_NOTEBOOK_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkBoxLayout * box_layout =
    GTK_BOX_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (box_layout), GTK_ORIENTATION_VERTICAL);

  const int           min_width = 160;
  GtkBox *            inspector_wrap;
  GtkScrolledWindow * scroll;

#define ADD_TAB(widget, icon, title) \
  { \
    PanelWidget * panel_widget = PANEL_WIDGET (panel_widget_new ()); \
    panel_widget_set_child (panel_widget, GTK_WIDGET (widget)); \
    panel_widget_set_icon_name (panel_widget, icon); \
    panel_widget_set_title (panel_widget, title); \
    panel_frame_add (self->panel_frame, panel_widget); \
  }

  /* setup track inspector */
  self->track_inspector = inspector_track_widget_new ();
  gtk_widget_set_size_request (
    GTK_WIDGET (self->track_inspector), min_width, -1);
  inspector_wrap = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (GTK_BOX (inspector_wrap), GTK_WIDGET (self->track_inspector));
  scroll = wrap_inspector_in_scrolled_window (self, GTK_WIDGET (inspector_wrap));
  self->track_inspector_scroll = scroll;
  ADD_TAB (scroll, "track-inspector", _ ("Track Inspector"));

  /* setup plugin inspector */
  self->plugin_inspector = inspector_plugin_widget_new ();
  gtk_widget_set_size_request (
    GTK_WIDGET (self->plugin_inspector), min_width, -1);
  inspector_wrap = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (inspector_wrap, GTK_WIDGET (self->plugin_inspector));
  scroll = wrap_inspector_in_scrolled_window (self, GTK_WIDGET (inspector_wrap));
  self->plugin_inspector_scroll = scroll;
  ADD_TAB (scroll, "plug", _ ("Plugin Inspector"));

#undef ADD_TAB
}

static void
left_dock_edge_widget_class_init (LeftDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "left_dock_edge.ui");

  gtk_widget_class_set_css_name (klass, "left-dock-edge");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, LeftDockEdgeWidget, x)

  BIND_CHILD (panel_frame);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BOX_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
