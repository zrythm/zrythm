// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/audio_region.h"
#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_marker.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <limits.h>

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)
#define TARGET_IS(x) \
  (self->target == RW_TARGET_##x)

void
editor_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble       start_x,
  gdouble       start_y)
{
  Position pos;
  ui_px_to_pos_editor (start_x, &pos, 1);
  if (
    !self->shift_held
    && SNAP_GRID_ANY_SNAP (SNAP_GRID_EDITOR))
    {
      position_snap (
        &pos, &pos, NULL, NULL, SNAP_GRID_EDITOR);
    }
  transport_move_playhead (
    TRANSPORT, &pos, F_PANIC, F_NO_SET_CUE_POINT,
    F_PUBLISH_EVENTS);
  self->drag_start_pos = pos;
  self->last_set_pos = pos;
  self->action = UI_OVERLAY_ACTION_STARTING_MOVING;
  self->target = RW_TARGET_PLAYHEAD;
}

void
editor_ruler_on_drag_update (
  RulerWidget * self,
  gdouble       offset_x,
  gdouble       offset_y)
{
  g_return_if_fail (self);

  if (ACTION_IS (MOVING))
    {
      Position  editor_pos;
      Position  region_local_pos;
      ZRegion * r =
        clip_editor_get_region (CLIP_EDITOR);
      ArrangerObject * r_obj = (ArrangerObject *) r;

      /* convert px to position */
      ui_px_to_pos_editor (
        self->start_x + offset_x, &editor_pos, 1);

      /* snap if not shift held */
      if (
        !self->shift_held
        && SNAP_GRID_ANY_SNAP (SNAP_GRID_EDITOR))
        {
          position_snap (
            &self->drag_start_pos, &editor_pos,
            NULL, NULL, SNAP_GRID_EDITOR);
        }

      position_from_ticks (
        &region_local_pos,
        position_to_ticks (&editor_pos)
          - position_to_ticks (&r_obj->pos));

      if (TARGET_IS (LOOP_START))
        {
          /* move to nearest acceptable position */
          if (region_local_pos.frames < 0)
            {
              position_init (&region_local_pos);
            }
          else if (
            position_is_after_or_equal (
              &region_local_pos,
              &r_obj->loop_end_pos))
            {
              position_set_to_pos (
                &region_local_pos,
                &r_obj->loop_end_pos);
              position_add_frames (
                &region_local_pos, -1);
            }

          if (arranger_object_is_position_valid (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_START))
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_START,
                F_VALIDATE);
              transport_update_positions (
                TRANSPORT, true);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (LOOP_END))
        {
          /* move to nearest acceptable position */
          if (position_is_before (
                &region_local_pos,
                &r_obj->clip_start_pos))
            {
              position_set_to_pos (
                &region_local_pos,
                &r_obj->clip_start_pos);
            }
          else if (r->id.type == REGION_TYPE_AUDIO)
            {
              AudioClip * clip =
                audio_region_get_clip (r);
              Position clip_frames;
              position_from_frames (
                &clip_frames,
                (signed_frame_t) clip->num_frames);
              if (position_is_after (
                    &region_local_pos, &clip_frames))
                {
                  position_set_to_pos (
                    &region_local_pos,
                    &clip_frames);
                }
            }

          if (arranger_object_is_position_valid (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_END))
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
                F_VALIDATE);
              transport_update_positions (
                TRANSPORT, true);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (CLIP_START))
        {
          /* move to nearest acceptable position */
          if (region_local_pos.frames < 0)
            {
              position_init (&region_local_pos);
            }
          else if (
            position_is_after_or_equal (
              &region_local_pos,
              &r_obj->loop_end_pos))
            {
              position_set_to_pos (
                &region_local_pos,
                &r_obj->loop_end_pos);
              position_add_frames (
                &region_local_pos, -1);
            }

          /* if position is acceptable */
          if (arranger_object_is_position_valid (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_CLIP_START))
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_CLIP_START,
                F_VALIDATE);
              transport_update_positions (
                TRANSPORT, true);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (PLAYHEAD))
        {
          Position timeline_start, timeline_end;
          position_init (&timeline_start);
          position_set_to_bar (
            &timeline_end, POSITION_MAX_BAR);

          /* if position is acceptable */
          if (
            position_is_after_or_equal (
              &editor_pos, &timeline_start)
            && position_is_before_or_equal (
              &editor_pos, &timeline_end))
            {
              transport_move_playhead (
                TRANSPORT, &editor_pos, F_PANIC,
                F_NO_SET_CUE_POINT,
                F_PUBLISH_EVENTS);
              self->last_set_pos = editor_pos;
            }

          /*ruler_marker_widget_update_tooltip (*/
          /*self->playhead, 1);*/
        }
    }
}

void
editor_ruler_on_drag_end (RulerWidget * self)
{
  /* prepare selections for edit action */
  ArrangerSelections * before_sel =
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_TIMELINE);
  ArrangerSelections * after_sel =
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_TIMELINE);
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  g_return_if_fail (r);
  ArrangerObject * r_clone_obj_before =
    arranger_object_clone ((ArrangerObject *) r);
  arranger_selections_add_object (
    before_sel, r_clone_obj_before);
  ArrangerObject * r_clone_obj_after =
    arranger_object_clone ((ArrangerObject *) r);
  arranger_selections_add_object (
    after_sel, r_clone_obj_after);

#define PERFORM_ACTION(pos_member) \
  r_clone_obj_before->pos_member = \
    self->drag_start_pos; \
  GError * err = NULL; \
  bool     ret = \
    arranger_selections_action_perform_edit ( \
      before_sel, after_sel, \
      ARRANGER_SELECTIONS_ACTION_EDIT_POS, \
      F_NOT_ALREADY_EDITED, &err); \
  if (!ret) \
    { \
      HANDLE_ERROR ( \
        err, "%s", _ ("Failed to edit position")); \
    }

  if ((ACTION_IS (MOVING)
       || ACTION_IS (STARTING_MOVING)))
    {
      if (TARGET_IS (PLAYHEAD))
        {
          /* set cue point */
          position_set_to_pos (
            &TRANSPORT->cue_pos,
            &self->last_set_pos);

          EVENTS_PUSH (
            ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
        }
      else if (TARGET_IS (LOOP_START))
        {
          PERFORM_ACTION (loop_start_pos);
        }
      else if (TARGET_IS (LOOP_END))
        {
          PERFORM_ACTION (loop_end_pos);
        }
      else if (TARGET_IS (CLIP_START))
        {
          PERFORM_ACTION (clip_start_pos);
        }
    }
}

int
editor_ruler_get_regions_in_range (
  RulerWidget * self,
  double        x_start,
  double        x_end,
  ZRegion **    regions)
{
  Position p1, p2;
  ui_px_to_pos_editor (x_start, &p1, true);
  ui_px_to_pos_editor (x_end, &p2, true);
  Track * track =
    clip_editor_get_track (CLIP_EDITOR);
  g_return_val_if_fail (track, 0);

  return track_get_regions_in_range (
    track, &p1, &p2, regions);
}
