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

void
timeline_selections_init_loaded (
  TimelineSelections * ts)
{
  int i;

#define _SET_OBJ(sc) \
  for (i = 0; i < ts->num_##sc##s; i++) \
    ts->sc##s[i] = sc##_find (ts->sc##s[i]);

  _SET_OBJ (region);
  _SET_OBJ (marker);
  _SET_OBJ (scale_object);

#undef _SET_OBJ
}

/**
 * Returns if there are any selections.
 */
int
timeline_selections_has_any (
  TimelineSelections * ts)
{
  return (
    ts->num_regions > 0 ||
    ts->num_scale_objects > 0 ||
    ts->num_markers > 0);
}

/**
 * Code to run after deserializing.
 *
 * Used for copy-paste.
 */
void
timeline_selections_post_deserialize (
  TimelineSelections * ts)
{
  int i;

#define _SET_OBJ(sc) \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc##_post_deserialize (ts->sc##s[i]); \
    }

  _SET_OBJ (region);
  _SET_OBJ (marker);
  _SET_OBJ (scale_object);

#undef _SET_OBJ
}

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
void
timeline_selections_set_cache_poses (
  TimelineSelections * ts)
{
  int i;

#define SET_CACHE_POS_W_LENGTH(cc,sc) \
  cc * sc; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_set_cache_start_pos ( \
        sc, &sc->start_pos); \
      sc##_set_cache_end_pos ( \
        sc, &sc->end_pos); \
    }

#define SET_CACHE_POS(cc,sc) \
  cc * sc; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_set_cache_pos ( \
        sc, &sc->pos); \
    }

  SET_CACHE_POS_W_LENGTH (Region, region);
  SET_CACHE_POS (ScaleObject, scale_object);
  SET_CACHE_POS (Marker, marker);

#undef SET_CACHE_POS_W_LENGTH
#undef SET_CACHE_POS
}

/**
 * Returns the position of the leftmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 */
void
timeline_selections_get_start_pos (
  TimelineSelections * ts,
  Position *           pos,
  int                  transient)
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);
  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Region, region, start_pos,
    transient, before, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ScaleObject, scale_object, pos,
    transient, before, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Marker, marker, pos,
    transient, before, widget);
}

/**
 * Returns the position of the rightmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 */
void
timeline_selections_get_end_pos (
  TimelineSelections * ts,
  Position *           pos,
  int                  transient)
{
  position_init (pos);
  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Region, region, end_pos,
    transient, after, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ScaleObject, scale_object, pos,
    transient, after, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Marker, marker, pos,
    transient, after, widget);
}

/**
 * Gets first object's widget.
 *
 * If transient is 1, transient objects are checked
 * instead.
 */
GtkWidget *
timeline_selections_get_first_object (
  TimelineSelections * ts,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (
    pos, TRANSPORT->total_bars);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Region, region, start_pos,
    transient, before, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ScaleObject, scale_object, pos,
    transient, before, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Marker, marker, pos,
    transient, before, widget);

  return widget;
}

/**
 * Gets last object's widget.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
GtkWidget *
timeline_selections_get_last_object (
  TimelineSelections * ts,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_init (pos);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Region, region, start_pos,
    transient, after, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ScaleObject, scale_object, pos,
    transient, after, widget);
  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, Marker, marker, pos,
    transient, after, widget);

  return widget;
}

ARRANGER_SELECTIONS_DECLARE_RESET_COUNTERPARTS (
  Timeline, timeline)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      region_reset_counterpart (
        self->regions[i], reset_trans);
    }
}

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_last_track (
  TimelineSelections * ts,
  int                  transient)
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
      if (transient)
        region = ts->regions[i]->obj_info.main_trans;
      else
        region = ts->regions[i]->obj_info.main;
      CHECK_POS (region_get_track (region));
    }
  CHECK_POS (P_CHORD_TRACK);

  return track;
#undef CHECK_POS
}

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_first_track (
  TimelineSelections * ts,
  int                  transient)
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
      if (transient)
        region = ts->regions[i]->obj_info.main_trans;
      else
        region = ts->regions[i]->obj_info.main;
      CHECK_POS (region_get_track (region));
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
 * Adds an object to the selections.
 */
#define DEFINE_ADD_OBJECT(caps,cc,sc) \
  void \
  timeline_selections_add_##sc ( \
    TimelineSelections * ts, \
    cc *                 sc) \
  { \
    ARRANGER_SELECTIONS_ADD_OBJECT (ts, caps, sc); \
  }

DEFINE_ADD_OBJECT (REGION, Region, region);
DEFINE_ADD_OBJECT (
  SCALE_OBJECT, ScaleObject, scale_object);
DEFINE_ADD_OBJECT (MARKER, Marker, marker);

#undef DEFINE_ADD_OBJECT

#define DEFINE_REMOVE_OBJ(caps,cc,sc) \
  void \
  timeline_selections_remove_##sc ( \
    TimelineSelections * ts, \
    cc *                 sc) \
  { \
    ARRANGER_SELECTIONS_REMOVE_OBJECT (ts, caps, sc); \
  }

DEFINE_REMOVE_OBJ (REGION, Region, region);
DEFINE_REMOVE_OBJ (
  SCALE_OBJECT, ScaleObject, scale_object);
DEFINE_REMOVE_OBJ (MARKER, Marker, marker);

#undef DEFINE_REMOVE_OBJ

#define DEFINE_CONTAINS_OBJ(caps,cc,sc) \
  int \
  timeline_selections_contains_##sc ( \
    TimelineSelections * self, \
    cc *                 sc) \
  { \
    return \
      array_contains ( \
        self->sc##s, self->num_##sc##s, sc); \
  }

DEFINE_CONTAINS_OBJ (REGION, Region, region);
DEFINE_CONTAINS_OBJ (
  SCALE_OBJECT, ScaleObject, scale_object);
DEFINE_CONTAINS_OBJ (MARKER, Marker, marker);

#undef DEFINE_CONTAINS_OBJ

/**
 * Clears selections.
 */
void
timeline_selections_clear (
  TimelineSelections * ts)
{
  int i;

/* use caches because ts->* will be operated on. */
#define TL_REMOVE_OBJS(caps,cc,sc) \
  int num_##sc##s; \
  cc * sc; \
  static cc * sc##s[600]; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc##s[i] = ts->sc##s[i]; \
    } \
  num_##sc##s = ts->num_##sc##s; \
  for (i = 0; i < num_##sc##s; i++) \
    { \
      sc = sc##s[i]; \
      timeline_selections_remove_##sc (ts, sc); \
      EVENTS_PUSH ( \
        ET_ARRANGER_OBJECT_SELECTION_CHANGED, \
        &sc->obj_info); \
    }

  TL_REMOVE_OBJS (
    REGION, Region, region);
  TL_REMOVE_OBJS (
    SCALE_OBJECT, ScaleObject, scale_object);
  TL_REMOVE_OBJS (
    MARKER, Marker, marker);

#undef TL_REMOVE_OBJS
}

/**
 * Clone the struct for copying, undoing, etc.
 */
TimelineSelections *
timeline_selections_clone (
  const TimelineSelections * src)
{
  TimelineSelections * new_ts =
    calloc (1, sizeof (TimelineSelections));

  int i;

#define TL_CLONE_OBJS(caps,cc,sc) \
  cc * sc, * new_##sc; \
  for (i = 0; i < src->num_##sc##s; i++) \
    { \
      sc = src->sc##s[i]; \
      new_##sc = \
        sc##_clone (sc, caps##_CLONE_COPY_MAIN); \
      array_append ( \
        new_ts->sc##s, new_ts->num_##sc##s, \
        new_##sc); \
    }

  TL_CLONE_OBJS (
    REGION, Region, region);
  TL_CLONE_OBJS (
    SCALE_OBJECT, ScaleObject, scale_object);
  TL_CLONE_OBJS (
    MARKER, Marker, marker);

#undef TL_CLONE_OBJS

  return new_ts;
}

/**
 * Moves the TimelineSelections by the given
 * amount of ticks.
 *
 * @param ticks Ticks to add.
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
timeline_selections_add_ticks (
  TimelineSelections * ts,
  long                 ticks,
  int                  use_cached_pos,
  ArrangerObjectUpdateFlag update_flag)
{
  int i;

#define UPDATE_TL_POSES(cc,sc) \
  cc * sc; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_move ( \
        sc, ticks, use_cached_pos, \
        update_flag); \
    }

  UPDATE_TL_POSES (
    Region, region);
  UPDATE_TL_POSES (
    ScaleObject, scale_object);
  UPDATE_TL_POSES (
    Marker, marker);

#undef UPDATE_TL_POSES
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
    timeline_selections_get_first_track (ts, 0);

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

  timeline_selections_clear (
    TL_SELECTIONS);

  long pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  timeline_selections_get_start_pos (
    ts, &start_pos, F_NO_TRANSIENTS);
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
      Track * region_track =
        tracklist_get_visible_track_after_delta (
          TRACKLIST, track, region->track_pos);
      g_return_if_fail (region_track);

      /* update positions */
      curr_ticks =
        position_to_ticks (&region->start_pos);
      position_from_ticks (
        &region->start_pos,
        pos_ticks + DIFF);
      curr_ticks =
        position_to_ticks (&region->end_pos);
      position_from_ticks (
        &region->end_pos,
        pos_ticks + DIFF);
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
              g_message ("old midi start");
              /*position_print_yaml (&mn->start_pos);*/
              g_message ("bars %d",
                         mn->start_pos.bars);
              g_message ("new midi start");
              ADJUST_POSITION (&mn->start_pos);
              position_print_yaml (&mn->start_pos);
              g_message ("old midi start");
              ADJUST_POSITION (&mn->end_pos);
              position_print_yaml (&mn->end_pos);
            }
        }

      /* TODO automation points */

      /* clone and add to track */
      Region * cp =
        region_clone (
          region, REGION_CLONE_COPY_MAIN);
      /* FIXME does not with automation regions */
      track_add_region (
        region_track, cp, NULL, region->lane_pos,
        F_GEN_NAME, F_PUBLISH_EVENTS);

      /* select it */
      timeline_selections_add_region (
        TL_SELECTIONS, cp);
    }
  for (i = 0; i < ts->num_scale_objects; i++)
    {
      ScaleObject * scale = ts->scale_objects[i];

      curr_ticks = position_to_ticks (&scale->pos);
      position_from_ticks (&scale->pos,
                           pos_ticks + DIFF);

      /* clone and add to track */
      ScaleObject * clone =
        scale_object_clone (
          scale, SCALE_OBJECT_CLONE_COPY_MAIN);
      chord_track_add_scale (
        P_CHORD_TRACK, clone);

      /* select it */
      timeline_selections_add_scale_object (
        TL_SELECTIONS, clone);
    }
  for (i = 0; i < ts->num_markers; i++)
    {
      Marker * m = ts->markers[i];

      curr_ticks = position_to_ticks (&m->pos);
      position_from_ticks (
        &m->pos, pos_ticks + DIFF);

      /* clone and add to track */
      Marker * clone =
        marker_clone (
          m, MARKER_CLONE_COPY_MAIN);
      marker_track_add_marker (
        P_MARKER_TRACK, clone);

      /* select it */
      timeline_selections_add_marker (
        TL_SELECTIONS, clone);
    }
#undef DIFF
}

void
timeline_selections_free_full (
  TimelineSelections * self)
{
  int i;
  for (i = 0; i < self->num_regions; i++)
    {
      region_free (self->regions[i]);
    }
  for (i = 0; i < self->num_markers; i++)
    {
      marker_free (self->markers[i]);
    }
  for (i = 0; i < self->num_scale_objects; i++)
    {
      scale_object_free (self->scale_objects[i]);
    }
}

void
timeline_selections_free (
  TimelineSelections * self)
{
  free (self);
}

SERIALIZE_SRC (
  TimelineSelections, timeline_selections)
DESERIALIZE_SRC (
  TimelineSelections, timeline_selections)
PRINT_YAML_SRC (
  TimelineSelections, timeline_selections)
