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
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_marker.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)
#define TARGET_IS(x) \
  (self->target == RW_TARGET_##x)

void
editor_ruler_on_drag_begin_no_marker_hit (
  RulerWidget * self,
  gdouble             start_x,
  gdouble             start_y)
{
  Position pos;
  ui_px_to_pos_editor (
    start_x, &pos, 1);
  if (!self->shift_held)
    position_snap_simple (
      &pos, SNAP_GRID_MIDI);
  transport_move_playhead (
    TRANSPORT, &pos, F_PANIC, F_NO_SET_CUE_POINT);
  self->last_set_pos = pos;
  self->action =
    UI_OVERLAY_ACTION_STARTING_MOVING;
  self->target = RW_TARGET_PLAYHEAD;
}

void
editor_ruler_on_drag_update (
  RulerWidget * self,
  gdouble             offset_x,
  gdouble             offset_y)
{
  g_return_if_fail (self);

  if (ACTION_IS (MOVING))
    {
      Position editor_pos;
      Position region_local_pos;
      ZRegion * r =
        clip_editor_get_region (CLIP_EDITOR);
      ArrangerObject * r_obj =
        (ArrangerObject *) r;

      /* convert px to position */
      if (self)
        ui_px_to_pos_editor (
          self->start_x + offset_x,
          &editor_pos, 1);

      /* snap if not shift held */
      if (!self->shift_held)
        position_snap_simple (
          &editor_pos, SNAP_GRID_MIDI);

      position_from_ticks (
        &region_local_pos,
        position_to_ticks (&editor_pos) -
        position_to_ticks (&r_obj->pos));

      if (TARGET_IS (LOOP_START))
        {
          /* make the position local to the region
           * for less calculations */
          /* if position is acceptable */
          if (position_compare (
                &region_local_pos,
                &r_obj->loop_end_pos) < 0 &&
              position_compare (
                &region_local_pos,
                &r_obj->clip_start_pos) >= 0)
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_START,
                F_VALIDATE);
              transport_update_position_frames (
                TRANSPORT);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (LOOP_END))
        {
          /* if position is acceptable */
          if (position_compare (
                &region_local_pos,
                &r_obj->loop_start_pos) > 0 &&
              position_compare (
                &region_local_pos,
                &r_obj->clip_start_pos) > 0)
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
                F_VALIDATE);
              transport_update_position_frames (
                TRANSPORT);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (CLIP_START))
        {
          /* if position is acceptable */
          if (position_is_before_or_equal (
                &region_local_pos,
                &r_obj->loop_start_pos))
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_CLIP_START,
                F_VALIDATE);
              transport_update_position_frames (
                TRANSPORT);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (PLAYHEAD))
        {
          Position timeline_start,
                   timeline_end;
          position_init (&timeline_start);
          position_init (&timeline_end);
          position_add_bars (
            &timeline_end,
            TRANSPORT->total_bars);

          /* if position is acceptable */
          if (position_is_after_or_equal (
                &editor_pos, &timeline_start) &&
              position_is_before_or_equal (
                &editor_pos, &timeline_end))
            {
              transport_move_playhead (
                TRANSPORT, &editor_pos,
                F_PANIC, F_NO_SET_CUE_POINT);
              self->last_set_pos = editor_pos;
              ruler_widget_redraw_whole (self);
              EVENTS_PUSH (
                ET_PLAYHEAD_POS_CHANGED_MANUALLY,
                NULL);
            }

          /*ruler_marker_widget_update_tooltip (*/
            /*self->playhead, 1);*/
        }
    }
}

void
editor_ruler_on_drag_end (
  RulerWidget * self)
{
  /* hide tooltips */
  /*if (TARGET_IS (PLAYHEAD))*/
    /*{*/
      /*ruler_marker_widget_update_tooltip (*/
        /*self->playhead, 0);*/
    /*}*/
  if ((ACTION_IS (MOVING) ||
         ACTION_IS (STARTING_MOVING)) &&
      TARGET_IS (PLAYHEAD))
    {
      /* set cue point */
      position_set_to_pos (
        &TRANSPORT->cue_pos,
        &self->last_set_pos);

      EVENTS_PUSH (
        ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
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
  ui_px_to_pos_editor (
    x_start, &p1, true);
  ui_px_to_pos_editor (
    x_end, &p2, true);
  Track * track =
    clip_editor_get_track (CLIP_EDITOR);
  g_return_val_if_fail (track, 0);

  return
    track_get_regions_in_range (
      track, &p1, &p2, regions);
}
