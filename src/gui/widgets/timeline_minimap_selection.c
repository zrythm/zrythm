/*
 * Copyright (C) 2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_minimap_selection.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  TimelineMinimapSelectionWidget,
  timeline_minimap_selection_widget,
  GTK_TYPE_WIDGET)

#define PADDING 2

static void
timeline_minimap_selection_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  GskRoundedRect rounded_rect;
  graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (
    0, PADDING, width, height - PADDING * 2);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect, 0);
  const float border_width = 2.f;
  GdkRGBA     border_color =
    Z_GDK_RGBA_INIT (0.9, 0.9, 0.9, 0.9);
  float border_widths[] = {
    border_width, border_width, border_width,
    border_width
  };
  GdkRGBA border_colors[] = {
    border_color, border_color, border_color,
    border_color
  };
  GdkRGBA inside_color = {
    border_color.red / 3.f,
    border_color.green / 3.f,
    border_color.blue / 3.f,
    border_color.alpha / 3.f,
  };

  gtk_snapshot_append_color (
    snapshot, &inside_color, &graphene_rect);
  gtk_snapshot_append_border (
    snapshot, &rounded_rect, border_widths,
    border_colors);
}

static void
on_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  TimelineMinimapSelectionWidget * self =
    Z_TIMELINE_MINIMAP_SELECTION_WIDGET (user_data);

  gtk_widget_unset_state_flags (
    GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT);
}

static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  double                     x,
  double                     y,
  gpointer                   user_data)
{
  TimelineMinimapSelectionWidget * self =
    Z_TIMELINE_MINIMAP_SELECTION_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (self);
  int         width =
    gtk_widget_get_allocated_width (widget);

  gtk_widget_set_state_flags (
    GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT, 0);
  if (x < UI_RESIZE_CURSOR_SPACE)
    {
      self->cursor = UI_CURSOR_STATE_RESIZE_L;
      if (
        self->parent->action
        != TIMELINE_MINIMAP_ACTION_MOVING)
        ui_set_cursor_from_name (widget, "w-resize");
    }
  else if (x > width - UI_RESIZE_CURSOR_SPACE)
    {
      self->cursor = UI_CURSOR_STATE_RESIZE_R;
      if (
        self->parent->action
        != TIMELINE_MINIMAP_ACTION_MOVING)
        ui_set_cursor_from_name (widget, "e-resize");
    }
  else
    {
      self->cursor = UI_CURSOR_STATE_DEFAULT;
      if (
        self->parent->action
          != TIMELINE_MINIMAP_ACTION_MOVING
        && self->parent->action
             != TIMELINE_MINIMAP_ACTION_STARTING_MOVING
        && self->parent->action
             != TIMELINE_MINIMAP_ACTION_RESIZING_L
        && self->parent->action
             != TIMELINE_MINIMAP_ACTION_RESIZING_R)
        {
          ui_set_cursor_from_name (
            widget, "default");
        }
    }
}

TimelineMinimapSelectionWidget *
timeline_minimap_selection_widget_new (
  TimelineMinimapWidget * parent)
{
  TimelineMinimapSelectionWidget * self =
    g_object_new (
      TIMELINE_MINIMAP_SELECTION_WIDGET_TYPE, NULL);

  self->parent = parent;

  return self;
}

static void
timeline_minimap_selection_widget_class_init (
  TimelineMinimapSelectionWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot =
    timeline_minimap_selection_snapshot;
  gtk_widget_class_set_css_name (
    wklass, "timeline-minimap-selection");
}

static void
timeline_minimap_selection_widget_init (
  TimelineMinimapSelectionWidget * self)
{
  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  GtkEventController * motion_controller =
    gtk_event_controller_motion_new ();
  gtk_widget_add_controller (
    GTK_WIDGET (self), motion_controller);
  g_signal_connect (
    G_OBJECT (motion_controller), "motion",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_leave), self);
}
