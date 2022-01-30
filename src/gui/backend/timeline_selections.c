/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <limits.h>

#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/marker_track.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/midi_region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Creates a new TimelineSelections instance for
 * the given range.
 *
 * @bool clone_objs True to clone each object,
 *   false to use pointers to project objects.
 */
TimelineSelections *
timeline_selections_new_for_range (
  Position * start_pos,
  Position * end_pos,
  bool       clone_objs)
{
  TimelineSelections * self =
    (TimelineSelections *)
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_TIMELINE);

#define ADD_OBJ(obj) \
  if (arranger_object_is_hit ( \
        (ArrangerObject *) (obj), \
        start_pos, end_pos)) \
    { \
      ArrangerObject * _obj = \
        (ArrangerObject *) (obj); \
      if (clone_objs) \
        { \
          _obj =  \
          arranger_object_clone (_obj); \
        } \
      arranger_selections_add_object ( \
        (ArrangerSelections *) self, _obj); \
    }

  /* regions */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      AutomationTracklist * atl =
        track_get_automation_tracklist (track);
      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];
          for (int k = 0; k < lane->num_regions; k++)
            {
              ADD_OBJ (lane->regions[k]);
            }
        }
      for (int j = 0; j < track->num_chord_regions;
           j++)
        {
          ADD_OBJ (track->chord_regions[j]);
        }
      if (atl)
        {
          for (int j = 0; j < atl->num_ats; j++)
            {
              AutomationTrack * at = atl->ats[j];

              for (int k = 0; k < at->num_regions;
                   k++)
                {
                  ADD_OBJ (at->regions[k]);
                }
            }
        }
      for (int j = 0; j < track->num_scales; j++)
        {
          ADD_OBJ (track->scales[j]);
        }
      for (int j = 0; j < track->num_markers; j++)
        {
          ADD_OBJ (track->markers[j]);
        }
    }

#undef ADD_OBJ

  return self;
}

/**
 * Gets highest track in the selections.
 */
Track *
timeline_selections_get_last_track (
  TimelineSelections * ts)
{
  int track_pos = -1;
  Track * track = NULL;
  int tmp_pos;
  Track * tmp_track;

#define CHECK_POS(_track) \
  tmp_track = _track; \
  tmp_pos = \
    tracklist_get_track_pos ( \
      TRACKLIST, tmp_track); \
  if (tmp_pos > track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  ZRegion * region;
  for (int i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      Track * _track =
        arranger_object_get_track (r_obj);
      CHECK_POS (_track);
    }
  CHECK_POS (P_CHORD_TRACK);

  return track;
#undef CHECK_POS
}

/**
 * Gets lowest track in the selections.
 */
Track *
timeline_selections_get_first_track (
  TimelineSelections * ts)
{
  int track_pos = INT_MAX;
  Track * track = NULL;
  int tmp_pos;
  Track * tmp_track;

#define CHECK_POS(_track) \
  tmp_track = _track; \
  tmp_pos = \
    tracklist_get_track_pos ( \
      TRACKLIST, tmp_track); \
  if (tmp_pos < track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  ZRegion * region;
  for (int i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      Track * _track =
        arranger_object_get_track (r_obj);
      CHECK_POS (_track);
    }
  if (ts->num_scale_objects > 0)
    {
      CHECK_POS (P_CHORD_TRACK);
    }
  if (ts->num_markers > 0)
    {
      CHECK_POS (P_MARKER_TRACK);
    }

  return track;
#undef CHECK_POS
}

/**
 * Gets index of the lowest track in the selections.
 *
 * Used during pasting.
 */
static int
get_lowest_track_pos (
  TimelineSelections * ts)
{
  int track_pos = INT_MAX;

#define CHECK_POS(id) \
  { \
  }

  for (int i = 0; i < ts->num_regions; i++)
    {
      ZRegion * r = ts->regions[i];
      Track * tr =
        tracklist_find_track_by_name_hash (
          TRACKLIST, r->id.track_name_hash);
      if (tr->pos < track_pos)
        {
          track_pos = tr->pos;
        }
    }

  if (ts->num_scale_objects > 0)
    {
      if (ts->chord_track_vis_index < track_pos)
        track_pos = ts->chord_track_vis_index;
    }
  if (ts->num_markers > 0)
    {
      if (ts->marker_track_vis_index < track_pos)
        track_pos = ts->marker_track_vis_index;
    }

  return track_pos;
}

/**
 * Replaces the track positions in each object with
 * visible track indices starting from 0.
 *
 * Used during copying.
 */
void
timeline_selections_set_vis_track_indices (
  TimelineSelections * ts)
{
  int i;
  Track * highest_tr =
    timeline_selections_get_first_track (ts);

  for (i = 0; i < ts->num_regions; i++)
    {
      ZRegion * r = ts->regions[i];
      Track * region_track =
        arranger_object_get_track (
          (ArrangerObject *) r);
      ts->region_track_vis_index =
        tracklist_get_visible_track_diff (
          TRACKLIST, highest_tr, region_track);
    }
  if (ts->num_scale_objects > 0)
    ts->chord_track_vis_index =
      tracklist_get_visible_track_diff (
        TRACKLIST, highest_tr, P_CHORD_TRACK);
  if (ts->num_markers > 0)
    ts->marker_track_vis_index =
      tracklist_get_visible_track_diff (
        TRACKLIST, highest_tr, P_MARKER_TRACK);
}

/**
 * Saves the track positions in the poses array
 * and the size in num_poses.
 */
static void
get_track_poses (
  TimelineSelections * ts,
  int *                poses,
  int *                num_poses)
{
  int i;
  for (i = 0; i < ts->num_regions; i++)
    {
      ZRegion * r = ts->regions[i];
      Track * region_track =
        arranger_object_get_track (
          (ArrangerObject *) r);
      if (!array_contains_int (
            poses, *num_poses, region_track->pos))
        {
          array_append (
            poses, *num_poses, region_track->pos);
        }
    }
  if (ts->num_scale_objects > 0)
    if (!array_contains_int (
          poses, *num_poses,
          ts->chord_track_vis_index))
      {
        array_append (
          poses, *num_poses,
          ts->chord_track_vis_index);
      }
  if (ts->num_markers > 0)
    if (!array_contains_int (
          poses, *num_poses,
          ts->marker_track_vis_index))
      {
        array_append (
          poses, *num_poses,
          ts->marker_track_vis_index);
      }
}

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param idx Track index to start pasting to.
 */
int
timeline_selections_can_be_pasted (
  TimelineSelections * ts,
  Position *           pos,
  const int            idx)
{
  int i, j;
  ZRegion * r;
  /*Marker * m;*/
  /*ScaleObject * s;*/
  int lowest_track_pos =
    get_lowest_track_pos (ts);
  Track * cur_track =
    TRACKLIST_SELECTIONS->tracks[0];
  int poses[800];
  int num_poses = 0;
  get_track_poses (ts, poses, &num_poses);
  if (!cur_track || num_poses <= 0
      || lowest_track_pos == INT_MAX)
    return false;

  /*check if enough visible tracks exist and the*/
  /*content can be pasted in each*/
  int ts_pos;
  for (i = 0; i < num_poses; i++)
    {
      ts_pos = poses[i];
      g_return_val_if_fail (ts_pos >= 0, 0);

      /*[> track at the new position <]*/
      Track * tr =
        tracklist_get_visible_track_after_delta (
          TRACKLIST, cur_track, ts_pos);
      if (!tr)
        {
          g_message (
            "no track at current pos (%d) + "
            "ts pos (%d)",
            cur_track->pos, ts_pos);
          return false;
        }

      /* check if content matches */
      for (j = 0; j < ts->num_regions; j++)
        {
          /* get region at this ts_pos */
          r = ts->regions[j];
          Track * region_track =
            arranger_object_get_track (
              (ArrangerObject *) r);
          if (region_track->pos != ts_pos)
            continue;

           /*check if this track can host this*/
           /*region*/
          if (!track_type_can_host_region_type (
                tr->type, r->id.type))
            {
              g_message (
                "track %s can't host region type %d",
                tr->name, r->id.type);
              return false;
            }
        }

      /* check for chord track/marker track too */
      if (ts->num_scale_objects > 0 &&
          ts_pos == ts->chord_track_vis_index &&
          tr->type != TRACK_TYPE_CHORD)
        return false;
      if (ts->num_markers > 0 &&
          ts_pos == ts->marker_track_vis_index &&
          tr->type != TRACK_TYPE_MARKER)
        return false;
    }

  return true;
}

#if 0
void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
  Position *           pos)
{
  Track * track =
    TRACKLIST_SELECTIONS->tracks[0];

  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS, F_NO_FREE);

  double pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  arranger_selections_get_start_pos (
    (ArrangerSelections *) ts, &start_pos,
    F_GLOBAL);
  double start_pos_ticks =
    position_to_ticks (&start_pos);

  double curr_ticks, diff;
  int i;
  for (i = 0; i < ts->num_regions; i++)
    {
      ZRegion * region = ts->regions[i];
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      Track * region_track =
        tracklist_get_visible_track_after_delta (
          TRACKLIST, track, region->id.track_pos);
      g_return_if_fail (region_track);

      /* update positions */
      curr_ticks =
        position_to_ticks (&r_obj->pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &r_obj->pos, pos_ticks + diff);
      curr_ticks =
        position_to_ticks (&r_obj->end_pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &r_obj->end_pos, pos_ticks + diff);
      /* TODO */

      /* clone and add to track */
      ZRegion * cp =
        (ZRegion *)
        arranger_object_clone (
          r_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);

      switch (region->id.type)
        {
        case REGION_TYPE_MIDI:
        case REGION_TYPE_AUDIO:
          track_add_region (
            region_track, cp, NULL,
            region->id.lane_pos,
            F_GEN_NAME, F_PUBLISH_EVENTS);
          break;
        case REGION_TYPE_AUTOMATION:
          {
            AutomationTrack * at =
              region_track->automation_tracklist.
                ats[region->id.at_idx];
            g_return_if_fail (at);
            track_add_region (
              region_track, cp, at, -1,
              F_GEN_NAME, F_PUBLISH_EVENTS);
          }
          break;
        case REGION_TYPE_CHORD:
          track_add_region (
            region_track, cp, NULL, -1,
            F_GEN_NAME, F_PUBLISH_EVENTS);
          break;
        }

      /* select it */
      arranger_object_select (
        (ArrangerObject *) cp, F_SELECT,
        F_APPEND);
    }
  for (i = 0; i < ts->num_scale_objects; i++)
    {
      ScaleObject * scale = ts->scale_objects[i];
      ArrangerObject * s_obj =
        (ArrangerObject *) scale;

      curr_ticks =
        position_to_ticks (&s_obj->pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &s_obj->pos, pos_ticks + diff);

      /* clone and add to track */
      ScaleObject * clone =
        (ScaleObject *)
        arranger_object_clone (
          s_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      chord_track_add_scale (
        P_CHORD_TRACK, clone);

      /* select it */
      arranger_object_select (
        (ArrangerObject *) clone, F_SELECT,
        F_APPEND);
    }
  for (i = 0; i < ts->num_markers; i++)
    {
      Marker * m = ts->markers[i];
      ArrangerObject * m_obj =
        (ArrangerObject *) m;

      curr_ticks =
        position_to_ticks (&m_obj->pos);
      diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &m_obj->pos, pos_ticks + diff);

      /* clone and add to track */
      Marker * clone =
        (Marker *)
        arranger_object_clone (
          m_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      marker_track_add_marker (
        P_MARKER_TRACK, clone);

      /* select it */
      arranger_object_select (
        (ArrangerObject *) clone, F_SELECT,
        F_APPEND);
    }
#undef DIFF
}
#endif

/**
 * @param with_parents Also mark all the track's
 *   parents recursively.
 */
void
timeline_selections_mark_for_bounce (
  TimelineSelections * ts,
  bool                 with_parents)
{
  engine_reset_bounce_mode (AUDIO_ENGINE);

  for (int i = 0; i < ts->num_regions; i++)
    {
      ZRegion * r = ts->regions[i];
      Track * track =
        arranger_object_get_track (
          (ArrangerObject *) r);
      g_return_if_fail (track);

      track_mark_for_bounce (
        track, F_BOUNCE, F_NO_MARK_REGIONS,
        F_MARK_CHILDREN, with_parents);
      r->bounce = 1;
    }
}

/**
 * Move the selected Regions to new Lanes.
 *
 * @param diff The delta to move the
 *   Tracks.
 *
 * @return True if moved.
 */
bool
timeline_selections_move_regions_to_new_lanes (
  TimelineSelections * self,
  const int            diff)
{
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self, false);

  /* store selected regions because they will be
   * deselected during moving */
  ZRegion * regions[600];
  int num_regions = 0;
  ZRegion * region;
  for (int i = 0; i < self->num_regions; i++)
    {
      regions[num_regions++] =
        self->regions[i];
    }

  /* check that:
   * - all regions are in the same track
   * - only lane regions are selected
   * - the lane bounds are not exceeded */
  bool compatible = true;
  for (int i = 0; i < num_regions; i++)
    {
      region = regions[i];
      if (region->id.lane_pos + diff < 0)
        {
          compatible = false;
          break;
        }
    }
  if (self->num_scale_objects > 0 ||
      self->num_markers > 0)
    {
      compatible = false;
    }
  if (!compatible)
    return false;

  /* new positions are all compatible, move the
   * regions */
  for (int i = 0; i < num_regions; i++)
    {
      region = regions[i];
      TrackLane * lane = region_get_lane (region);
      g_return_val_if_fail (region && lane, -1);

      TrackLane * lane_to_move_to = NULL;
      int new_lane_pos =
        lane->pos +  diff;
      g_return_val_if_fail (
        new_lane_pos >= 0, -1);
      Track * track = track_lane_get_track (lane);
      track_create_missing_lanes (
        track, new_lane_pos);
      lane_to_move_to =
        track->lanes[new_lane_pos];
      g_warn_if_fail (lane_to_move_to);

      region_move_to_lane (
        region, lane_to_move_to, -1);
    }

  EVENTS_PUSH (
    ET_TRACK_LANES_VISIBILITY_CHANGED, NULL);

  return true;
}

/**
 * Move the selected Regions to the new Track.
 *
 * @param new_track_is_before 1 if the Region's
 *   should move to their previous tracks, 0 for
 *   their next tracks.
 *
 * @return True if moved.
 */
bool
timeline_selections_move_regions_to_new_tracks (
  TimelineSelections * self,
  const int            vis_track_diff)
{
  g_debug (
    "moving %d regions to new tracks "
    "(visible track diff %d)...",
    self->num_regions, vis_track_diff);

  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self, false);

  /* store selected regions because they will be
   * deselected during moving */
  ZRegion * regions[600];
  int num_regions = 0;
  ZRegion * region;
  ArrangerObject * r_obj;
  for (int i = 0; i < self->num_regions; i++)
    {
      regions[num_regions++] =
        self->regions[i];
    }

  /* check that all regions can be moved to a
   * compatible track */
  bool compatible = true;
  for (int i = 0; i < num_regions; i++)
    {
      region = regions[i];
      r_obj = (ArrangerObject *) region;
      Track * region_track =
        arranger_object_get_track (r_obj);
      Track * visible =
        tracklist_get_visible_track_after_delta (
          TRACKLIST,
          region_track,
          vis_track_diff);
      if (
        !visible ||
        !track_type_is_compatible_for_moving (
           region_track->type,
           visible->type) ||
        /* do not allow moving automation tracks
         * to other tracks for now */
        region->id.type == REGION_TYPE_AUTOMATION)
        {
          compatible = false;
          break;
        }
    }
  if (!compatible)
    {
      return false;
    }

  /* new positions are all compatible, move the
   * regions */
  for (int i = 0; i < num_regions; i++)
    {
      region = regions[i];
      r_obj = (ArrangerObject *) region;
      Track * region_track =
        arranger_object_get_track (r_obj);
      g_warn_if_fail (region && region_track);
      Track * track_to_move_to =
        tracklist_get_visible_track_after_delta (
          TRACKLIST,
          region_track,
          vis_track_diff);
      g_warn_if_fail (track_to_move_to);

      region_move_to_track (
        region, track_to_move_to, -1);
    }

  g_debug ("moved");

  return true;
}

/**
 * Sets the regions'
 * \ref ZRegion.index_in_prev_lane.
 */
void
timeline_selections_set_index_in_prev_lane (
  TimelineSelections * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * r = self->regions[i];
      ArrangerObject * r_obj = (ArrangerObject *) r;
      r_obj->index_in_prev_lane = r->id.idx;
    }
}

bool
timeline_selections_contains_only_regions (
  TimelineSelections * self)
{
  return
    self->num_regions > 0
    && self->num_scale_objects == 0
    && self->num_markers == 0 ;
}

bool
timeline_selections_contains_only_region_types (
  TimelineSelections * self,
  RegionType           types)
{
  if (!timeline_selections_contains_only_regions (self))
    return false;

  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * r = self->regions[i];
      if (!(types & r->id.type))
        return false;
    }
  return true;
}
