/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/event_manager.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/midi_region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

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
  int tmp_pos, i;

#define CHECK_POS(_pos) \
  tmp_pos = _pos; \
  if (tmp_pos < track_pos) \
    { \
      track_pos = tmp_pos; \
    }

  ZRegion * region;
  for (i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      CHECK_POS (region->id.track_pos);
    }
  if (ts->num_scale_objects > 0)
    {
      CHECK_POS (ts->chord_track_vis_index);
    }
  if (ts->num_markers > 0)
    {
      CHECK_POS (ts->marker_track_vis_index);
    }

#undef CHECK_POS

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

  ZRegion * region;
  for (i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      region->id.track_pos =
        tracklist_get_visible_track_diff (
          TRACKLIST, highest_tr,
          TRACKLIST->tracks[region->id.track_pos]);
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
  ZRegion * region;
  for (i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      g_warn_if_fail (region->id.track_pos >= 0);
      if (!array_contains_int (
            poses, *num_poses, region->id.track_pos))
        {
          array_append (
            poses, *num_poses, region->id.track_pos);
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
  g_return_val_if_fail (
    cur_track && num_poses > 0 &&
    lowest_track_pos < INT_MAX, 0);

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
          return 0;
        }

      /*[> check if content matches <]*/
      for (j = 0; j < ts->num_regions; j++)
        {
          /*[> get region at this ts_pos <]*/
          r = ts->regions[j];
          if (r->id.track_pos != ts_pos)
            continue;

           /*check if this track can host this*/
           /*region*/
          if (!track_type_can_host_region_type (
                tr->type, r->id.type))
            {
              g_message (
                "track %s cant host region type %d",
                tr->name, r->id.type);
              return 0;
            }
        }

      /*[> check for chord track/marker track too <]*/
      if (ts->num_scale_objects > 0 &&
          ts_pos == ts->chord_track_vis_index &&
          tr->type != TRACK_TYPE_CHORD)
        return 0;
      if (ts->num_markers > 0 &&
          ts_pos == ts->marker_track_vis_index &&
          tr->type != TRACK_TYPE_MARKER)
        return 0;
    }

  return 1;
}

void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
  Position *           pos)
{
  Track * track =
    TRACKLIST_SELECTIONS->tracks[0];

  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS);

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

void
timeline_selections_mark_for_bounce (
  TimelineSelections * ts)
{
  engine_reset_bounce_mode (AUDIO_ENGINE);

  for (int i = 0; i < ts->num_regions; i++)
    {
      ZRegion * r = ts->regions[i];
      Track * track =
        arranger_object_get_track (
          (ArrangerObject *) r);
      g_return_if_fail (track);

      track->bounce = 1;
      r->bounce = 1;
    }
}

SERIALIZE_SRC (
  TimelineSelections, timeline_selections)
DESERIALIZE_SRC (
  TimelineSelections, timeline_selections)
PRINT_YAML_SRC (
  TimelineSelections, timeline_selections)
