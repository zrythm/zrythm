// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "dsp/engine.h"
#include "dsp/position.h"
#include "dsp/transport.h"
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
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/rt_thread_id.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"
#include <limits.h>

#define ACTION_IS(x) (self->action == UiOverlayAction::x)
#define TARGET_IS(x) (self->target == RWTarget::RW_TARGET_##x)

#define RESIZE_HANDLE_WIDTH 4

/**
 * @param is_loop_range True for loop range, false for time range.
 */
static void
on_drag_begin_range_hit (
  RulerWidget * self,
  double        x,
  Position *    cur_pos,
  bool          is_loop_range)
{
  /* check if resize l or resize r */
  bool resize_l = false;
  bool resize_r = false;

  if (
    is_loop_range
      ? ruler_widget_is_loop_range_hit (self, RulerWidgetRangeType::Start, x, 0)
      : ruler_widget_is_range_hit (self, RulerWidgetRangeType::Start, x, 0))
    {
      resize_l = true;
    }
  if (
    is_loop_range
      ? ruler_widget_is_loop_range_hit (self, RulerWidgetRangeType::End, x, 0)
      : ruler_widget_is_range_hit (self, RulerWidgetRangeType::End, x, 0))
    {
      resize_r = true;
    }

  /* update arranger action */
  if (resize_l)
    {
      self->action = UiOverlayAction::RESIZING_L;
    }
  else if (resize_r)
    {
      self->action = UiOverlayAction::RESIZING_R;
    }
  else
    {
      self->action = UiOverlayAction::STARTING_MOVING;
    }

  self->range1_start_pos =
    is_loop_range ? TRANSPORT->loop_start_pos_ : TRANSPORT->range_1_;
  self->range2_start_pos =
    is_loop_range ? TRANSPORT->loop_end_pos_ : TRANSPORT->range_2_;

  if (is_loop_range)
    {
      /* change range1_first because it affects later calculations */
      self->range1_first = true;
    }
}

void
timeline_ruler_on_drag_end (RulerWidget * self)
{
  /* hide tooltips */
  /*if (self->target == RWTarget::RW_TARGET_PLAYHEAD)*/
  /*ruler_marker_widget_update_tooltip (*/
  /*self->playhead, 0);*/
  if (
    (ACTION_IS (MOVING) || ACTION_IS (STARTING_MOVING))
    && self->target == RWTarget::Playhead)
    {
      /* set cue point */
      TRANSPORT->cue_pos_ = self->last_set_pos;

      EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr);
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
      Position pos = ui_px_to_pos_timeline (start_x, true);
      if (!self->shift_held && SNAP_GRID_TIMELINE->any_snap ())
        {
          pos.snap (&pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
        }

      self->action = UiOverlayAction::STARTING_MOVING;
      self->target = RWTarget::Playhead;
      if (self->ctrl_held)
        {
          /* check if range is hit */
          Position cur_pos = ui_px_to_pos_timeline (start_x, true);
          bool     range_hit =
            cur_pos >= TRANSPORT->loop_start_pos_
            && cur_pos <= TRANSPORT->loop_end_pos_;

          z_info ("loop range hit {}", range_hit);

          /* if within existing range */
          if (range_hit)
            {
              on_drag_begin_range_hit (self, start_x, &cur_pos, true);
              self->target = RWTarget::LoopRange;
            }
        }
      else
        {
          TRANSPORT->move_playhead (&pos, F_PANIC, F_NO_SET_CUE_POINT, true);
        }
      self->drag_start_pos = pos;
      self->last_set_pos = pos;
    }
  else /* if upper 1/4th */
    {
      /* check if range is hit */
      Position cur_pos = ui_px_to_pos_timeline (start_x, true);
      auto [first_range, last_range] = TRANSPORT->get_range_positions ();
      bool range_hit = cur_pos >= first_range && cur_pos <= last_range;

      z_debug ("has range {} range hit {}", TRANSPORT->has_range_, range_hit);
      first_range.print ();
      last_range.print ();
      cur_pos.print ();

      /* if within existing range */
      if (TRANSPORT->has_range_ && range_hit)
        {
          on_drag_begin_range_hit (self, start_x, &cur_pos, false);
        }
      else
        {
          /* set range if project doesn't have range or range is not hit*/
          TRANSPORT->set_has_range (true);
          self->action = UiOverlayAction::RESIZING_R;
          TRANSPORT->range_1_ = ui_px_to_pos_timeline (start_x, true);
          if (!self->shift_held && SNAP_GRID_TIMELINE->any_snap ())
            {
              TRANSPORT->range_1_.snap (
                &TRANSPORT->range_1_, nullptr, nullptr, *SNAP_GRID_TIMELINE);
            }
          TRANSPORT->range_2_ = TRANSPORT->range_1_;
        }
      self->target = RWTarget::Range;
    }
}

void
timeline_ruler_on_drag_update (
  RulerWidget * self,
  gdouble       offset_x,
  gdouble       offset_y)
{
  z_debug ("update");
  /* handle x */
  switch (self->action)
    {
    case UiOverlayAction::RESIZING_L:
    case UiOverlayAction::RESIZING_R:
      if (self->target == RWTarget::Range)
        {
          Position tmp = ui_px_to_pos_timeline (self->start_x + offset_x, true);
          const Position * start_pos;
          const bool       snap = !self->shift_held;
          bool             range1 = true;
          if (
            (self->range1_first && ACTION_IS (RESIZING_L))
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
          TRANSPORT->set_range (range1, start_pos, &tmp, snap);
          EVENTS_PUSH (EventType::ET_RANGE_SELECTION_CHANGED, nullptr);
        }
      break;
    case UiOverlayAction::MOVING:
      if (self->target == RWTarget::Range)
        {
          Position diff_pos = ui_px_to_pos_timeline (fabs (offset_x), false);
          double   ticks_diff = diff_pos.ticks_;
          if (offset_x < 0)
            {
              /*z_info ("offset {:f}", offset_x);*/
              ticks_diff = -ticks_diff;
            }
          double r1_ticks = TRANSPORT->range_1_.ticks_;
          double r2_ticks = TRANSPORT->range_2_.ticks_;
          {
            double r1_start_ticks = self->range1_start_pos.ticks_;
            double r2_start_ticks = self->range2_start_pos.ticks_;
            /*z_info ("ticks diff before {:f} r1 start ticks {:f} r2 start ticks
             * {:f}", ticks_diff, r1_start_ticks, r2_start_ticks);*/
            if (r1_start_ticks + ticks_diff < 0.0)
              {
                ticks_diff -= (ticks_diff + r1_start_ticks);
              }
            if (r2_start_ticks + ticks_diff < 0.0)
              {
                ticks_diff -= (ticks_diff + r2_start_ticks);
              }
          }
          double ticks_length =
            self->range1_first ? r2_ticks - r1_ticks : r1_ticks - r2_ticks;

          /*z_info ("ticks diff {:f}", ticks_diff);*/

          if (self->range1_first)
            {
              TRANSPORT->range_1_ = self->range1_start_pos;
              TRANSPORT->range_1_.add_ticks (ticks_diff);
              if (!self->shift_held && SNAP_GRID_TIMELINE->any_snap ())
                {
                  TRANSPORT->range_1_.snap (
                    &self->range1_start_pos, nullptr, nullptr,
                    *SNAP_GRID_TIMELINE);
                }
              TRANSPORT->range_2_ = TRANSPORT->range_1_;
              TRANSPORT->range_2_.add_ticks (ticks_length);
            }
          else /* range_2 first */
            {
              TRANSPORT->range_2_ = self->range2_start_pos;
              TRANSPORT->range_2_.add_ticks (ticks_diff);
              if (!self->shift_held && SNAP_GRID_TIMELINE->any_snap ())
                {
                  TRANSPORT->range_2_.snap (
                    &self->range2_start_pos, nullptr, nullptr,
                    *SNAP_GRID_TIMELINE);
                }
              TRANSPORT->range_1_ = TRANSPORT->range_2_;
              TRANSPORT->range_1_.add_ticks (ticks_length);
            }
          EVENTS_PUSH (EventType::ET_RANGE_SELECTION_CHANGED, nullptr);
        }
      else if (self->target == RWTarget::LoopRange)
        {
          Position diff_pos = ui_px_to_pos_timeline (fabs (offset_x), false);
          double   ticks_diff = diff_pos.ticks_;
          if (offset_x < 0)
            {
              /*z_info ("offset {:f}", offset_x);*/
              ticks_diff = -ticks_diff;
            }
          double r1_ticks = TRANSPORT->loop_start_pos_.ticks_;
          double r2_ticks = TRANSPORT->loop_end_pos_.ticks_;
          {
            double r1_start_ticks = self->range1_start_pos.ticks_;
            double r2_start_ticks = self->range2_start_pos.ticks_;
            /*z_info ("ticks diff before {:f} r1 start ticks {:f} r2 start ticks
             * {:f}", ticks_diff, r1_start_ticks, r2_start_ticks);*/
            if (r1_start_ticks + ticks_diff < 0.0)
              {
                ticks_diff -= (ticks_diff + r1_start_ticks);
              }
            if (r2_start_ticks + ticks_diff < 0.0)
              {
                ticks_diff -= (ticks_diff + r2_start_ticks);
              }
          }
          double ticks_length =
            self->range1_first ? r2_ticks - r1_ticks : r1_ticks - r2_ticks;

          if (self->range1_first)
            {
              TRANSPORT->loop_start_pos_ = self->range1_start_pos;
              TRANSPORT->loop_start_pos_.add_ticks (ticks_diff);
              if (!self->shift_held && SNAP_GRID_TIMELINE->any_snap ())
                {
                  TRANSPORT->loop_start_pos_.snap (
                    &self->range1_start_pos, nullptr, nullptr,
                    *SNAP_GRID_TIMELINE);
                }
              TRANSPORT->loop_end_pos_ = TRANSPORT->loop_start_pos_;
              TRANSPORT->loop_end_pos_.add_ticks (ticks_length);
            }
          EVENTS_PUSH (EventType::ET_TIMELINE_LOOP_MARKER_POS_CHANGED, nullptr);
        }
      else /* not range */
        {
          /* set some useful positions */
          Position timeline_start, timeline_end;
          timeline_end.set_to_bar (POSITION_MAX_BAR);

          /* convert px to position */
          Position tmp = ui_px_to_pos_timeline (self->start_x + offset_x, true);

          /* snap if not shift held */
          if (!self->shift_held && SNAP_GRID_TIMELINE->any_snap ())
            {
              tmp.snap (
                &self->drag_start_pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
            }

          if (self->target == RWTarget::Playhead)
            {
              /* if position is acceptable */
              if (tmp >= timeline_start && tmp <= timeline_end)
                {
                  TRANSPORT->move_playhead (
                    &tmp, F_PANIC, F_NO_SET_CUE_POINT, true);
                  self->last_set_pos = tmp;
                }

              /*ruler_marker_widget_update_tooltip (*/
              /*self->playhead, 1);*/
            }
          else if (self->target == RWTarget::PunchIn)
            {
              z_info ("moving punch in");
              /* if position is acceptable */
              if (tmp >= timeline_start && tmp < TRANSPORT->punch_out_pos_)
                {
                  TRANSPORT->punch_in_pos_ = tmp;
                  TRANSPORT->update_positions (true);
                  EVENTS_PUSH (
                    EventType::ET_TIMELINE_PUNCH_MARKER_POS_CHANGED, nullptr);
                }
            }
          else if (self->target == RWTarget::PunchOut)
            {
              /* if position is acceptable */
              if (tmp <= timeline_end && tmp > TRANSPORT->punch_in_pos_)
                {
                  TRANSPORT->punch_out_pos_ = tmp;
                  TRANSPORT->update_positions (true);
                  EVENTS_PUSH (
                    EventType::ET_TIMELINE_PUNCH_MARKER_POS_CHANGED, nullptr);
                }
            }
          else if (self->target == RWTarget::LoopStart)
            {
              z_info ("moving loop start");
              /* if position is acceptable */
              if (tmp >= timeline_start && tmp < TRANSPORT->loop_end_pos_)
                {
                  TRANSPORT->loop_start_pos_ = tmp;
                  TRANSPORT->update_positions (true);
                  EVENTS_PUSH (
                    EventType::ET_TIMELINE_LOOP_MARKER_POS_CHANGED, nullptr);
                }
            }
          else if (self->target == RWTarget::LoopEnd)
            {
              /* if position is acceptable */
              if (tmp <= timeline_end && tmp > TRANSPORT->loop_start_pos_)
                {
                  TRANSPORT->loop_end_pos_ = tmp;
                  TRANSPORT->update_positions (true);
                  EVENTS_PUSH (
                    EventType::ET_TIMELINE_LOOP_MARKER_POS_CHANGED, nullptr);
                }
            }
        }
      break;
    default:
      z_warn_if_reached ();
      break;
    }
}
