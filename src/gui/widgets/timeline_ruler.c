/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/ruler_range.h"
#include "gui/widgets/timeline_arranger.h"
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
        ui_pos_to_px_timeline (
          &PROJECT->range_1,
          1);
      allocation->width =
        ui_pos_to_px_timeline (
          &PROJECT->range_2,
          1) - allocation->x;
    }
  else
    {
      allocation->x =
        ui_pos_to_px_timeline (
          &PROJECT->range_2,
          1);
      allocation->width =
        ui_pos_to_px_timeline (
          &PROJECT->range_1,
          1) - allocation->x;
    }
  allocation->y = 0;
  allocation->height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self)) / 4;
}

void
timeline_ruler_widget_set_ruler_marker_position (
  TimelineRulerWidget * self,
  RulerMarkerWidget *    rm,
  GtkAllocation *       allocation)
{
  switch (rm->type)
    {
    case RULER_MARKER_TYPE_SONG_START:
      allocation->x =
        ui_pos_to_px_timeline (
          &TRANSPORT->start_marker_pos,
          1);
      allocation->y = 0;
      allocation->width = RULER_MARKER_SIZE;
      allocation->height = RULER_MARKER_SIZE;
      break;
    case RULER_MARKER_TYPE_SONG_END:
      allocation->x =
        ui_pos_to_px_timeline (
          &TRANSPORT->end_marker_pos,
          1) - RULER_MARKER_SIZE;
      allocation->y = 0;
      allocation->width = RULER_MARKER_SIZE;
      allocation->height = RULER_MARKER_SIZE;
      break;
    case RULER_MARKER_TYPE_LOOP_START:
      allocation->x =
        ui_pos_to_px_timeline (
          &TRANSPORT->loop_start_pos,
          1);
      allocation->y = RULER_MARKER_SIZE + 1;
      allocation->width = RULER_MARKER_SIZE;
      allocation->height = RULER_MARKER_SIZE;
      break;
    case RULER_MARKER_TYPE_LOOP_END:
      allocation->x =
        ui_pos_to_px_timeline (
          &TRANSPORT->loop_end_pos,
          1) - RULER_MARKER_SIZE;
      allocation->y = RULER_MARKER_SIZE + 1;
      allocation->width = RULER_MARKER_SIZE;
      allocation->height = RULER_MARKER_SIZE;
      break;
    case RULER_MARKER_TYPE_CUE_POINT:
      allocation->x =
        ui_pos_to_px_timeline (
          &TRANSPORT->cue_pos,
          1);
      if (MAIN_WINDOW && MW_RULER)
        {
      allocation->y =
        ((gtk_widget_get_allocated_height (
          GTK_WIDGET (MW_RULER)) - RULER_MARKER_SIZE) - CUE_MARKER_HEIGHT) - 1;
        }
      else
        allocation->y = RULER_MARKER_SIZE *2;
      allocation->width = CUE_MARKER_WIDTH;
      allocation->height = CUE_MARKER_HEIGHT;
      break;
    }

}

void
timeline_ruler_widget_refresh ()
{
  RULER_WIDGET_GET_PRIVATE (MW_RULER);

  /*adjust for zoom level*/
  rw_prv->px_per_tick =
    DEFAULT_PX_PER_TICK * rw_prv->zoom_level;
  rw_prv->px_per_sixteenth =
    rw_prv->px_per_tick * TICKS_PER_SIXTEENTH_NOTE;
  rw_prv->px_per_beat =
    rw_prv->px_per_tick * TICKS_PER_BEAT;
  rw_prv->px_per_bar =
    rw_prv->px_per_beat * TRANSPORT->beats_per_bar;

  Position pos;
  position_set_to_bar (&pos,
                       TRANSPORT->total_bars + 1);
  rw_prv->total_px =
    rw_prv->px_per_tick * position_to_ticks (&pos);

  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (MW_RULER),
    rw_prv->total_px,
    -1);

  gtk_widget_queue_draw (
    GTK_WIDGET (MW_RULER));

  gtk_widget_set_visible (
    GTK_WIDGET (MW_RULER->range),
    PROJECT->has_range);
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

  position_set_to_pos (
    &self->range1_start_pos,
    &PROJECT->range_1);
  position_set_to_pos (
    &self->range2_start_pos,
    &PROJECT->range_2);
}

static gboolean
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  TimelineRulerWidget * self)
{
  if (n_press == 2)
    {
      Position pos;
      ui_px_to_pos_timeline (
        x,
        &pos,
        1);
      position_snap_simple (
        &pos,
        SNAP_GRID_TIMELINE);
      position_set_to_pos (&TRANSPORT->cue_pos,
                           &pos);

    }

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);
  if (state_mask & GDK_SHIFT_MASK)
    self->shift_held = 1;

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
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  RulerMarkerWidget * hit_marker =
    Z_RULER_MARKER_WIDGET (
      ui_get_hit_child (
        GTK_CONTAINER (self),
        start_x,
        start_y,
        RULER_MARKER_WIDGET_TYPE));

  /* if one of the markers hit */
  if (hit_marker)
    {
      if (hit_marker == self->song_start)
        {
          self->action =
            UI_OVERLAY_ACTION_STARTING_MOVING;
          self->target = TRW_TARGET_SONG_START;
        }
      else if (hit_marker == self->song_end)
        {
          self->action =
            UI_OVERLAY_ACTION_STARTING_MOVING;
          self->target = TRW_TARGET_SONG_END;
        }
      else if (hit_marker == self->loop_start)
        {
          self->action =
            UI_OVERLAY_ACTION_STARTING_MOVING;
          self->target = TRW_TARGET_LOOP_START;
        }
      else if (hit_marker == self->loop_end)
        {
          self->action =
            UI_OVERLAY_ACTION_STARTING_MOVING;
          self->target = TRW_TARGET_LOOP_END;
        }
    }
  /* if lower 3/4ths */
  else if (start_y > (height * 1) / 4)
    {
      Position pos;
      ui_px_to_pos_timeline (
        start_x,
        &pos,
        1);
      if (!self->shift_held)
        position_snap_simple (
          &pos,
          SNAP_GRID_TIMELINE);
      transport_move_playhead (&pos, 1);
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = TRW_TARGET_PLAYHEAD;
    }
  else /* if upper 1/4th */
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
          ui_px_to_pos_timeline (
            start_x,
            &PROJECT->range_1,
            1);
          if (!self->shift_held)
            position_snap_simple (
              &PROJECT->range_1,
              SNAP_GRID_TIMELINE);
          position_set_to_pos (
            &PROJECT->range_2,
            &PROJECT->range_1);
          gtk_widget_set_visible (
            GTK_WIDGET (self->range), 1);
        }
      self->target = TRW_TARGET_RANGE;
    }
  self->last_offset_x = 0;
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
            TimelineRulerWidget * self)
{
  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);
  if (state_mask & GDK_SHIFT_MASK)
    self->shift_held = 1;
  else
    self->shift_held = 0;

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
          if (self->range1_first)
            {
              ui_px_to_pos_timeline (
                self->start_x + offset_x,
                &PROJECT->range_1,
                1);
              if (!self->shift_held)
                position_snap_simple (
                  &PROJECT->range_1,
                  SNAP_GRID_TIMELINE);
            }
          else
            {
              ui_px_to_pos_timeline (
                self->start_x + offset_x,
                &PROJECT->range_2,
                1);
              if (!self->shift_held)
                position_snap_simple (
                  &PROJECT->range_2,
                  SNAP_GRID_TIMELINE);
            }
        }
    } /* endif RESIZING_L */

  else if (self->action == UI_OVERLAY_ACTION_RESIZING_R)
    {
      if (self->target ==
            TRW_TARGET_RANGE)
        {
          if (self->range1_first)
            {
              ui_px_to_pos_timeline (
                self->start_x + offset_x,
                &PROJECT->range_2,
                1);
              if (!self->shift_held)
                position_snap_simple (
                  &PROJECT->range_2,
                  SNAP_GRID_TIMELINE);
            }
          else
            {
              ui_px_to_pos_timeline (
                self->start_x + offset_x,
                &PROJECT->range_1,
                1);
              if (!self->shift_held)
                position_snap_simple (
                  &PROJECT->range_1,
                  SNAP_GRID_TIMELINE);
            }
        }
    } /*endif RESIZING_R */

  /* if moving the selection */
  else if (self->action == UI_OVERLAY_ACTION_MOVING)
    {
      if (self->target == TRW_TARGET_RANGE)
        {
          Position diff_pos;
          ui_px_to_pos_timeline (
            abs (offset_x),
            &diff_pos,
            0);
          int ticks_diff =
            position_to_ticks (&diff_pos);
          int r1_ticks =
            position_to_ticks (
              (&PROJECT->range_1));
          int r2_ticks =
            position_to_ticks (
              (&PROJECT->range_2));
          int ticks_length =
            self->range1_first? r2_ticks - r1_ticks :
            r1_ticks - r2_ticks;

          if (offset_x >= 0)
            {
              if (self->range1_first)
                {
                  position_set_to_pos (
                    &PROJECT->range_1,
                    &self->range1_start_pos);
                  position_add_ticks (
                    &PROJECT->range_1,
                    ticks_diff);
                  if (!self->shift_held)
                    position_snap_simple (
                      &PROJECT->range_1,
                      SNAP_GRID_TIMELINE);
                  position_set_to_pos (
                    &PROJECT->range_2,
                    &PROJECT->range_1);
                  position_add_ticks (
                    &PROJECT->range_2,
                    ticks_length);
                }
              else /* range_2 first */
                {
                  position_set_to_pos (
                    &PROJECT->range_2,
                    &self->range2_start_pos);
                  position_add_ticks (
                    &PROJECT->range_2,
                    ticks_diff);
                  if (!self->shift_held)
                    position_snap_simple (
                      &PROJECT->range_2,
                      SNAP_GRID_TIMELINE);
                  position_set_to_pos (
                    &PROJECT->range_1,
                    &PROJECT->range_2);
                  position_add_ticks (
                    &PROJECT->range_1,
                    ticks_length);
                }
            }
          else /* if negative offset */
            {
              if (self->range1_first)
                {
                  position_set_to_pos (
                    &PROJECT->range_1,
                    &self->range1_start_pos);
                  position_add_ticks (
                    &PROJECT->range_1,
                    -ticks_diff);
                  if (!self->shift_held)
                    position_snap_simple (
                      &PROJECT->range_1,
                      SNAP_GRID_TIMELINE);
                  position_set_to_pos (
                    &PROJECT->range_2,
                    &PROJECT->range_1);
                  position_add_ticks (
                    &PROJECT->range_2,
                    ticks_length);
                }
              else
                {
                  position_set_to_pos (
                    &PROJECT->range_2,
                    &self->range2_start_pos);
                  position_add_ticks (
                    &PROJECT->range_2,
                    -ticks_diff);
                  if (!self->shift_held)
                    position_snap_simple (
                      &PROJECT->range_2,
                      SNAP_GRID_TIMELINE);
                  position_set_to_pos (
                    &PROJECT->range_1,
                    &PROJECT->range_2);
                  position_add_ticks (
                    &PROJECT->range_1,
                    ticks_length);
                }
            }
        }
      else if (self->target == TRW_TARGET_PLAYHEAD)
        {
          Position pos;
          ui_px_to_pos_timeline (
            self->start_x + offset_x,
            &pos,
            1);
          if (!self->shift_held)
            position_snap_simple (
              &pos,
              SNAP_GRID_TIMELINE);
          transport_move_playhead (&pos, 1);
        }
      else if (self->target == TRW_TARGET_LOOP_START)
        {
          ui_px_to_pos_timeline (
            self->start_x + offset_x,
            &TRANSPORT->loop_start_pos,
            1);
          if (!self->shift_held)
            position_snap_simple (
              &TRANSPORT->loop_start_pos,
              SNAP_GRID_TIMELINE);
          transport_update_position_frames ();
        }
      else if (self->target == TRW_TARGET_LOOP_END)
        {
          ui_px_to_pos_timeline (
            self->start_x + offset_x,
            &TRANSPORT->loop_end_pos,
            1);
          if (!self->shift_held)
            position_snap_simple (
              &TRANSPORT->loop_end_pos,
              SNAP_GRID_TIMELINE);
          transport_update_position_frames ();
          gtk_widget_queue_draw (
            GTK_WIDGET (self->loop_end));
        }
      else if (self->target == TRW_TARGET_SONG_START)
        {
          ui_px_to_pos_timeline (
            self->start_x + offset_x,
            &TRANSPORT->start_marker_pos,
            1);
          if (!self->shift_held)
            position_snap_simple (
              &TRANSPORT->start_marker_pos,
              SNAP_GRID_TIMELINE);
          transport_update_position_frames ();
        }
      else if (self->target == TRW_TARGET_SONG_END)
        {
          ui_px_to_pos_timeline (
            self->start_x + offset_x,
            &TRANSPORT->end_marker_pos,
            1);
          if (!self->shift_held)
            position_snap_simple (
              &TRANSPORT->end_marker_pos,
              SNAP_GRID_TIMELINE);
          transport_update_position_frames ();
        }
    } /* endif MOVING */

  gtk_widget_queue_allocate (GTK_WIDGET (self));
  self->last_offset_x = offset_x;

  if (self->target ==
        TRW_TARGET_RANGE)
    {
      ARRANGER_WIDGET_GET_PRIVATE (
        MW_TIMELINE);
      gtk_widget_queue_draw (
        GTK_WIDGET (
          ar_prv->bg));
      ar_prv =
        arranger_widget_get_private (
          Z_ARRANGER_WIDGET (MIDI_ARRANGER));
      gtk_widget_queue_draw (
        GTK_WIDGET (
          ar_prv->bg));
      ar_prv =
        arranger_widget_get_private (
          Z_ARRANGER_WIDGET (MIDI_MODIFIER_ARRANGER));
      gtk_widget_queue_draw (
        GTK_WIDGET (
          ar_prv->bg));
    }

  /* TODO update inspector */
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
            TimelineRulerWidget * self)
{
  self->start_x = 0;
  self->shift_held = 0;

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

  /* add all the markers */
  self->song_start =
    ruler_marker_widget_new (
      RULER_MARKER_TYPE_SONG_START);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->song_start));
  self->song_end =
    ruler_marker_widget_new (
      RULER_MARKER_TYPE_SONG_END);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->song_end));
  self->loop_start =
    ruler_marker_widget_new (
      RULER_MARKER_TYPE_LOOP_START);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->loop_start));
  self->loop_end =
    ruler_marker_widget_new (
      RULER_MARKER_TYPE_LOOP_END);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->loop_end));
  self->cue_point =
    ruler_marker_widget_new (
      RULER_MARKER_TYPE_CUE_POINT);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->cue_point));

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
