// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/cc_bindings.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/port_connections.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <libpanel.h>
#ifdef __cplusplus
}
#endif

G_DEFINE_TYPE (MainNotebookWidget, main_notebook_widget, GTK_TYPE_WIDGET)

void
main_notebook_widget_setup (MainNotebookWidget * self)
{
  timeline_panel_widget_setup (self->timeline_panel);
  event_viewer_widget_setup (
    self->event_viewer, EventViewerType::EVENT_VIEWER_TYPE_TIMELINE);

  /* set event viewer visibility */
  gtk_widget_set_visible (
    GTK_WIDGET (self->event_viewer),
    g_settings_get_boolean (S_UI, "timeline-event-viewer-visible"));
}

void
main_notebook_widget_refresh (MainNotebookWidget * self)
{
  cc_bindings_widget_refresh (self->cc_bindings);
  port_connections_widget_refresh (self->port_connections);
}

/**
 * Prepare for finalization.
 */
void
main_notebook_widget_tear_down (MainNotebookWidget * self)
{
  timeline_panel_widget_tear_down (self->timeline_panel);
}

static void
dispose (MainNotebookWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->panel_frame));

  G_OBJECT_CLASS (main_notebook_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
main_notebook_widget_init (MainNotebookWidget * self)
{
  self->panel_frame = PANEL_FRAME (panel_frame_new ());
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self->panel_frame), GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_parent (GTK_WIDGET (self->panel_frame), GTK_WIDGET (self));

  g_object_set_data (
    G_OBJECT (g_application_get_default ()), "zrythm-main-panel",
    self->panel_frame);

  self->timeline_plus_event_viewer_paned =
    GTK_PANED (gtk_paned_new (GTK_ORIENTATION_HORIZONTAL));
  gtk_widget_set_focusable (
    GTK_WIDGET (self->timeline_plus_event_viewer_paned), false);
  gtk_paned_set_shrink_start_child (
    self->timeline_plus_event_viewer_paned, false);
  gtk_paned_set_shrink_end_child (self->timeline_plus_event_viewer_paned, false);
  gtk_paned_set_resize_end_child (self->timeline_plus_event_viewer_paned, false);

  self->timeline_panel = timeline_panel_widget_new ();
  gtk_widget_set_hexpand (GTK_WIDGET (self->timeline_panel), true);
  gtk_paned_set_start_child (
    self->timeline_plus_event_viewer_paned, GTK_WIDGET (self->timeline_panel));
  self->event_viewer = event_viewer_widget_new ();
  gtk_paned_set_end_child (
    self->timeline_plus_event_viewer_paned, GTK_WIDGET (self->event_viewer));

  self->port_connections_box =
    GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

  self->cc_bindings_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

  self->scenes_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

  /* add tabs */
#define ADD_TAB(widget, icon, title) \
  { \
    PanelWidget * panel_widget = PANEL_WIDGET (panel_widget_new ()); \
    panel_widget_set_child (panel_widget, GTK_WIDGET (widget)); \
    panel_widget_set_icon_name (panel_widget, icon); \
    panel_widget_set_title (panel_widget, title); \
    panel_frame_add (self->panel_frame, panel_widget); \
  }

  ADD_TAB (
    GTK_WIDGET (self->timeline_plus_event_viewer_paned), "roadmap",
    _ ("Timeline"));
  ADD_TAB (
    GTK_WIDGET (self->port_connections_box), "connector",
    _ ("Port Connections"));
  ADD_TAB (
    GTK_WIDGET (self->cc_bindings_box), "signal-midi", _ ("MIDI CC Bindings"));
#if 0
  ADD_TAB (
    GTK_WIDGET (self->scenes_box), "carousel-horizontal",
    _ ("Scenes (Live View)"));
#endif

  /* setup CC bindings */
  self->cc_bindings = cc_bindings_widget_new ();
  gtk_widget_set_visible (GTK_WIDGET (self->cc_bindings), true);
  gtk_box_append (self->cc_bindings_box, GTK_WIDGET (self->cc_bindings));

  /* setup port connections */
  self->port_connections = port_connections_widget_new ();
  gtk_widget_set_visible (GTK_WIDGET (self->port_connections), true);
  gtk_box_append (
    self->port_connections_box, GTK_WIDGET (self->port_connections));
}

static void
main_notebook_widget_class_init (MainNotebookWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "main-notebook");

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
