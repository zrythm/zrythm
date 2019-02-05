/*
 * gui/widgets/timeline_ruler.c - Ruler
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_range.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineRulerWidget,
               timeline_ruler_widget,
               RULER_WIDGET_TYPE)


void
timeline_ruler_widget_set_ruler_range_position (
  TimelineRulerWidget * self,
  RulerRangeWidget *    rr,
  GtkAllocation *       allocation)
{
  int range1_first =
    position_compare (&PROJECT->range_1,
                      &PROJECT->range_2) <= 0;

  if (range1_first)
    {
      allocation->x =
        ui_pos_to_px (
          &PROJECT->range_1,
          1);
      allocation->width =
        ui_pos_to_px (
          &PROJECT->range_2,
          1) - allocation->x;
    }
  else
    {
      allocation->x =
        ui_pos_to_px (
          &PROJECT->range_2,
          1);
      allocation->width =
        ui_pos_to_px (
          &PROJECT->range_1,
          1) - allocation->x;
    }
  allocation->y = 0;
  allocation->height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self)) / 2;
}

void
on_drag_begin_range_hit (TimelineRulerWidget * self,
                         RulerRangeWidget *    rr)
{
  /* update arranger action */
  if (rr->cursor_state == UI_CURSOR_STATE_RESIZE_L)
    self->action = UI_OVERLAY_ACTION_RESIZING_L;
  else if (rr->cursor_state == UI_CURSOR_STATE_RESIZE_R)
    self->action = UI_OVERLAY_ACTION_RESIZING_R;
  else
    {
      self->action = UI_OVERLAY_ACTION_STARTING_MOVING;
      ui_set_cursor (GTK_WIDGET (rr), "grabbing");
    }
}

static gboolean
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  if (n_press == 2)
    {
      Position pos;
      ui_px_to_pos (
        x,
        &pos,
        1);
      position_set_to_pos (&TRANSPORT->cue_pos,
                           &pos);

    }
  return FALSE;
}

static void
drag_begin (GtkGestureDrag *       gesture,
            gdouble               start_x,
            gdouble               start_y,
            TimelineRulerWidget * self)
{
  g_message ("drag begin");
  self->start_x = start_x;
  self->range1_first =
    position_compare (&PROJECT->range_1,
                      &PROJECT->range_2) <= 0;

  guint height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));

  /* if lower half */
  if (start_y > height / 2)
    {
      Position pos;
      ui_px_to_pos (
        start_x,
        &pos,
        1);
      transport_move_playhead (&pos, 1);
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = TRW_TARGET_PLAYHEAD;
    }
  else /* if upper half */
    {
      /* check if range is hit */
      int range_hit =
        ui_is_child_hit (GTK_CONTAINER (self),
                         GTK_WIDGET (self->range),
                         start_x,
                         start_y) &&
        gtk_widget_get_visible (GTK_WIDGET (self->range));

      /* if within existing range */
      if (PROJECT->has_range && range_hit)
        {
          on_drag_begin_range_hit (self,
                                   self->range);
        }
      else
        {
          /* set range if project doesn't have range or range
           * is not hit*/
          PROJECT->has_range = 1;
          self->action =
            UI_OVERLAY_ACTION_RESIZING_R;
          self->target = TRW_TARGET_RANGE;
          ui_px_to_pos (
            start_x,
            &PROJECT->range_1,
            1);
          ui_px_to_pos (
            start_x,
            &PROJECT->range_2,
            1);
          gtk_widget_set_visible (
            GTK_WIDGET (self->range), 1);
        }
    }
  self->last_offset_x = 0;
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
            TimelineRulerWidget * self)
{
  if (self->action == UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      self->action = UI_OVERLAY_ACTION_MOVING;
    }

  /* handle x */
  if (self->action == UI_OVERLAY_ACTION_RESIZING_L)
    {
      if (self->target ==
            TRW_TARGET_RANGE)
        {
          ui_px_to_pos (
            self->start_x + offset_x,
            &PROJECT->range_1,
            1);
        }
    } /* endif RESIZING_L */

  else if (self->action == UI_OVERLAY_ACTION_RESIZING_R)
    {
      if (self->target ==
            TRW_TARGET_RANGE)
        {
          ui_px_to_pos (
            self->start_x + offset_x,
            &PROJECT->range_2,
            1);
        }
    } /*endif RESIZING_R */

  /* if moving the selection */
  else if (self->action == UI_OVERLAY_ACTION_MOVING)
    {
      if (self->target == TRW_TARGET_RANGE)
        {
          Position diff_pos;
          ui_px_to_pos (abs (offset_x - self->last_offset_x),
                        &diff_pos,
                        0);
          long frames_diff = position_to_frames (&diff_pos);

          if (offset_x >= self->last_offset_x)
            {
              position_add_frames (&PROJECT->range_1,
                                   frames_diff);
              position_add_frames (&PROJECT->range_2,
                                   frames_diff);
            }
          else
            {
              position_add_frames (&PROJECT->range_1,
                                   -frames_diff);
              position_add_frames (&PROJECT->range_2,
                                   -frames_diff);
            }
        }
      else if (self->target == TRW_TARGET_PLAYHEAD)
        {
          Position pos;
          ui_px_to_pos (
            self->start_x + offset_x,
            &pos,
            1);
          transport_move_playhead (&pos, 1);
        }
    } /* endif MOVING */

  gtk_widget_queue_allocate(GTK_WIDGET (self));
  self->last_offset_x = offset_x;

  /* TODO update inspector */
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
            TimelineRulerWidget * self)
{
  self->start_x = 0;

  self->action = UI_OVERLAY_ACTION_NONE;
}

static void
timeline_ruler_widget_class_init (
  TimelineRulerWidgetClass * klass)
{
}

static void
timeline_ruler_widget_init (
  TimelineRulerWidget * self)
{
  /* add invisible range */
  self->range =
    ruler_range_widget_new ();
  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                           GTK_WIDGET (self->range));

  self->drag = GTK_GESTURE_DRAG (
    gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
    gtk_gesture_multi_press_new (GTK_WIDGET (self)));

  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
}
