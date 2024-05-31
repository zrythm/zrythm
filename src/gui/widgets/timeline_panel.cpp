// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/tracklist_header.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (TimelinePanelWidget, timeline_panel_widget, GTK_TYPE_BOX)

static void
on_vertical_divider_position_change (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  GtkPaned * paned = GTK_PANED (gobject);
  PRJ_TIMELINE->tracks_width = gtk_paned_get_position (paned);
}

void
timeline_panel_widget_setup (TimelinePanelWidget * self)
{
  g_return_if_fail (Z_IS_TIMELINE_PANEL_WIDGET (self));

  tracklist_widget_setup (self->tracklist, TRACKLIST);

  /* set tracklist header size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self->tracklist_header), -1, RW_HEIGHT);
  gtk_widget_set_size_request (GTK_WIDGET (self->ruler), -1, RW_HEIGHT);
  tracklist_header_widget_setup (self->tracklist_header);

  ruler_widget_refresh (Z_RULER_WIDGET (MW_RULER));
  ruler_widget_refresh (Z_RULER_WIDGET (EDITOR_RULER));

  /* setup unpinned timeline */
  arranger_wrapper_widget_setup (
    Z_ARRANGER_WRAPPER_WIDGET (self->timeline_wrapper),
    ArrangerWidgetType::ARRANGER_WIDGET_TYPE_TIMELINE, SNAP_GRID_TIMELINE);

  /* for some reason the size group in TracklistWidget
   * doesn't work, so just vexpand here */
  gtk_widget_set_vexpand (GTK_WIDGET (self->timeline), true);

  /* setup pinned timeline */
  self->pinned_timeline->is_pinned = 1;
  gtk_widget_add_css_class (GTK_WIDGET (self->pinned_timeline), "pinned");
  arranger_widget_setup (
    Z_ARRANGER_WIDGET (self->pinned_timeline),
    ArrangerWidgetType::ARRANGER_WIDGET_TYPE_TIMELINE, SNAP_GRID_TIMELINE);

  timeline_toolbar_widget_setup (self->timeline_toolbar);
}

/**
 * Prepare for finalization.
 */
void
timeline_panel_widget_tear_down (TimelinePanelWidget * self)
{
  g_message ("tearing down %p...", self);

  tracklist_widget_tear_down (self->tracklist);

  g_message ("done");
}

TimelinePanelWidget *
timeline_panel_widget_new (void)
{
  TimelinePanelWidget * self = static_cast<TimelinePanelWidget *> (
    g_object_new (TIMELINE_PANEL_WIDGET_TYPE, NULL));

  return self;
}

static void
timeline_panel_widget_init (TimelinePanelWidget * self)
{
  g_type_ensure (TRACKLIST_HEADER_WIDGET_TYPE);
  g_type_ensure (TRACKLIST_WIDGET_TYPE);
  g_type_ensure (TIMELINE_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (RULER_WIDGET_TYPE);
  g_type_ensure (ARRANGER_WIDGET_TYPE);
  g_type_ensure (ARRANGER_WRAPPER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->timeline = self->timeline_wrapper->child;

  self->ruler->type = RulerWidgetType::RULER_WIDGET_TYPE_TIMELINE;
  self->timeline->type = ArrangerWidgetType::ARRANGER_WIDGET_TYPE_TIMELINE;
  self->pinned_timeline->type =
    ArrangerWidgetType::ARRANGER_WIDGET_TYPE_TIMELINE;
  self->pinned_timeline->is_pinned = 1;

  self->timeline_ruler_h_size_group =
    gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (
    self->timeline_ruler_h_size_group, GTK_WIDGET (self->ruler));
  gtk_size_group_add_widget (
    self->timeline_ruler_h_size_group, GTK_WIDGET (self->timeline));
  gtk_size_group_add_widget (
    self->timeline_ruler_h_size_group, GTK_WIDGET (self->pinned_timeline));

  gtk_paned_set_shrink_start_child (self->tracklist_timeline, false);
  gtk_paned_set_shrink_end_child (self->tracklist_timeline, false);

  gtk_widget_set_name (GTK_WIDGET (self->tracklist_top), "tracklist-top-box");
  gtk_widget_set_name (
    GTK_WIDGET (self->timeline_divider_box), "timeline-divider-box");

  gtk_widget_set_focus_on_click (GTK_WIDGET (self), false);

  g_signal_connect (
    G_OBJECT (self->tracklist_timeline), "notify::position",
    G_CALLBACK (on_vertical_divider_position_change), NULL);
}

static void
timeline_panel_widget_class_init (TimelinePanelWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (wklass, "timeline_panel.ui");

  gtk_widget_class_set_css_name (wklass, "timeline-panel");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_GROUP);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (wklass, TimelinePanelWidget, x)

  BIND_CHILD (tracklist_timeline);
  BIND_CHILD (tracklist_top);
  BIND_CHILD (tracklist_header);
  BIND_CHILD (tracklist);
  BIND_CHILD (timeline_wrapper);
  BIND_CHILD (ruler);
  BIND_CHILD (timeline_divider_box);
  BIND_CHILD (pinned_timeline);
  BIND_CHILD (timeline_toolbar);
  BIND_CHILD (timelines_plus_ruler);

#undef BIND_CHILD
}
