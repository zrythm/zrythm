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

#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
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
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/tracklist_header.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  TimelinePanelWidget,
  timeline_panel_widget,
  GTK_TYPE_BOX)

static void
on_hadj_value_changed (
  GtkAdjustment * adj,
  gpointer        user_data)
{
#if 0
  g_debug (
    "horizontal adjustment value changed to %f",
    gtk_adjustment_get_value (adj));
#endif
  EVENTS_PUSH (ET_RULER_VIEWPORT_CHANGED, MW_RULER);
}

void
timeline_panel_widget_setup (
  TimelinePanelWidget * self)
{
  g_return_if_fail (
    Z_IS_TIMELINE_PANEL_WIDGET (self));

  tracklist_widget_setup (
    self->tracklist, TRACKLIST);
  /*pinned_tracklist_widget_setup (*/
  /*self->pinned_tracklist,*/
  /*TRACKLIST);*/

  /* set tracklist header size */
  GtkRequisition req;
  gtk_widget_get_preferred_size (
    GTK_WIDGET (self->ruler_scroll), &req, NULL);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->tracklist_header), -1,
    req.height);
  tracklist_header_widget_setup (
    self->tracklist_header);

  /* setup ruler */
  gtk_scrolled_window_set_hadjustment (
    self->ruler_scroll,
    gtk_scrolled_window_get_hadjustment (
      self->timeline_scroll));

  /* set pinned timeline to follow main timeline's
   * hscroll */
  gtk_scrolled_window_set_hadjustment (
    self->pinned_timeline_scroll,
    gtk_scrolled_window_get_hadjustment (
      self->timeline_scroll));

  ruler_widget_refresh (Z_RULER_WIDGET (MW_RULER));
  ruler_widget_refresh (
    Z_RULER_WIDGET (EDITOR_RULER));

  /* setup timeline */
  arranger_widget_setup (
    Z_ARRANGER_WIDGET (self->timeline),
    ARRANGER_WIDGET_TYPE_TIMELINE,
    SNAP_GRID_TIMELINE);
  self->pinned_timeline->is_pinned = 1;
  gtk_widget_add_css_class (
    GTK_WIDGET (self->pinned_timeline), "pinned");
  arranger_widget_setup (
    Z_ARRANGER_WIDGET (self->pinned_timeline),
    ARRANGER_WIDGET_TYPE_TIMELINE,
    SNAP_GRID_TIMELINE);

  /* link vertical scroll of timeline to
   * tracklist */
  gtk_scrolled_window_set_vadjustment (
    self->timeline_scroll,
    gtk_scrolled_window_get_vadjustment (
      self->tracklist->unpinned_scroll));

  GtkAdjustment * adj =
    gtk_scrollable_get_hadjustment (
      GTK_SCROLLABLE (self->ruler_viewport));

  g_signal_connect (
    G_OBJECT (adj), "value-changed",
    G_CALLBACK (on_hadj_value_changed), self);

  timeline_toolbar_widget_setup (
    self->timeline_toolbar);
}

/**
 * Prepare for finalization.
 */
void
timeline_panel_widget_tear_down (
  TimelinePanelWidget * self)
{
  g_message ("tearing down %p...", self);

  tracklist_widget_tear_down (self->tracklist);

  g_message ("done");
}

TimelinePanelWidget *
timeline_panel_widget_new (void)
{
  TimelinePanelWidget * self =
    g_object_new (TIMELINE_PANEL_WIDGET_TYPE, NULL);

  return self;
}

static void
timeline_panel_widget_init (
  TimelinePanelWidget * self)
{
  g_type_ensure (TIMELINE_BOT_BOX_WIDGET_TYPE);
  g_type_ensure (TRACKLIST_HEADER_WIDGET_TYPE);
  g_type_ensure (TRACKLIST_WIDGET_TYPE);
  g_type_ensure (TIMELINE_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (RULER_WIDGET_TYPE);
  g_type_ensure (ARRANGER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->ruler->type = RULER_WIDGET_TYPE_TIMELINE;
  self->timeline->type =
    ARRANGER_WIDGET_TYPE_TIMELINE;
  self->pinned_timeline->type =
    ARRANGER_WIDGET_TYPE_TIMELINE;
  self->pinned_timeline->is_pinned = 1;

  self->timeline_ruler_h_size_group =
    gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (
    self->timeline_ruler_h_size_group,
    GTK_WIDGET (self->ruler));
  gtk_size_group_add_widget (
    self->timeline_ruler_h_size_group,
    GTK_WIDGET (self->timeline));
  gtk_size_group_add_widget (
    self->timeline_ruler_h_size_group,
    GTK_WIDGET (self->pinned_timeline));

  gtk_paned_set_shrink_start_child (
    self->tracklist_timeline, false);
  gtk_paned_set_shrink_end_child (
    self->tracklist_timeline, false);

  gtk_widget_set_name (
    GTK_WIDGET (self->tracklist_top),
    "tracklist-top-box");
  gtk_widget_set_name (
    GTK_WIDGET (self->ruler_scroll),
    "ruler-scrolled-window");
  gtk_widget_set_name (
    GTK_WIDGET (self->timeline_divider_box),
    "timeline-divider-box");
}

static void
timeline_panel_widget_class_init (
  TimelinePanelWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "timeline_panel.ui");

  gtk_widget_class_set_css_name (
    klass, "timeline-panel");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, TimelinePanelWidget, x)

  BIND_CHILD (tracklist_timeline);
  BIND_CHILD (tracklist_top);
  BIND_CHILD (tracklist_header);
  BIND_CHILD (tracklist);
  BIND_CHILD (ruler_scroll);
  BIND_CHILD (ruler_viewport);
  BIND_CHILD (timeline_scroll);
  BIND_CHILD (timeline_viewport);
  BIND_CHILD (timeline);
  BIND_CHILD (ruler);
  BIND_CHILD (timeline_divider_box);
  BIND_CHILD (pinned_timeline_scroll);
  BIND_CHILD (pinned_timeline_viewport);
  BIND_CHILD (pinned_timeline);
  BIND_CHILD (timeline_toolbar);
  BIND_CHILD (timelines_plus_ruler);
  BIND_CHILD (bot_box);

#undef BIND_CHILD
}
