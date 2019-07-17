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
#include "gui/backend/chord_selections.h"
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
chord_selections_init_loaded (
  ChordSelections * ts)
{
  int i;

#define _SET_OBJ(sc) \
  for (i = 0; i < ts->num_##sc##s; i++) \
    ts->sc##s[i] = sc##_find (ts->sc##s[i]);

  _SET_OBJ (chord_object);

#undef _SET_OBJ
}

/**
 * Returns if there are any selections.
 */
int
chord_selections_has_any (
  ChordSelections * ts)
{
  return (
    ts->num_chord_objects > 0);
}

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
void
chord_selections_set_cache_poses (
  ChordSelections * ts)
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

  SET_CACHE_POS (ChordObject, chord_object);

#undef SET_CACHE_POS_W_LENGTH
#undef SET_CACHE_POS
}

/**
 * Set all transient Position's to their main
 * counterparts.
 */
void
chord_selections_reset_transient_poses (
  ChordSelections * ts)
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

  RESET_TRANS_POS (
    CHORD_OBJECT, ChordObject, chord_object);

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
chord_selections_get_start_pos (
  ChordSelections * ts,
  Position *        pos,
  int               transient,
  int               global)
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);
  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ChordObject, chord_object, pos,
    transient, before, widget);

  if (global)
    {
      /* TODO convert to global pos */
    }
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
chord_selections_get_end_pos (
  ChordSelections * ts,
  Position *        pos,
  int               transient,
  int               global)
{
  position_init (pos);
  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ChordObject, chord_object, pos,
    transient, after, widget);

  if (global)
    {
      /* TODO convert to global pos */
    }
}

/**
 * Gets first object's widget.
 *
 * If transient is 1, transient objects are checked
 * instead.
 */
GtkWidget *
chord_selections_get_first_object (
  ChordSelections * ts,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (
    pos, TRANSPORT->total_bars);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ChordObject, chord_object, pos,
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
chord_selections_get_last_object (
  ChordSelections * ts,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_init (pos);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, ChordObject, chord_object, pos,
    transient, after, widget);

  return widget;
}

/**
 * Adds an object to the selections.
 */
#define DEFINE_ADD_OBJECT(caps,cc,sc) \
  void \
  chord_selections_add_##sc ( \
    ChordSelections * ts, \
    cc *                 sc) \
  { \
    ADD_OBJECT (caps, sc); \
  }

DEFINE_ADD_OBJECT (
  CHORD_OBJECT, ChordObject, chord_object);

#undef DEFINE_ADD_OBJECT

#define DEFINE_REMOVE_OBJ(caps,cc,sc) \
  void \
  chord_selections_remove_##sc ( \
    ChordSelections * ts, \
    cc *                 sc) \
  { \
    REMOVE_OBJECT (caps, sc); \
  }

DEFINE_REMOVE_OBJ (
  CHORD_OBJECT, ChordObject, chord_object);

#undef DEFINE_REMOVE_OBJ

#define DEFINE_CONTAINS_OBJ(caps,cc,sc) \
  int \
  chord_selections_contains_##sc ( \
    ChordSelections * self, \
    cc *                 sc) \
  { \
    return \
      array_contains ( \
        self->sc##s, self->num_##sc##s, sc); \
  }

DEFINE_CONTAINS_OBJ (
  CHORD_OBJECT, ChordObject, chord_object);

#undef DEFINE_CONTAINS_OBJ

/**
 * Set all main Position's to their transient
 * counterparts.
 */
void
chord_selections_set_to_transient_poses (
  ChordSelections * ts)
{
  int i;

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

  SET_TO_TRANS_POS (
    CHORD_OBJECT, ChordObject, chord_object);

#undef SET_TO_TRANS_POS
}

/**
 * Clears selections.
 */
void
chord_selections_clear (
  ChordSelections * ts)
{
  int i;

/* use caches because ts->* will be operated on. */
#define CS_REMOVE_OBJS(caps,cc,sc) \
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
      chord_selections_remove_##sc (ts, sc); \
      EVENTS_PUSH ( \
        ET_ARRANGER_OBJECT_SELECTION_CHANGED, \
        &sc->obj_info); \
    }

  CS_REMOVE_OBJS (
    CHORD_OBJECT, ChordObject, chord_object);

#undef CS_REMOVE_OBJS
}

/**
 * Clone the struct for copying, undoing, etc.
 */
ChordSelections *
chord_selections_clone (
  ChordSelections * src)
{
  ChordSelections * new_ts =
    calloc (1, sizeof (ChordSelections));

  int i;

#define CS_CLONE_OBJS(caps,cc,sc) \
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

  CS_CLONE_OBJS (
    CHORD_OBJECT, ChordObject, chord_object);

#undef CS_CLONE_OBJS

  return new_ts;
}

/**
 * Similar to set_to_transient_poses, but handles
 * values for objects that support them (like
 * AutomationPoint's).
 */
void
chord_selections_set_to_transient_values (
  ChordSelections * ts)
{
}

/**
 * Moves the ChordSelections by the given
 * amount of ticks.
 *
 * @param ticks Ticks to add.
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
chord_selections_add_ticks (
  ChordSelections * ts,
  long                 ticks,
  int                  use_cached_pos,
  ArrangerObjectUpdateFlag update_flag)
{
  int i;

#define UPDATE_CS_POSES(cc,sc) \
  cc * sc; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_move ( \
        sc, ticks, use_cached_pos, \
        update_flag); \
    }

  UPDATE_CS_POSES (
    ChordObject, chord_object);

#undef UPDATE_CS_POSES
}

void
chord_selections_paste_to_pos (
  ChordSelections * ts,
  Position *           pos)
{
  int pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  chord_selections_get_start_pos (
    ts, &start_pos, F_NO_TRANSIENTS, 0);
  int start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  int curr_ticks, i;
  ChordObject * chord;
  for (i = 0; i < ts->num_chord_objects; i++)
    {
      chord = ts->chord_objects[i];

      curr_ticks = position_to_ticks (&chord->pos);
      position_from_ticks (&chord->pos,
                           pos_ticks + DIFF);
    }
#undef DIFF
}

void
chord_selections_free (
  ChordSelections * self)
{
  free (self);
}

SERIALIZE_SRC (
  ChordSelections, chord_selections)
DESERIALIZE_SRC (
  ChordSelections, chord_selections)
PRINT_YAML_SRC (
  ChordSelections, chord_selections)
