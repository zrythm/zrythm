/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
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
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <limits.h>

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)
#define TARGET_IS(x) \
  (self->target == RW_TARGET_##x)

#define RESIZE_HANDLE_WIDTH 4

static void
on_drag_begin_range_hit (
  RulerWidget * self,
  double        x,
  Position *    cur_pos)
{
  /*Position range_start, range_end;*/
  /*transport_get_range_pos (*/
  /*TRANSPORT, true, &range_start);*/
  /*transport_get_range_pos (*/
  /*TRANSPORT, false, &range_end);*/
  /*double range_start_px =*/
  /*ui_pos_to_px_timeline (&range_start, true);*/
  /*double range_end_px =*/
  /*ui_pos_to_px_timeline (&range_end, true);*/

  /* check if resize l or resize r */
  bool resize_l = false;
  bool resize_r = false;

  if (ruler_widget_is_range_hit (
        self, RW_RANGE_START, x, 0))
    {
      resize_l = true;
    }
  if (ruler_widget_is_range_hit (
        self, RW_RANGE_END, x, 0))
    {
      resize_r = true;
    }

  /* update arranger action */
  if (resize_l)
    {
      self->action = UI_OVERLAY_ACTION_RESIZING_L;
    }
  else if (resize_r)
    {
      self->action = UI_OVERLAY_ACTION_RESIZING_R;
    }
  else
    {
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      ui_set_cursor_from_name (
        GTK_WIDGET (self), "grabbing");
    }

  position_set_to_pos (
    &self->range1_start_pos, &TRANSPORT->range_1);
  position_set_to_pos (
    &self->range2_start_pos, &TRANSPORT->range_2);
}

void
timeline_ruler_on_drag_end (RulerWidget * self)
{
  /* hide tooltips */
  /*if (self->target == RW_TARGET_PLAYHEAD)*/
  /*ruler_marker_widget_update_tooltip (*/
  /*self->playhead, 0);*/
  if (
    (ACTION_IS (MOVING)
     || ACTION_IS (STARTING_MOVING))
    && self->target == RW_TARGET_PLAYHEAD)
    {
      /* set cue point */
      position_set_to_pos (
        &TRANSPORT->cue_pos, &self->last_set_pos);

      EVENTS_PUSH (
        ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
    }
}

void
timeline_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble       start_x,
  gdouble       start_y,
  int           height)
{
  /* if lower 3/4ths */
  if (start_y > (height * 1) / 4)
    {
      Position pos;
      ui_px_to_pos_timeline (start_x, &pos, 1);
      if (
        !self->shift_held
        && SNAP_GRID_ANY_SNAP (SNAP_GRID_TIMELINE))
        {
          position_snap (
            &pos, &pos, NULL, NULL,
            SNAP_GRID_TIMELINE);
        }
      transport_move_playhead (
        TRANSPORT, &pos, F_PANIC,
        F_NO_SET_CUE_POINT, F_PUBLISH_EVENTS);
      self->drag_start_pos = pos;
      self->last_set_pos = pos;
      self->action =
        UI_OVERLAY_ACTION_STARTING_MOVING;
      self->target = RW_TARGET_PLAYHEAD;
    }
  else /* if upper 1/4th */
    {
      /* check if range is hit */
      Position cur_pos;
      ui_px_to_pos_timeline (
        start_x, &cur_pos, true);
      Position first_range, last_range;
      transport_get_range_pos (
        TRANSPORT, true, &first_range);
      transport_get_range_pos (
        TRANSPORT, false, &last_range);
      bool range_hit =
        position_is_after_or_equal (
          &cur_pos, &first_range)
        && position_is_before_or_equal (
          &cur_pos, &last_range);

      g_message (
        "%s: has range %d range hit %d", __func__,
        TRANSPORT->has_range, range_hit);
      position_print (&first_range);
      position_print (&last_range);
      position_print (&cur_pos);

      /* if within existing range */
      if (TRANSPORT->has_range && range_hit)
        {
          on_drag_begin_range_hit (
            self, start_x, &cur_pos);
        }
      else
        {
          /* set range if project doesn't have
           * range or range is not hit*/
          transport_set_has_range (TRANSPORT, true);
          self->action =
            UI_OVERLAY_ACTION_RESIZING_R;
          ui_px_to_pos_timeline (
            start_x, &TRANSPORT->range_1, true);
          if (
            !self->shift_held
            && SNAP_GRID_ANY_SNAP (
              SNAP_GRID_TIMELINE))
            {
              position_snap (
                &TRANSPORT->range_1,
                &TRANSPORT->range_1, NULL, NULL,
                SNAP_GRID_TIMELINE);
            }
          position_set_to_pos (
            &TRANSPORT->range_2,
            &TRANSPORT->range_1);
        }
      self->target = RW_TARGET_RANGE;
    }
}

void
timeline_ruler_on_drag_update (
  RulerWidget * self,
  gdouble       offset_x,
  gdouble       offset_y)
{
  g_debug ("update");
  /* handle x */
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_L:
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (self->target == RW_TARGET_RANGE)
        {
          Position tmp;
          ui_px_to_pos_timeline (
            self->start_x + offset_x, &tmp, true);
          const Position * start_pos;
          const bool snap = !self->shift_held;
          bool       range1 = true;
          if (
            (self->range1_first
             && ACTION_IS (RESIZING_L))
            || (!self->range1_first && !ACTION_IS (RESIZING_L)))
            {
              start_pos = &self->range1_start_pos;
              range1 = true;
            }
          else
            {
              start_pos = &self->range2_start_pos;
              range1 = false;
            }
          transport_set_range (
            TRANSPORT, range1, start_pos, &tmp,
            snap);
          EVENTS_PUSH (
            ET_RANGE_SELECTION_CHANGED, NULL);
        }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      if (self->target == RW_TARGET_RANGE)
        {
          Position diff_pos;
          ui_px_to_pos_timeline (
            fabs (offset_x), &diff_pos, 0);
          double ticks_diff =
            position_to_ticks (&diff_pos);
          if (offset_x < 0)
            {
              /*g_message ("offset %f", offset_x);*/
              ticks_diff = -ticks_diff;
            }
          double r1_ticks = position_to_ticks (
            (&TRANSPORT->range_1));
          double r2_ticks = position_to_ticks (
            (&TRANSPORT->range_2));
          {
            double r1_start_ticks =
              position_to_ticks (
                (&self->range1_start_pos));
            double r2_start_ticks =
              position_to_ticks (
                (&self->range2_start_pos));
            /*g_message ("ticks diff before %f r1 start ticks %f r2 start ticks %f", ticks_diff, r1_start_ticks, r2_start_ticks);*/
            if (r1_start_ticks + ticks_diff < 0.0)
              {
                ticks_diff -=
                  (ticks_diff + r1_start_ticks);
              }
            if (r2_start_ticks + ticks_diff < 0.0)
              {
                ticks_diff -=
                  (ticks_diff + r2_start_ticks);
              }
          }
          double ticks_length =
            self->range1_first
              ? r2_ticks - r1_ticks
              : r1_ticks - r2_ticks;

          /*g_message ("ticks diff %f", ticks_diff);*/

          if (self->range1_first)
            {
              position_set_to_pos (
                &TRANSPORT->range_1,
                &self->range1_start_pos);
              position_add_ticks (
                &TRANSPORT->range_1, ticks_diff);
              if (
                !self->shift_held
                && SNAP_GRID_ANY_SNAP (
                  SNAP_GRID_TIMELINE))
                {
                  position_snap (
                    &self->range1_start_pos,
                    &TRANSPORT->range_1, NULL,
                    NULL, SNAP_GRID_TIMELINE);
                }
              position_set_to_pos (
                &TRANSPORT->range_2,
                &TRANSPORT->range_1);
              position_add_ticks (
                &TRANSPORT->range_2, ticks_length);
            }
          else /* range_2 first */
            {
              position_set_to_pos (
                &TRANSPORT->range_2,
                &self->range2_start_pos);
              position_add_ticks (
                &TRANSPORT->range_2, ticks_diff);
              if (
                !self->shift_held
                && SNAP_GRID_ANY_SNAP (
                  SNAP_GRID_TIMELINE))
                {
                  position_snap (
                    &self->range2_start_pos,
                    &TRANSPORT->range_2, NULL,
                    NULL, SNAP_GRID_TIMELINE);
                }
              position_set_to_pos (
                &TRANSPORT->range_1,
                &TRANSPORT->range_2);
              position_add_ticks (
                &TRANSPORT->range_1, ticks_length);
            }
          EVENTS_PUSH (
            ET_RANGE_SELECTION_CHANGED, NULL);
        }
      else /* not range */
        {
          /* set some useful positions */
          Position tmp;
          Position timeline_start, timeline_end;
          position_init (&timeline_start);
          position_init (&timeline_end);
          position_set_to_bar (
            &timeline_end, POSITION_MAX_BAR);

          /* convert px to position */
          ui_px_to_pos_timeline (
            self->start_x + offset_x, &tmp,
            F_HAS_PADDING);

          /* snap if not shift held */
          if (
            !self->shift_held
            && SNAP_GRID_ANY_SNAP (
              SNAP_GRID_TIMELINE))
            {
              position_snap (
                &self->drag_start_pos, &tmp, NULL,
                NULL, SNAP_GRID_TIMELINE);
            }

          if (self->target == RW_TARGET_PLAYHEAD)
            {
              /* if position is acceptable */
              if (
                position_is_after_or_equal (
                  &tmp, &timeline_start)
                && position_is_before_or_equal (
                  &tmp, &timeline_end))
                {
                  transport_move_playhead (
                    TRANSPORT, &tmp, F_PANIC,
                    F_NO_SET_CUE_POINT,
                    F_PUBLISH_EVENTS);
                  self->last_set_pos = tmp;
                }

              /*ruler_marker_widget_update_tooltip (*/
              /*self->playhead, 1);*/
            }
          else if (self->target == RW_TARGET_PUNCH_IN)
            {
              g_message ("moving punch in");
              /* if position is acceptable */
              if (
                position_compare (
                  &tmp, &timeline_start)
                  >= 0
                && position_compare (
                     &tmp, &TRANSPORT->punch_out_pos)
                     < 0)
                {
                  position_set_to_pos (
                    &TRANSPORT->punch_in_pos, &tmp);
                  transport_update_positions (
                    TRANSPORT, true);
                  EVENTS_PUSH (
                    ET_TIMELINE_PUNCH_MARKER_POS_CHANGED,
                    NULL);
                }
            }
          else if (self->target == RW_TARGET_PUNCH_OUT)
            {
              /* if position is acceptable */
              if (
                position_compare (
                  &tmp, &timeline_end)
                  <= 0
                && position_compare (
                     &tmp, &TRANSPORT->punch_in_pos)
                     > 0)
                {
                  position_set_to_pos (
                    &TRANSPORT->punch_out_pos,
                    &tmp);
                  transport_update_positions (
                    TRANSPORT, true);
                  EVENTS_PUSH (
                    ET_TIMELINE_PUNCH_MARKER_POS_CHANGED,
                    NULL);
                }
            }
          else if (self->target == RW_TARGET_LOOP_START)
            {
              g_message ("moving loop start");
              /* if position is acceptable */
              if (
                position_compare (
                  &tmp, &timeline_start)
                  >= 0
                && position_compare (
                     &tmp, &TRANSPORT->loop_end_pos)
                     < 0)
                {
                  position_set_to_pos (
                    &TRANSPORT->loop_start_pos,
                    &tmp);
                  transport_update_positions (
                    TRANSPORT, true);
                  EVENTS_PUSH (
                    ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
                    NULL);
                }
            }
          else if (self->target == RW_TARGET_LOOP_END)
            {
              /* if position is acceptable */
              if (
                position_compare (
                  &tmp, &timeline_end)
                  <= 0
                && position_compare (
                     &tmp, &TRANSPORT->loop_start_pos)
                     > 0)
                {
                  position_set_to_pos (
                    &TRANSPORT->loop_end_pos, &tmp);
                  transport_update_positions (
                    TRANSPORT, true);
                  EVENTS_PUSH (
                    ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
                    NULL);
                }
            }
        }
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}
