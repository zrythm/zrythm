// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_region.h"
#include "common/dsp/track.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/actions/arranger_selections.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/editor_ruler.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/midi_modifier_arranger.h"
#include "gui/backend/gtk_widgets/ruler.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>
#define ACTION_IS(x) (self->action == x)
#define TARGET_IS(x) (self->target == RWTarget::x)

void
editor_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  double        start_x,
  double        start_y)
{
  Position pos = ui_px_to_pos_editor (start_x, true);
  if (!self->shift_held && SNAP_GRID_EDITOR->any_snap ())
    {
      pos.snap (&pos, nullptr, nullptr, *SNAP_GRID_EDITOR);
    }
  TRANSPORT->move_playhead (&pos, true, false, true);
  self->drag_start_pos = pos;
  self->last_set_pos = pos;
  self->action = UiOverlayAction::STARTING_MOVING;
  self->target = RWTarget::Playhead;
}

void
editor_ruler_on_drag_update (RulerWidget * self, double offset_x, double offset_y)
{
  if (ACTION_IS (UiOverlayAction::MOVING))
    {
      Position editor_pos;
      Position region_local_pos;
      auto     r = CLIP_EDITOR->get_region ();

      /* convert px to position */
      editor_pos = ui_px_to_pos_editor (self->start_x + offset_x, true);

      /* snap if not shift held */
      if (!self->shift_held && SNAP_GRID_EDITOR->any_snap ())
        {
          editor_pos.snap (
            &self->drag_start_pos, nullptr, nullptr, *SNAP_GRID_EDITOR);
        }

      region_local_pos.from_ticks (editor_pos.ticks_ - r->pos_.ticks_);

      if (TARGET_IS (LoopStart))
        {
          /* move to nearest acceptable position */
          if (region_local_pos.frames_ < 0)
            {
              region_local_pos.zero ();
            }
          else if (region_local_pos >= r->loop_end_pos_)
            {
              region_local_pos = r->loop_end_pos_;
              region_local_pos.add_frames (-1);
            }

          if (r->is_position_valid (
                region_local_pos, ArrangerObject::PositionType::LoopStart))
            {
              /* set it */
              r->set_position (
                &region_local_pos, ArrangerObject::PositionType::LoopStart,
                true);
              TRANSPORT->update_positions (true);
              EVENTS_PUSH (EventType::ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (LoopEnd))
        {
          /* move to nearest acceptable position */
          if (region_local_pos < r->clip_start_pos_)
            {
              region_local_pos = r->clip_start_pos_;
              region_local_pos.add_frames (1);
            }
          else if (r->is_audio ())
            {
              auto     audio_region = dynamic_cast<AudioRegion *> (r);
              auto     clip = audio_region->get_clip ();
              Position clip_frames ((signed_frame_t) clip->num_frames_);
              if (region_local_pos > clip_frames)
                {
                  region_local_pos = clip_frames;
                }
            }

          if (r->is_position_valid (
                region_local_pos, ArrangerObject::PositionType::LoopEnd))
            {
              /* set it */
              r->set_position (
                &region_local_pos, ArrangerObject::PositionType::LoopEnd, true);
              TRANSPORT->update_positions (true);
              EVENTS_PUSH (EventType::ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (ClipStart))
        {
          /* move to nearest acceptable position */
          if (region_local_pos.frames_ < 0)
            {
              region_local_pos.zero ();
            }
          else if (region_local_pos >= r->loop_end_pos_)
            {
              region_local_pos = r->loop_end_pos_;
              region_local_pos.add_frames (-1);
            }

          /* if position is acceptable */
          if (r->is_position_valid (
                region_local_pos, ArrangerObject::PositionType::ClipStart))
            {
              /* set it */
              r->set_position (
                &region_local_pos, ArrangerObject::PositionType::ClipStart,
                true);
              TRANSPORT->update_positions (true);
              EVENTS_PUSH (EventType::ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (Playhead))
        {
          Position timeline_start;
          Position timeline_end;
          timeline_end.set_to_bar (POSITION_MAX_BAR);

          /* if position is acceptable */
          if (editor_pos >= timeline_start && editor_pos <= timeline_end)
            {
              TRANSPORT->move_playhead (&editor_pos, true, false, true);
              self->last_set_pos = editor_pos;
            }
        }
    }
}

void
editor_ruler_on_drag_end (RulerWidget * self)
{
  /* prepare selections for edit action */
  auto before_sel = std::make_unique<TimelineSelections> ();
  auto after_sel = std::make_unique<TimelineSelections> ();
  auto r = CLIP_EDITOR->get_region ();
  z_return_if_fail (r);

  auto r_variant = convert_to_variant<RegionPtrVariant> (r);

  std::visit (
    [&] (auto &&region) {
      before_sel->add_object_owned (region->clone_unique ());
      after_sel->add_object_owned (region->clone_unique ());
    },
    r_variant);

#define PERFORM_ACTION(pos_member) \
  dynamic_cast<Region *> (before_sel->objects_.back ().get ())->pos_member = \
    self->drag_start_pos; \
  try \
    { \
      UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> ( \
        *before_sel, after_sel.get (), \
        ArrangerSelectionsAction::EditType::Position, false)); \
    } \
  catch (const ZrythmException &e) \
    { \
      e.handle (_ ("Failed to edit position")); \
    }

  if ((ACTION_IS (UiOverlayAction::MOVING)
       || ACTION_IS (UiOverlayAction::STARTING_MOVING)))
    {
      if (TARGET_IS (Playhead))
        {
          /* set cue point */
          TRANSPORT->cue_pos_ = self->last_set_pos;

          EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr);
        }
      else if (TARGET_IS (LoopStart))
        {
          PERFORM_ACTION (loop_start_pos_);
        }
      else if (TARGET_IS (LoopEnd))
        {
          PERFORM_ACTION (loop_end_pos_);
        }
      else if (TARGET_IS (ClipStart))
        {
          PERFORM_ACTION (clip_start_pos_);
        }
    }
}

void
editor_ruler_get_regions_in_range (
  RulerWidget *          self,
  double                 x_start,
  double                 x_end,
  std::vector<Region *> &regions)
{
  Position p1 = ui_px_to_pos_editor (x_start, true);
  Position p2 = ui_px_to_pos_editor (x_end, true);
  auto     track = CLIP_EDITOR->get_track ();
  z_return_if_fail (track);

  track->get_regions_in_range (regions, &p1, &p2);
}
