/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/transport.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

G_DEFINE_TYPE (
  RulerMarkerWidget,
  ruler_marker_widget,
  GTK_TYPE_DRAWING_AREA)
/**
 * Sets the appropriate cursor.
 */
static void
on_motion (
  GtkWidget *         widget,
  GdkEventMotion *    event,
  RulerMarkerWidget * self)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget, &allocation);

  RULER_WIDGET_GET_PRIVATE (self->ruler);

  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (
        self->type == RULER_MARKER_TYPE_LOOP_START
        || self->type == RULER_MARKER_TYPE_CLIP_START)
        {
          self->cursor_state = UI_CURSOR_STATE_RESIZE_L;
          ui_set_cursor_from_name (widget, "w-resize");
        }
      else if (self->type == RULER_MARKER_TYPE_LOOP_END)
        {
          self->cursor_state = UI_CURSOR_STATE_RESIZE_R;
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
      switch (self->type)
        {
        case RULER_MARKER_TYPE_PLAYHEAD:
          break;
        case RULER_MARKER_TYPE_CLIP_START:
          break;
        case RULER_MARKER_TYPE_LOOP_START:
          break;
        case RULER_MARKER_TYPE_LOOP_END:
          break;
        case RULER_MARKER_TYPE_CUE_POINT:
          break;
        }
    }
  /*[> if leaving <]*/
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

void
ruler_marker_widget_update_tooltip (
  RulerMarkerWidget * self,
  int                 show)
{
  /* set tooltip text */
  if (self->type == RULER_MARKER_TYPE_PLAYHEAD)
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self), "Playhead");
    }

  /* set tooltip window */
  if (show)
    {
      char tooltip[50];
      int  set = 0;
      if (self->type == RULER_MARKER_TYPE_PLAYHEAD)
        {
          position_to_string (PLAYHEAD, tooltip);
          set = 1;
        }

      if (set)
        {
          gtk_label_set_text (self->tooltip_label, tooltip);
          gtk_window_present (self->tooltip_win);
        }
    }
  else
    gtk_widget_hide (GTK_WIDGET (self->tooltip_win));
}

RulerMarkerWidget *
ruler_marker_widget_new (
  RulerWidget *   ruler,
  RulerMarkerType type)
{
  RulerMarkerWidget * self =
    g_object_new (RULER_MARKER_WIDGET_TYPE, NULL);

  self->type = type;
  self->ruler = ruler;

  /* set tooltip text */
  /*char * tooltip;*/
  if (type == RULER_MARKER_TYPE_PLAYHEAD)
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self), _ ("Playhead"));
    }

  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  return self;
}

static void
ruler_marker_widget_init (RulerMarkerWidget * self)
{
  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  gtk_widget_set_visible (GTK_WIDGET (self), 1);

  /* set tooltip window */
  self->tooltip_win =
    GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));
  gtk_window_set_type_hint (
    self->tooltip_win, GDK_WINDOW_TYPE_HINT_TOOLTIP);
  self->tooltip_label = GTK_LABEL (gtk_label_new ("label"));
  gtk_widget_set_visible (GTK_WIDGET (self->tooltip_label), 1);
  gtk_container_add (
    GTK_CONTAINER (self->tooltip_win),
    GTK_WIDGET (self->tooltip_label));
  gtk_window_set_position (
    self->tooltip_win, GTK_WIN_POS_MOUSE);

  /* connect signals */
  g_signal_connect (
    GTK_WIDGET (self), "draw",
    G_CALLBACK (ruler_marker_draw_cb), self);
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
ruler_marker_widget_class_init (
  RulerMarkerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "ruler-marker");
}

/**
 * @}
 */
