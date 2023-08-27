/*
 * Copyright (C) 2019 Alexandros Theodotou
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

/** \file
 */

#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler_range.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  RulerRangeWidget,
  ruler_range_widget,
  GTK_TYPE_DRAWING_AREA)

static gboolean
ruler_range_draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  gpointer    data)
{
  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  int width =

    gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 1.0);
  cairo_set_line_width (cr, 2);
  cairo_move_to (cr, 1, 0);
  cairo_line_to (cr, 1, height);
  cairo_move_to (cr, width - 1, 0);
  cairo_line_to (cr, width - 1, height);
  cairo_stroke (cr);

  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 0.3);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  return 0;
}

/**
 * Sets the appropriate cursor.
 */
static void
on_motion (GtkWidget * widget, GdkEventMotion * event)
{
  RulerRangeWidget * self = Z_RULER_RANGE_WIDGET (widget);
  GtkAllocation      allocation;
  gtk_widget_get_allocation (widget, &allocation);

  TimelineRulerWidget * trw = MW_RULER;
  RULER_WIDGET_GET_PRIVATE (trw);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (event->x < UI_RESIZE_CURSOR_SPACE)
        {
          self->cursor_state = UI_CURSOR_STATE_RESIZE_L;
          if (rw_prv->action != UI_OVERLAY_ACTION_MOVING)
            ui_set_cursor_from_name (widget, "w-resize");
        }
      else if (
        event->x > allocation.width - UI_RESIZE_CURSOR_SPACE)
        {
          self->cursor_state = UI_CURSOR_STATE_RESIZE_R;
          if (rw_prv->action != UI_OVERLAY_ACTION_MOVING)
            ui_set_cursor_from_name (widget, "e-resize");
        }
      else
        {
          self->cursor_state = UI_CURSOR_STATE_DEFAULT;
          if (
            rw_prv->action != UI_OVERLAY_ACTION_MOVING
            && rw_prv->action != UI_OVERLAY_ACTION_STARTING_MOVING
            && rw_prv->action != UI_OVERLAY_ACTION_RESIZING_L
            && rw_prv->action != UI_OVERLAY_ACTION_RESIZING_R)
            {
              ui_set_cursor_from_name (widget, "default");
            }
        }
    }
  /* if leaving */
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (
        rw_prv->action != UI_OVERLAY_ACTION_MOVING
        && rw_prv->action != UI_OVERLAY_ACTION_RESIZING_L
        && rw_prv->action != UI_OVERLAY_ACTION_RESIZING_R)
        {
          ui_set_cursor_from_name (widget, "default");
        }
    }
}

RulerRangeWidget *
ruler_range_widget_new ()
{
  RulerRangeWidget * self =
    g_object_new (RULER_RANGE_WIDGET_TYPE, NULL);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
ruler_range_widget_init (RulerRangeWidget * self)
{
  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (
    GTK_WIDGET (self), "draw",
    G_CALLBACK (ruler_range_draw_cb), NULL);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "leave-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "motion-notify-event",
    G_CALLBACK (on_motion), self);
}

static void
ruler_range_widget_class_init (RulerRangeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "ruler-range");
}
