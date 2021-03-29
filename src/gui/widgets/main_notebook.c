/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/cc_bindings.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/port_connections.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  MainNotebookWidget, main_notebook_widget,
  FOLDABLE_NOTEBOOK_WIDGET_TYPE)

void
main_notebook_widget_setup (
  MainNotebookWidget * self)
{
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
}

void
main_notebook_widget_refresh (
  MainNotebookWidget * self)
{
  cc_bindings_widget_refresh (self->cc_bindings);
  port_connections_widget_refresh (
    self->port_connections);
}

/**
 * Prepare for finalization.
 */
void
main_notebook_widget_tear_down (
  MainNotebookWidget * self)
{
  g_message ("tearing down %p...", self);

  timeline_panel_widget_tear_down (
    self->timeline_panel);

  g_message ("done");
}

static void
main_notebook_widget_init (
  MainNotebookWidget * self)
{
  g_type_ensure (TIMELINE_PANEL_WIDGET_TYPE);
  g_type_ensure (EVENT_VIEWER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  /* setup CC bindings */
  self->cc_bindings = cc_bindings_widget_new ();
  gtk_widget_set_visible (
    GTK_WIDGET (self->cc_bindings), true);
  gtk_box_pack_start (
    self->cc_bindings_box,
    GTK_WIDGET (self->cc_bindings),
    F_EXPAND, F_FILL, 0);

  /* setup port connections */
  self->port_connections =
    port_connections_widget_new ();
  gtk_widget_set_visible (
    GTK_WIDGET (self->port_connections), true);
  gtk_box_pack_start (
    self->port_connections_box,
    GTK_WIDGET (self->port_connections),
    F_EXPAND, F_FILL, 0);
}

static void
main_notebook_widget_class_init (
  MainNotebookWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "main_notebook.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, MainNotebookWidget, x)

  BIND_CHILD (timeline_panel);
  BIND_CHILD (event_viewer);
  BIND_CHILD (end_stack);
  BIND_CHILD (cc_bindings_box);
  BIND_CHILD (port_connections_box);
  BIND_CHILD (timeline_plus_event_viewer_paned);
}
