/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/range_action.h"
#include "audio/channel.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

void
range_action_init_loaded (
  RangeAction * self)
{
}

/**
 * Creates a new action.
 *
 * @param start_pos Range start.
 * @param end_pos Range end.
 */
UndoableAction *
range_action_new (
  RangeActionType type,
  Position *      start_pos,
  Position *      end_pos)
{
  RangeAction * self = object_new (RangeAction);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_RANGE;

  self->type = type;
  self->start_pos = *start_pos;
  self->end_pos = *end_pos;

  /* create selections for overlapping objects */
  Position inf;
  position_set_to_bar (&inf, 160000);
  self->sel_before =
    timeline_selections_new_for_range (
      start_pos, &inf, F_CLONE);
  self->sel_after = timeline_selections_new ();

  return ua;
}

#define _MOVE_TRANSPORT_MARKER(x,_ticks) \
if (position_is_after_or_equal ( \
      &TRANSPORT->x, \
      &self->start_pos)) \
  { \
    position_add_ticks ( \
      &TRANSPORT->x, range_size_ticks); \
  }

#define MOVE_TRANSPORT_MARKER(x) \
  _MOVE_TRANSPORT_MARKER (x, range_size_ticks)

#define UNMOVE_TRANSPORT_MARKER(x) \
  _MOVE_TRANSPORT_MARKER (x, - range_size_ticks)

int
range_action_do (
  RangeAction * self)
{
  int num_objs;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      (ArrangerSelections *) self->sel_before,
      &num_objs);
  double range_size_ticks =
    position_to_ticks (&self->end_pos) -
    position_to_ticks (&self->start_pos);

  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      /* move all objects */
      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];

          obj->flags |=
            ARRANGER_OBJECT_FLAG_NON_PROJECT;

          /* get project object */
          ArrangerObject * prj_obj =
            arranger_object_find (obj);

          /* object starts before the range and ends
           * after the range start - split at range
           * start */
          if (arranger_object_is_hit (
                prj_obj, &self->start_pos, NULL) &&
              position_is_before (
                &prj_obj->pos, &self->start_pos))
            {
              /* split at range start */
              ArrangerObject * part1, * part2;
              arranger_object_split (
                obj, &self->start_pos,
                false, &part1, &part2, false);

              /* move part2 by the range amount */
              arranger_object_move (
                part2, range_size_ticks);

              /* save new parts to sel_after */
              arranger_selections_add_object (
                (ArrangerSelections *)
                self->sel_after, part1);
              arranger_selections_add_object (
                (ArrangerSelections *)
                self->sel_after, part2);

              /* remove previous object */
              arranger_object_remove_from_project (
                prj_obj);

              /* create clones and add to project */
              part1 =
                arranger_object_clone (
                  part1,
                  ARRANGER_OBJECT_CLONE_COPY_MAIN);
              arranger_object_add_to_project (part1);
              part2 =
                arranger_object_clone (
                  part2,
                  ARRANGER_OBJECT_CLONE_COPY_MAIN);
              arranger_object_add_to_project (part2);
            }
          /* object starts at or after range start -
           * only needs a move */
          else
            {
              arranger_object_move (
                prj_obj, range_size_ticks);

              /* clone and add to sel_after */
              ArrangerObject * obj_clone =
                arranger_object_clone (
                  prj_obj,
                  ARRANGER_OBJECT_CLONE_COPY_MAIN);
              arranger_selections_add_object (
                (ArrangerSelections *)
                self->sel_after,
                obj_clone);
            }
        }

      /* move transport markers */
      MOVE_TRANSPORT_MARKER (playhead_pos);
      MOVE_TRANSPORT_MARKER (loop_start_pos);
      MOVE_TRANSPORT_MARKER (loop_end_pos);
      break;
    case RANGE_ACTION_REMOVE:
      break;
    default:
      break;
    }
  free (objs);

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_ACTION_FINISHED,
    self->sel_after);

  return 0;
}

/**
 * Edits the plugin.
 */
int
range_action_undo (
  RangeAction * self)
{
  int num_objs_before;
  ArrangerObject ** objs_before =
    arranger_selections_get_all_objects (
      (ArrangerSelections *) self->sel_before,
      &num_objs_before);
  int num_objs_after;
  ArrangerObject ** objs_after =
    arranger_selections_get_all_objects (
      (ArrangerSelections *) self->sel_after,
      &num_objs_after);
  double range_size_ticks =
    position_to_ticks (&self->end_pos) -
    position_to_ticks (&self->start_pos);

  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      /* remove all matching project objects from
       * sel_after */
      for (int i = 0; i < num_objs_after; i++)
        {
          ArrangerObject * obj = objs_after[i];

          /* get project object and remove it from
           * the project */
          ArrangerObject * prj_obj =
            arranger_object_find (obj);
          arranger_object_remove_from_project (
            prj_obj);
        }
      /* add all objects from sel_before */
      for (int i = 0; i < num_objs_before; i++)
        {
          ArrangerObject * obj = objs_before[i];

          /* clone object and add to project */
          ArrangerObject * prj_obj =
            arranger_object_clone (
              obj, ARRANGER_OBJECT_CLONE_COPY_MAIN);
          arranger_object_add_to_project (prj_obj);
        }

      /* move transport markers */
      MOVE_TRANSPORT_MARKER (playhead_pos);
      MOVE_TRANSPORT_MARKER (loop_start_pos);
      MOVE_TRANSPORT_MARKER (loop_end_pos);
      break;
    case RANGE_ACTION_REMOVE:
      break;
    default:
      break;
    }
  free (objs_before);
  free (objs_after);

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_ACTION_FINISHED, NULL);

  return 0;
}

char *
range_action_stringize (
  RangeAction * self)
{
  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      return g_strdup (_("Insert silence"));
      break;
    case RANGE_ACTION_REMOVE:
      return g_strdup (_("Delete range"));
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_reached (NULL);
}

void
range_action_free (
  RangeAction * self)
{
  arranger_selections_free_full (
    (ArrangerSelections *) self->sel_before);
  self->sel_before = NULL;
  arranger_selections_free_full (
    (ArrangerSelections *) self->sel_after);
  self->sel_after = NULL;

  object_zero_and_free (self);
}
