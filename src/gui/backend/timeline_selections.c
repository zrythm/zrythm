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

#include "audio/chord_track.h"
#include "audio/engine.h"
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

#define ADD_OBJECT(caps,sc) \
  if (!array_contains ( \
         ts->sc##s, ts->num_##sc##s, sc)) \
    { \
      array_append ( \
        ts->sc##s, ts->num_##sc##s, sc); \
      EVENTS_PUSH ( \
        ET_ARRANGER_OBJECT_SELECTION_CHANGED, \
        &sc->obj_info); \
    }

#define REMOVE_OBJECT(caps,sc) \
  if (!array_contains ( \
         ts->sc##s, ts->num_##sc##s, sc)) \
    { \
      EVENTS_PUSH ( \
        ET_ARRANGER_OBJECT_SELECTION_CHANGED, \
        &sc->obj_info); \
      return; \
    } \
  array_delete ( \
    ts->sc##s, ts->num_##sc##s, sc)


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
 * Set all transient Position's to their main
 * counterparts.
 */
void
timeline_selections_reset_transient_poses (
  TimelineSelections * ts)
{
  int i;

#define RESET_TRANS_POS_W_LENGTH(caps,cc,sc) \
  cc * sc; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_set_start_pos ( \
        sc, &sc->start_pos, \
        AO_UPDATE_ALL); \
      sc##_set_end_pos ( \
        sc, &sc->end_pos, \
        AO_UPDATE_ALL); \
    } \
  EVENTS_PUSH (ET_##caps##_POSITIONS_CHANGED, \
               NULL)

#define RESET_TRANS_POS(caps,cc,sc) \
  cc * sc; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_set_pos ( \
        sc, &sc->pos, \
        AO_UPDATE_ALL); \
    } \
  EVENTS_PUSH (ET_##caps##_POSITIONS_CHANGED, \
               NULL)

  RESET_TRANS_POS_W_LENGTH (
    REGION, Region, region);
  RESET_TRANS_POS (
    SCALE_OBJECT, ScaleObject, scale_object);
  RESET_TRANS_POS (
    MARKER, Marker, marker);

#undef RESET_TRANS_POS_W_LENGTH
#undef RESET_TRANS_POS
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
    ts, Region, region, start_pos,
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

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_highest_track (
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
timeline_selections_get_lowest_track (
  TimelineSelections * ts,
  int                  transient)
{
  int track_pos = 8000;
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
  CHECK_POS (P_CHORD_TRACK);

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
    ADD_OBJECT (caps, sc); \
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
    REMOVE_OBJECT (caps, sc); \
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
 * Set all main Position's to their transient
 * counterparts.
 */
void
timeline_selections_set_to_transient_poses (
  TimelineSelections * ts)
{
  int i;

#define SET_TO_TRANS_POS_W_LENGTH(caps,cc,sc) \
  cc * sc, * sc##_trans; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_trans =  \
        sc##_get_main_trans_##sc (sc); \
      sc##_set_start_pos ( \
        sc, &sc##_trans->start_pos, \
        AO_UPDATE_ALL); \
      sc##_set_end_pos ( \
        sc, &sc##_trans->end_pos, \
        AO_UPDATE_ALL); \
    } \
  EVENTS_PUSH (ET_##caps##_POSITIONS_CHANGED, \
               NULL)

#define SET_TO_TRANS_POS(caps,cc,sc) \
  cc * sc, * sc##_trans; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_trans =  \
        sc##_get_main_trans_##sc (sc); \
      sc##_set_pos ( \
        sc, &sc##_trans->pos, \
        AO_UPDATE_ALL); \
    } \
  EVENTS_PUSH (ET_##caps##_POSITIONS_CHANGED, \
               NULL)

  SET_TO_TRANS_POS_W_LENGTH (
    REGION, Region, region);
  SET_TO_TRANS_POS (
    SCALE_OBJECT, ScaleObject, scale_object);
  SET_TO_TRANS_POS (
    MARKER, Marker, marker);

#undef SET_TO_TRANS_POS_W_LENGTH
#undef SET_TO_TRANS_POS
}

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
 * Similar to set_to_transient_poses, but handles
 * values for objects that support them (like
 * AutomationPoint's).
 */
void
timeline_selections_set_to_transient_values (
  TimelineSelections * ts)
{
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

void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
  Position *           pos)
{
  int pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  timeline_selections_get_start_pos (
    ts, &start_pos, F_NO_TRANSIENTS);
  int start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  g_message ("[before loop]num regions %d num midi notes %d",
             ts->num_regions,
             ts->regions[0]->num_midi_notes);

  int curr_ticks, i;
  for (i = 0; i < ts->num_regions; i++)
    {
      Region * region = ts->regions[i];

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
              /*position_print (&mn->start_pos);*/
              g_message ("bars %d",
                         mn->start_pos.bars);
              g_message ("new midi start");
              ADJUST_POSITION (&mn->start_pos);
              position_print (&mn->start_pos);
              g_message ("old midi start");
              ADJUST_POSITION (&mn->end_pos);
              position_print (&mn->end_pos);
            }
        }

      /* clone and add to track */
      Region * cp =
        region_clone (
          region, REGION_CLONE_COPY_MAIN);
      region_print (cp);
      track_add_region (
        cp->lane->track, cp, 0, F_GEN_NAME,
        F_GEN_WIDGET);
    }
  for (i = 0; i < ts->num_scale_objects; i++)
    {
      ScaleObject * scale = ts->scale_objects[i];

      curr_ticks = position_to_ticks (&scale->pos);
      position_from_ticks (&scale->pos,
                           pos_ticks + DIFF);
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
