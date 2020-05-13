/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/ruler_range.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)
#define TARGET_IS(x) \
  (self->target == RW_TARGET_##x)

#if 0
static void
on_drag_begin_range_hit (
  RulerWidget * self,
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
      ui_set_cursor_from_name (GTK_WIDGET (rr), "grabbing");
    }

  position_set_to_pos (
    &self->range1_start_pos,
    &PROJECT->range_1);
  position_set_to_pos (
    &self->range2_start_pos,
    &PROJECT->range_2);
}
#endif

void
timeline_ruler_on_drag_end (
  RulerWidget * self)
{
  /* hide tooltips */
  /*if (self->target == RW_TARGET_PLAYHEAD)*/
    /*ruler_marker_widget_update_tooltip (*/
      /*self->playhead, 0);*/
  if ((ACTION_IS (MOVING) ||
         ACTION_IS (STARTING_MOVING)) &&
      self->target == RW_TARGET_PLAYHEAD)
    {
      /* set cue point */
      position_set_to_pos (
        &TRANSPORT->cue_pos,
        &self->last_set_pos);

      EVENTS_PUSH (
        ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
    }
}

void
timeline_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble               start_x,
  gdouble               start_y,
  int                  height)
{
  /* if lower 3/4ths */
  if (start_y > (height * 1) / 4)
    {
      Position pos;
      ui_px_to_pos_timeline (
        start_x, &pos, 1);
      if (!self->shift_held)
        position_snap_simple (
          &pos,
          SNAP_GRID_TIMELINE);
      transport_move_playhead (
        TRANSPORT, &pos, F_PANIC,
        F_NO_SET_CUE_POINT);
      self->last_set_pos = pos;
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_PLAYHEAD;
    }
  else /* if upper 1/4th */
    {
#if 0
      /* check if range is hit */
      int range_hit =
        ui_is_child_hit (
          GTK_WIDGET (self),
          GTK_WIDGET (self->range),
          1, 1, start_x, start_y, 0, 0) &&
        gtk_widget_get_visible (
          GTK_WIDGET (self->range));

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
      self->target = RW_TARGET_RANGE;
#endif
    }
}

void
timeline_ruler_on_drag_update (
  RulerWidget * self,
  gdouble               offset_x,
  gdouble               offset_y)
{
  /* handle x */
  if (ACTION_IS (RESIZING_L))
    {
      if (self->target == RW_TARGET_RANGE)
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
          EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED,
                       NULL);
        }
    } /* endif RESIZING_L */

  else if (ACTION_IS (RESIZING_R))
    {
      if (self->target ==
            RW_TARGET_RANGE)
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
          EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED,
                       NULL);
        }
    } /*endif RESIZING_R */

  /* if moving the selection */
  else if (ACTION_IS (MOVING))
    {
      if (self->target == RW_TARGET_RANGE)
        {
          Position diff_pos;
          ui_px_to_pos_timeline (
            fabs (offset_x),
            &diff_pos,
            0);
          double ticks_diff =
            position_to_ticks (&diff_pos);
          double r1_ticks =
            position_to_ticks (
              (&PROJECT->range_1));
          double r2_ticks =
            position_to_ticks (
              (&PROJECT->range_2));
          double ticks_length =
            self->range1_first ?
            r2_ticks - r1_ticks :
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
          EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED,
                       NULL);
        }
      else /* not range */
        {
          /* set some useful positions */
          Position tmp;
          Position timeline_start,
                   timeline_end;
          position_init (&timeline_start);
          position_init (&timeline_end);
          position_add_bars (
            &timeline_end,
            TRANSPORT->total_bars);

          /* convert px to position */
          ui_px_to_pos_timeline (
            self->start_x + offset_x,
            &tmp, F_HAS_PADDING);

          /* snap if not shift held */
          if (!self->shift_held)
            position_snap_simple (
              &tmp, SNAP_GRID_TIMELINE);

          if (self->target == RW_TARGET_PLAYHEAD)
            {
              /* if position is acceptable */
              if (position_compare (
                    &tmp, &timeline_start) >= 0 &&
                  position_compare (
                    &tmp, &timeline_end) <= 0)
                {
                  transport_move_playhead (
                    TRANSPORT, &tmp,
                    F_PANIC, F_NO_SET_CUE_POINT);
                  self->last_set_pos = tmp;
                  EVENTS_PUSH (
                    ET_PLAYHEAD_POS_CHANGED_MANUALLY,
                    NULL);
                }

              /*ruler_marker_widget_update_tooltip (*/
                /*self->playhead, 1);*/
            }
          else if (self->target ==
                     RW_TARGET_LOOP_START)
            {
              g_message ("moving loop start");
              /* if position is acceptable */
              if (position_compare (
                    &tmp, &timeline_start) >= 0 &&
                  position_compare (
                    &tmp, &TRANSPORT->loop_end_pos) < 0)
                {
                  position_set_to_pos (
                    &TRANSPORT->loop_start_pos, &tmp);
                  transport_update_position_frames (
                    TRANSPORT);
                  EVENTS_PUSH (
                    ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
                    NULL);
                }
            }
          else if (self->target ==
                     RW_TARGET_LOOP_END)
            {
              /* if position is acceptable */
              if (position_compare (
                    &tmp, &timeline_end) <= 0 &&
                  position_compare (
                    &tmp,
                    &TRANSPORT->loop_start_pos) > 0)
                {
                  position_set_to_pos (
                    &TRANSPORT->loop_end_pos, &tmp);
                  transport_update_position_frames (
                    TRANSPORT);
                  EVENTS_PUSH (
                    ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
                    NULL);
                }
            }
        }
    } /* endif MOVING */
}
