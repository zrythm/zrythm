// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_region.h"
#include "common/dsp/position.h"
#include "common/dsp/snap_grid.h"
#include "common/utils/math.h"
#include "common/utils/rt_thread_id.h"
#include "gui/cpp/backend/actions/arranger_selections.h"
#include "gui/cpp/backend/audio_selections.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/gtk_widgets/audio_arranger.h"
#include "gui/cpp/gtk_widgets/audio_editor_space.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

ArrangerCursor
audio_arranger_widget_get_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ArrangerCursor::Select;
  UiOverlayAction action = self->action;

  switch (action)
    {
    case UiOverlayAction::None:
      if (P_TOOL == Tool::Select)
        {
          /* gain line */
          if (audio_arranger_widget_is_cursor_gain (
                self, self->hover_x, self->hover_y))
            return ArrangerCursor::ResizingUp;

          if (!arranger_widget_is_cursor_in_top_half (self, self->hover_y))
            {
              /* set cursor to range selection */
              return ArrangerCursor::Range;
            }

          /* resize fade in */
          /* note cursor is opposite */
          if (audio_arranger_widget_is_cursor_in_fade (
                *self, self->hover_x, self->hover_y, true, true))
            {
              return ArrangerCursor::ARRANGER_CURSOR_RESIZING_R_FADE;
            }
          /* resize fade out */
          if (audio_arranger_widget_is_cursor_in_fade (
                *self, self->hover_x, self->hover_y, false, true))
            {
              return ArrangerCursor::ResizingLFade;
            }
          /* fade in curviness */
          if (audio_arranger_widget_is_cursor_in_fade (
                *self, self->hover_x, self->hover_y, true, false))
            {
              return ArrangerCursor::ResizingUpFadeIn;
            }
          /* fade out curviness */
          if (audio_arranger_widget_is_cursor_in_fade (
                *self, self->hover_x, self->hover_y, false, false))
            {
              return ArrangerCursor::ResizingUpFadeOut;
            }

          /* set cursor to normal */
          return ac;
        }
      else if (P_TOOL == Tool::Edit)
        ac = ArrangerCursor::Edit;
      else if (P_TOOL == Tool::Eraser)
        ac = ArrangerCursor::Eraser;
      else if (P_TOOL == Tool::Ramp)
        ac = ArrangerCursor::Ramp;
      else if (P_TOOL == Tool::Audition)
        ac = ArrangerCursor::Audition;
      break;
    case UiOverlayAction::STARTING_DELETE_SELECTION:
    case UiOverlayAction::DELETE_SELECTING:
    case UiOverlayAction::STARTING_ERASING:
    case UiOverlayAction::ERASING:
      ac = ArrangerCursor::Eraser;
      break;
    case UiOverlayAction::STARTING_MOVING_COPY:
    case UiOverlayAction::MovingCopy:
      ac = ArrangerCursor::GrabbingCopy;
      break;
    case UiOverlayAction::STARTING_MOVING:
    case UiOverlayAction::MOVING:
      ac = ArrangerCursor::Grabbing;
      break;
    case UiOverlayAction::STARTING_MOVING_LINK:
    case UiOverlayAction::MOVING_LINK:
      ac = ArrangerCursor::GrabbingLink;
      break;
    case UiOverlayAction::StartingPanning:
    case UiOverlayAction::Panning:
      ac = ArrangerCursor::Panning;
      break;
    case UiOverlayAction::RESIZING_UP:
      ac = ArrangerCursor::ResizingUp;
      break;
    case UiOverlayAction::RESIZING_UP_FADE_IN:
      ac = ArrangerCursor::ResizingUpFadeIn;
      break;
    case UiOverlayAction::RESIZING_UP_FADE_OUT:
      ac = ArrangerCursor::ResizingUpFadeOut;
      break;
    case UiOverlayAction::ResizingL:
      ac = ArrangerCursor::ResizingL;
      break;
    case UiOverlayAction::ResizingLFade:
      ac = ArrangerCursor::ResizingLFade;
      break;
    case UiOverlayAction::ResizingR:
      ac = ArrangerCursor::ResizingR;
      break;
    case UiOverlayAction::ResizingRFade:
      ac = ArrangerCursor::ARRANGER_CURSOR_RESIZING_R_FADE;
      break;
    default:
      ac = ArrangerCursor::Select;
      break;
    }

  return ac;
}

void
audio_arranger_widget_snap_range_r (ArrangerWidget * self, Position * pos)
{
  if (self->resizing_range_start)
    {
      /* set range 1 at current point */
      AUDIO_SELECTIONS->sel_start_ = ui_px_to_pos_editor (self->start_x, true);
      if (self->snap_grid->any_snap () && !self->shift_held)
        {
          AUDIO_SELECTIONS->sel_start_.snap_simple (*SNAP_GRID_EDITOR);
        }
      AUDIO_SELECTIONS->sel_end_ = AUDIO_SELECTIONS->sel_start_;

      self->resizing_range_start = false;
    }

  /* set range */
  if (self->snap_grid->any_snap () && !self->shift_held)
    {
      pos->snap_simple (*SNAP_GRID_EDITOR);
    }
  AUDIO_SELECTIONS->sel_end_ = *pos;
  AUDIO_SELECTIONS->set_has_range (true);
}

bool
audio_arranger_widget_is_cursor_in_fade (
  const ArrangerWidget &self,
  double                x,
  double                y,
  bool                  fade_in,
  bool                  resize)
{
  auto r = CLIP_EDITOR->get_region<AudioRegion> ();
  z_return_val_if_fail (r, false);

  const int start_px = ui_pos_to_px_editor (r->pos_, true);
  const int end_px = ui_pos_to_px_editor (r->end_pos_, true);
  const int fade_in_px = start_px + ui_pos_to_px_editor (r->fade_in_pos_, true);
  const int fade_out_px =
    start_px + ui_pos_to_px_editor (r->fade_out_pos_, true);

  constexpr int resize_padding = 4;

  int fade_in_px_w_padding = fade_in_px + resize_padding;
  int fade_out_px_w_padding = fade_out_px - resize_padding;

  if (fade_in && x >= start_px && x <= fade_in_px_w_padding)
    {
      return resize ? x >= fade_in_px - resize_padding : true;
    }
  if (!fade_in && x >= fade_out_px_w_padding && x < end_px)
    {
      return resize ? x <= fade_out_px + resize_padding : true;
    }

  return false;
}

static double
get_region_gain_y (const ArrangerWidget &self, const AudioRegion &region)
{
  int   height = gtk_widget_get_height (GTK_WIDGET (&self));
  float gain_fader_val = math_get_fader_val_from_amp (region.gain_);
  return height * (1.0 - static_cast<double> (gain_fader_val));
}

bool
audio_arranger_widget_is_cursor_gain (
  const ArrangerWidget * self,
  double                 x,
  double                 y)
{
  auto r = CLIP_EDITOR->get_region<AudioRegion> ();
  z_return_val_if_fail (r, false);

  const int start_px = ui_pos_to_px_editor (r->pos_, true);
  const int end_px = ui_pos_to_px_editor (r->end_pos_, true);

  double gain_y = get_region_gain_y (*self, *r);

  constexpr int padding = 4;

  return x >= start_px && x < end_px && y >= gain_y - padding
         && y <= gain_y + padding;
}

UiOverlayAction
audio_arranger_widget_get_action_on_drag_begin (ArrangerWidget * self)
{
  self->action = UiOverlayAction::None;
  ArrangerCursor cursor = arranger_widget_get_cursor (self);
  const auto     r = CLIP_EDITOR->get_region<AudioRegion> ();
  z_return_val_if_fail (r, UiOverlayAction::None);

  switch (cursor)
    {
    case ArrangerCursor::ResizingUp:
      self->fval_at_start = r->gain_;
      return UiOverlayAction::RESIZING_UP;
    case ArrangerCursor::ResizingUpFadeIn:
      self->dval_at_start = r->fade_in_opts_.curviness_;
      return UiOverlayAction::RESIZING_UP_FADE_IN;
    case ArrangerCursor::ResizingUpFadeOut:
      self->dval_at_start = r->fade_out_opts_.curviness_;
      return UiOverlayAction::RESIZING_UP_FADE_OUT;
    /* note cursor is opposite */
    case ArrangerCursor::ARRANGER_CURSOR_RESIZING_R_FADE:
      self->fade_pos_at_start = r->fade_in_pos_;
      return UiOverlayAction::ResizingLFade;
    case ArrangerCursor::ResizingLFade:
      self->fade_pos_at_start = r->fade_out_pos_;
      return UiOverlayAction::ResizingRFade;
    case ArrangerCursor::Range:
      return UiOverlayAction::ResizingR;
    case ArrangerCursor::Select:
      return UiOverlayAction::STARTING_SELECTION;
    default:
      break;
    }
  z_return_val_if_reached (UiOverlayAction::SELECTING)
}

void
audio_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  bool             fade_in)
{
  auto r = CLIP_EDITOR->get_region<AudioRegion> ();
  if (!r)
    return;

  auto  &opts = fade_in ? r->fade_in_opts_ : r->fade_out_opts_;
  double delta = (self->last_offset_y - offset_y) * 0.008;
  opts.curviness_ = std::clamp<double> (opts.curviness_ + delta, -1.0, 1.0);

  EVENTS_PUSH (
    fade_in
      ? EventType::ET_AUDIO_REGION_FADE_IN_CHANGED
      : EventType::ET_AUDIO_REGION_FADE_OUT_CHANGED,
    r);
}

void
audio_arranger_widget_update_gain (ArrangerWidget * self, double offset_y)
{
  auto r = CLIP_EDITOR->get_region<AudioRegion> ();
  z_return_if_fail (r);

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* get current Y value */
  double gain_y = self->start_y + offset_y;

  /* convert to fader value (0.0 - 1.0) and clamp to limits */
  double gain_fader = std::clamp<double> (1.0 - gain_y / height, 0.02, 1.0);

  /* set */
  r->gain_ = math_get_amp_val_from_fader (static_cast<float> (gain_fader));

  EVENTS_PUSH (EventType::ET_AUDIO_REGION_GAIN_CHANGED, r);
}

bool
audio_arranger_widget_snap_fade (
  ArrangerWidget * self,
  Position        &pos,
  bool             fade_in,
  bool             dry_run)
{
  auto r = CLIP_EDITOR->get_region<AudioRegion> ();
  if (!r)
    return false;

  /* get delta with region's start pos */
  double delta =
    pos.ticks_
    - (r->pos_.ticks_ + (fade_in ? r->fade_in_pos_ : r->fade_out_pos_).ticks_);

  /* new start pos, calculated by adding delta to the region's original start
   * pos */
  Position new_pos = fade_in ? r->fade_in_pos_ : r->fade_out_pos_;
  new_pos.add_ticks (delta);

  /* negative positions not allowed */
  if (!new_pos.is_positive ())
    return false;

  if (self->snap_grid->any_snap () && !self->shift_held)
    {
      auto track = r->get_track ();
      z_return_val_if_fail (track, false);
      new_pos.snap (&self->fade_pos_at_start, track, nullptr, *self->snap_grid);
    }

  if (
    !r->is_position_valid (
      new_pos,
      fade_in ? ArrangerObject::PositionType::FadeIn
              : ArrangerObject::PositionType::FadeOut))
    {
      return false;
    }

  if (!dry_run)
    {
      double diff =
        new_pos.ticks_ - (fade_in ? r->fade_in_pos_ : r->fade_out_pos_).ticks_;
      try
        {
          r->resize (fade_in, ArrangerObject::ResizeType::Fade, diff, true);
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to resize object");
          return false;
        }
    }

  EVENTS_PUSH (
    fade_in
      ? EventType::ET_AUDIO_REGION_FADE_IN_CHANGED
      : EventType::ET_AUDIO_REGION_FADE_OUT_CHANGED,
    r);

  return true;
}

void
audio_arranger_on_drag_end (ArrangerWidget * self)
{
  try
    {
      switch (self->action)
        {
        case UiOverlayAction::ResizingR:
          {
            /* if start range selection is after end, fix it */
            if (
              AUDIO_SELECTIONS->has_selection_
              && AUDIO_SELECTIONS->sel_start_ > AUDIO_SELECTIONS->sel_end_)
              {
                Position tmp = AUDIO_SELECTIONS->sel_start_;
                AUDIO_SELECTIONS->sel_start_ = AUDIO_SELECTIONS->sel_end_;
                AUDIO_SELECTIONS->sel_end_ = tmp;
              }
          }
          break;
        case UiOverlayAction::ResizingLFade:
        case UiOverlayAction::ResizingRFade:
          {
            auto region = CLIP_EDITOR->get_region<AudioRegion> ();
            z_return_if_fail (region);
            bool   is_fade_in = self->action == UiOverlayAction::ResizingLFade;
            double ticks_diff =
              (is_fade_in
                 ? region->fade_in_pos_.ticks_
                 : region->fade_out_pos_.ticks_)
              - self->fade_pos_at_start.ticks_;

            auto sel = TimelineSelections ();
            sel.add_object_owned (region->clone_unique ());
            auto sel_at_start = sel.clone_unique ();
            auto [obj_at_start, pos_at_start] =
              sel_at_start->get_first_object_and_pos (false);
            auto ar_at_start = dynamic_cast<AudioRegion *> (obj_at_start);
            if (is_fade_in)
              {
                ar_at_start->fade_in_pos_ = self->fade_pos_at_start;
              }
            else
              {
                ar_at_start->fade_out_pos_ = self->fade_pos_at_start;
              }

            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::ResizeAction> (
                *sel_at_start, &sel,
                is_fade_in
                  ? ArrangerSelectionsAction::ResizeType::LFade
                  : ArrangerSelectionsAction::ResizeType::RFade,
                ticks_diff));
          }
          break;
        case UiOverlayAction::RESIZING_UP_FADE_IN:
        case UiOverlayAction::RESIZING_UP_FADE_OUT:
        case UiOverlayAction::RESIZING_UP:
          {
            auto region = CLIP_EDITOR->get_region<AudioRegion> ();
            z_return_if_fail (region);

            /* prepare current selections */
            auto sel = TimelineSelections ();
            sel.add_object_owned (region->clone_unique ());

            ArrangerSelectionsAction::EditType edit_type;

            /* prepare selections before */
            auto sel_before = TimelineSelections ();
            auto clone_r = region->clone_unique ();
            if (self->action == UiOverlayAction::RESIZING_UP_FADE_IN)
              {
                clone_r->fade_in_opts_.curviness_ = self->dval_at_start;
                edit_type = ArrangerSelectionsAction::EditType::Fades;
              }
            else if (self->action == UiOverlayAction::RESIZING_UP_FADE_OUT)
              {
                clone_r->fade_out_opts_.curviness_ = self->dval_at_start;
                edit_type = ArrangerSelectionsAction::EditType::Fades;
              }
            else if (self->action == UiOverlayAction::RESIZING_UP)
              {
                clone_r->gain_ = self->fval_at_start;
                edit_type = ArrangerSelectionsAction::EditType::Primitive;
              }
            else
              {
                z_return_if_reached ();
              }
            sel_before.add_object_owned (std::move (clone_r));
            UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
              sel_before, &sel, edit_type, true));
          }
          break;
        default:
          break;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to perform action"));
    }
}