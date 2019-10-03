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
#include "gui/backend/automation_selections.h"
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
automation_selections_init_loaded (
  AutomationSelections * ts)
{
  int i;

#define _SET_OBJ(sc) \
  for (i = 0; i < ts->num_##sc##s; i++) \
    ts->sc##s[i] = sc##_find (ts->sc##s[i]);

  _SET_OBJ (automation_point);

#undef _SET_OBJ
}

/**
 * Returns if there are any selections.
 */
int
automation_selections_has_any (
  AutomationSelections * ts)
{
  return (
    ts->num_automation_points > 0);
}

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
void
automation_selections_set_cache_poses (
  AutomationSelections * ts)
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

  SET_CACHE_POS (AutomationPoint, automation_point);

#undef SET_CACHE_POS_W_LENGTH
#undef SET_CACHE_POS
}

ARRANGER_SELECTIONS_DECLARE_RESET_COUNTERPARTS (
  Automation, automation)
{
  for (int i = 0;
       i < self->num_automation_points; i++)
    {
      automation_point_reset_counterpart (
        self->automation_points[i], reset_trans);
    }
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
automation_selections_get_start_pos (
  AutomationSelections * ts,
  Position *           pos,
  int                  transient,
  int                  global)
{
  position_set_to_bar (
    pos, TRANSPORT->total_bars);

  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, AutomationPoint, automation_point, pos,
    transient, before, widget);

  if (global)
    position_add_ticks (
      pos,
      ts->automation_points[0]->region->
        start_pos.total_ticks);
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
automation_selections_get_end_pos (
  AutomationSelections * ts,
  Position *           pos,
  int                  transient,
  int                   global)
{
  position_init (pos);
  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, AutomationPoint, automation_point, pos,
    transient, after, widget);
}

/**
 * Gets first object's widget.
 *
 * If transient is 1, transient objects are checked
 * instead.
 */
GtkWidget *
automation_selections_get_first_object (
  AutomationSelections * ts,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (
    pos, TRANSPORT->total_bars);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, AutomationPoint, automation_point, pos,
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
automation_selections_get_last_object (
  AutomationSelections * ts,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_init (pos);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    ts, AutomationPoint, automation_point, pos,
    transient, after, widget);

  return widget;
}

/**
 * Adds an object to the selections.
 */
#define DEFINE_ADD_OBJECT(caps,cc,sc) \
  void \
  automation_selections_add_##sc ( \
    AutomationSelections * ts, \
    cc *                 sc) \
  { \
    ADD_OBJECT (caps, sc); \
  }

DEFINE_ADD_OBJECT (
  AUTOMATION_POINT, AutomationPoint,
  automation_point);

#undef DEFINE_ADD_OBJECT

#define DEFINE_REMOVE_OBJ(caps,cc,sc) \
  void \
  automation_selections_remove_##sc ( \
    AutomationSelections * ts, \
    cc *                 sc) \
  { \
    REMOVE_OBJECT (caps, sc); \
  }

DEFINE_REMOVE_OBJ (
  AUTOMATION_POINT, AutomationPoint,
  automation_point);

#undef DEFINE_REMOVE_OBJ

#define DEFINE_CONTAINS_OBJ(caps,cc,sc) \
  int \
  automation_selections_contains_##sc ( \
    AutomationSelections * self, \
    cc *                 sc) \
  { \
    return \
      array_contains ( \
        self->sc##s, self->num_##sc##s, sc); \
  }

DEFINE_CONTAINS_OBJ (
  AUTOMATION_POINT, AutomationPoint,
  automation_point);

#undef DEFINE_CONTAINS_OBJ

/**
 * Clears selections.
 */
void
automation_selections_clear (
  AutomationSelections * ts)
{
  int i;

/* use caches because ts->* will be operated on. */
#define AS_REMOVE_OBJS(caps,cc,sc) \
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
      automation_selections_remove_##sc (ts, sc); \
      EVENTS_PUSH ( \
        ET_ARRANGER_OBJECT_SELECTION_CHANGED, \
        &sc->obj_info); \
    }

  AS_REMOVE_OBJS (
    AUTOMATION_POINT, AutomationPoint,
    automation_point);

#undef AS_REMOVE_OBJS
}

/**
 * Clone the struct for copying, undoing, etc.
 */
AutomationSelections *
automation_selections_clone (
  const AutomationSelections * src)
{
  AutomationSelections * new_ts =
    calloc (1, sizeof (AutomationSelections));

  int i;

#define AS_CLONE_OBJS(caps,cc,sc) \
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

  AS_CLONE_OBJS (
    AUTOMATION_POINT, AutomationPoint,
    automation_point);

#undef AS_CLONE_OBJS

  return new_ts;
}

/**
 * Moves the AutomationSelections by the given
 * amount of ticks.
 *
 * @param ticks Ticks to add.
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
automation_selections_add_ticks (
  AutomationSelections * ts,
  long                 ticks,
  int                  use_cached_pos,
  ArrangerObjectUpdateFlag update_flag)
{
  int i;

#define UPDATE_AS_POSES(cc,sc) \
  cc * sc; \
  for (i = 0; i < ts->num_##sc##s; i++) \
    { \
      sc = ts->sc##s[i]; \
      sc##_move ( \
        sc, ticks, use_cached_pos, \
        update_flag); \
    }

  UPDATE_AS_POSES (
    AutomationPoint, automation_point);

#undef UPDATE_AS_POSES
}

void
automation_selections_paste_to_pos (
  AutomationSelections * ts,
  Position *           pos)
{
  long pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  automation_selections_get_start_pos (
    ts, &start_pos, F_NO_TRANSIENTS, 0);
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
  AutomationPoint * ap;
  for (i = 0; i < ts->num_automation_points; i++)
    {
      ap =
        ts->automation_points[i];

      curr_ticks = position_to_ticks (&ap->pos);
      position_from_ticks (
        &ap->pos,
        pos_ticks + DIFF);
    }
#undef DIFF
}

/**
 * Frees all the objects as well.
 *
 * To be used in actions where the selections are
 * all clones.
 */
void
automation_selections_free_full (
  AutomationSelections * self)
{
  for (int i = 0;
       i < self->num_automation_points; i++)
    {
      automation_point_free (
        self->automation_points[i]);
    }

  automation_selections_free (self);
}

void
automation_selections_free (
  AutomationSelections * self)
{
  free (self);
}

SERIALIZE_SRC (
  AutomationSelections, automation_selections)
DESERIALIZE_SRC (
  AutomationSelections, automation_selections)
PRINT_YAML_SRC (
  AutomationSelections, automation_selections)
