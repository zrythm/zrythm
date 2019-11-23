/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/events.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/audio_region.h"
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

  Region * region;
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

  Region * region;
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

  Region * region;
  for (i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      CHECK_POS (region->track_pos);
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

  Region * region;
  for (i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      region->track_pos =
        tracklist_get_visible_track_diff (
          TRACKLIST, highest_tr,
          TRACKLIST->tracks[region->track_pos]);
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
  Region * region;
  for (i = 0; i < ts->num_regions; i++)
    {
      region = ts->regions[i];
      g_warn_if_fail (region->track_pos >= 0);
      if (!array_contains_int (
            poses, *num_poses, region->track_pos))
        {
          array_append (
            poses, *num_poses, region->track_pos);
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
  Region * r;
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
          if (r->track_pos != ts_pos)
            continue;

           /*check if this track can host this*/
           /*region*/
          if (!track_type_can_host_region_type (
                tr->type, r->type))
            {
              g_message (
                "track %s cant host region type %d",
                tr->name, r->type);
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

  long pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  arranger_selections_get_start_pos (
    (ArrangerSelections *) ts, &start_pos,
    F_GLOBAL);
  long start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  long curr_ticks;
  int i;
  for (i = 0; i < ts->num_regions; i++)
    {
      Region * region = ts->regions[i];
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      Track * region_track =
        tracklist_get_visible_track_after_delta (
          TRACKLIST, track, region->track_pos);
      g_return_if_fail (region_track);

      /* update positions */
      curr_ticks =
        position_to_ticks (&r_obj->pos);
      position_from_ticks (
        &r_obj->pos, pos_ticks + DIFF);
      curr_ticks =
        position_to_ticks (&r_obj->end_pos);
      position_from_ticks (
        &r_obj->end_pos, pos_ticks + DIFF);
      /* TODO */
      /*position_set_to_pos (&region->unit_end_pos,*/
                           /*&region->end_pos);*/
  g_message ("[in loop]num regions %d num midi notes %d",
             ts->num_regions,
             ts->regions[0]->num_midi_notes);

      /* same for midi notes */
      g_message ("region type %d", region->type);
      if (region->type == REGION_TYPE_MIDI)
        {
          MidiRegion * mr = region;
          g_message ("HELLO?");
          g_message ("num midi notes here %d",
                     mr->num_midi_notes);
          for (int j = 0; j < mr->num_midi_notes; j++)
            {
              MidiNote * mn = mr->midi_notes[j];
              ArrangerObject * mn_obj =
                (ArrangerObject *) mn;
              g_message ("old midi start");
              /*position_print_yaml (&mn->start_pos);*/
              g_message ("bars %d",
                         mn_obj->pos.bars);
              g_message ("new midi start");
              ADJUST_POSITION (&mn_obj->pos);
              position_print_yaml (
                &mn_obj->pos);
              g_message ("old midi start");
              ADJUST_POSITION (&mn_obj->end_pos);
              position_print_yaml (&mn_obj->end_pos);
            }
        }

      /* TODO automation points */

      /* clone and add to track */
      Region * cp =
        (Region *)
        arranger_object_clone (
          r_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      /* FIXME does not with automation regions */
      track_add_region (
        region_track, cp, NULL, region->lane_pos,
        F_GEN_NAME, F_PUBLISH_EVENTS);

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
      position_from_ticks (
        &s_obj->pos, pos_ticks + DIFF);

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
      position_from_ticks (
        &m_obj->pos, pos_ticks + DIFF);

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

SERIALIZE_SRC (
  TimelineSelections, timeline_selections)
DESERIALIZE_SRC (
  TimelineSelections, timeline_selections)
PRINT_YAML_SRC (
  TimelineSelections, timeline_selections)
