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
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
range_action_init_loaded (
  RangeAction * self)
{
  if (self->sel_before)
    {
      arranger_selections_init_loaded (
        (ArrangerSelections *) self->sel_before,
        F_NOT_PROJECT);
    }
  if (self->sel_after)
    {
      arranger_selections_init_loaded (
        (ArrangerSelections *) self->sel_after,
        F_NOT_PROJECT);
    }
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

  g_return_val_if_fail (
    position_validate (start_pos) &&
    position_validate (end_pos), NULL);

  self->type = type;
  position_set_to_pos (&self->start_pos, start_pos);
  self->end_pos = *end_pos;
  self->first_run = true;

  /* create selections for overlapping objects */
  Position inf;
  position_set_to_bar (&inf, 160000);
  self->sel_before =
    timeline_selections_new_for_range (
      start_pos, &inf, F_CLONE);
  self->sel_after = timeline_selections_new ();

  self->transport = transport_clone (TRANSPORT);

  return ua;
}

#define _MOVE_TRANSPORT_MARKER(x,_ticks,_do) \
  if (self->type == RANGE_ACTION_REMOVE && \
      position_is_after_or_equal ( \
        &TRANSPORT->x, \
        &self->start_pos) && \
      position_is_before_or_equal ( \
        &TRANSPORT->x, \
        &self->end_pos)) \
    { \
      /* move position to range start or back to
       * original pos */ \
      position_set_to_pos ( \
        &TRANSPORT->x, \
        _do ? \
          &self->start_pos : &self->transport->x); \
    } \
  else if (position_is_after_or_equal ( \
             &TRANSPORT->x, \
             &self->start_pos)) \
    { \
      position_add_ticks ( \
        &TRANSPORT->x, _ticks); \
    }

#define MOVE_TRANSPORT_MARKER(x,_do) \
  _MOVE_TRANSPORT_MARKER (x, range_size_ticks, _do)

#define UNMOVE_TRANSPORT_MARKER(x,_do) \
  _MOVE_TRANSPORT_MARKER (x, - range_size_ticks, _do)

#define MOVE_TRANSPORT_MARKERS(_do) \
  MOVE_TRANSPORT_MARKER (playhead_pos, _do); \
  MOVE_TRANSPORT_MARKER (cue_pos, _do); \
  MOVE_TRANSPORT_MARKER (loop_start_pos, _do); \
  MOVE_TRANSPORT_MARKER (loop_end_pos, _do)

#define UNMOVE_TRANSPORT_MARKERS(_do) \
  UNMOVE_TRANSPORT_MARKER (playhead_pos, _do); \
  UNMOVE_TRANSPORT_MARKER (cue_pos, _do); \
  UNMOVE_TRANSPORT_MARKER (loop_start_pos, _do); \
  UNMOVE_TRANSPORT_MARKER (loop_end_pos, _do)

int
range_action_do (
  RangeAction * self)
{
  /* sort the selections in ascending order */
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_before, false);
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_after, false);

  int num_before_objs;
  ArrangerObject ** before_objs =
    arranger_selections_get_all_objects (
      (ArrangerSelections *) self->sel_before,
      &num_before_objs);
  int num_after_objs;
  ArrangerObject ** after_objs =
    arranger_selections_get_all_objects (
      (ArrangerSelections *) self->sel_after,
      &num_after_objs);
  double range_size_ticks =
    position_to_ticks (&self->end_pos) -
    position_to_ticks (&self->start_pos);

  /* temporary place to store project objects, so
   * we can get their final identifiers at the end */
  ArrangerObject * prj_objs[num_before_objs * 2];
  int num_prj_objs = 0;

  /* after objects corresponding to the above */
  ArrangerObject * after_objs_for_prj[num_before_objs * 2];

#define ADD_AFTER(_prj_obj,_after_obj) \
  prj_objs[num_prj_objs] = _prj_obj; \
  g_message ("adding %d", num_prj_objs); \
  arranger_object_print (_prj_obj); \
  after_objs_for_prj[ \
    num_prj_objs++] = _after_obj; \
  arranger_selections_add_object ( \
    (ArrangerSelections *) \
    self->sel_after, _after_obj);

  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      if (self->first_run)
        {
          for (int i = num_before_objs - 1; i >= 0;
               i--)
            {
              ArrangerObject * obj = before_objs[i];
              g_message (
                "looping backwards. "
                "current object %d:", i);
              arranger_object_print (obj);

              obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* get project object */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);

              /* object starts before the range and
               * ends after the range start -
               * split at range start */
              if (arranger_object_is_hit (
                    prj_obj, &self->start_pos,
                    NULL) &&
                  position_is_before (
                    &prj_obj->pos, &self->start_pos))
                {
                  /* split at range start */
                  ArrangerObject * part1, * part2;
                  arranger_object_split (
                    obj, &self->start_pos,
                    false, &part1, &part2, false);

                  /* move part2 by the range
                   * amount */
                  arranger_object_move (
                    part2, range_size_ticks);

                  /* remove previous object */
                  arranger_object_remove_from_project (
                    prj_obj);

                  /* create clones and add to
                   * project */
                  ArrangerObject * prj_part1 =
                    arranger_object_clone (
                      part1,
                      ARRANGER_OBJECT_CLONE_COPY_MAIN);
                  arranger_object_add_to_project (
                    prj_part1, F_NO_PUBLISH_EVENTS);

                  ArrangerObject * prj_part2 =
                    arranger_object_clone (
                      part2,
                      ARRANGER_OBJECT_CLONE_COPY_MAIN);
                  arranger_object_add_to_project (
                    prj_part2, F_NO_PUBLISH_EVENTS);

                  g_message (
                    "object split and moved into the"
                    " following objects");
                  arranger_object_print (prj_part1);
                  arranger_object_print (prj_part2);

                  ADD_AFTER (prj_part1, part1);
                  ADD_AFTER (prj_part2, part2);
                }
              /* object starts at or after range
               * start - only needs a move */
              else
                {
                  arranger_object_move (
                    prj_obj, range_size_ticks);

                  /* clone and add to sel_after */
                  ArrangerObject * obj_clone =
                    arranger_object_clone (
                      prj_obj,
                      ARRANGER_OBJECT_CLONE_COPY_MAIN);

                  g_message ("moved to object:");
                  arranger_object_print (prj_obj);

                  ADD_AFTER (prj_obj, obj_clone);
                }
            }
          self->first_run = false;
        }
      else /* not first run */
        {
          /* remove all matching project objects
           * from sel_before */
          for (int i = num_before_objs - 1; i >= 0;
               i--)
            {
              ArrangerObject * obj = before_objs[i];

              /* get project object and remove it
               * from the project */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);
              arranger_object_remove_from_project (
                prj_obj);
            }
          /* add all objects from sel_after */
          for (int i = 0; i < num_after_objs; i++)
            {
              ArrangerObject * obj = after_objs[i];

              /* clone object and add to project */
              ArrangerObject * prj_obj =
                arranger_object_clone (
                  obj,
                  ARRANGER_OBJECT_CLONE_COPY_MAIN);
              arranger_object_insert_to_project (
                prj_obj);
            }
        }

      /* move transport markers */
      MOVE_TRANSPORT_MARKERS (true);
      break;
    case RANGE_ACTION_REMOVE:
      if (self->first_run)
        {
          for (int i = num_before_objs - 1; i >= 0;
               i--)
            {
              ArrangerObject * obj = before_objs[i];
              g_message (
                "looping backwards. "
                "current object %d:", i);
              arranger_object_print (obj);

              obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* get project object */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);

              /* object starts before the range and
               * ends after the range start -
               * split at range start */
              if (arranger_object_is_hit (
                    prj_obj, &self->start_pos,
                    NULL) &&
                  position_is_before (
                    &prj_obj->pos, &self->start_pos))
                {
                  /* split at range start */
                  ArrangerObject * part1, * part2;
                  arranger_object_split (
                    obj, &self->start_pos,
                    false, &part1, &part2, false);

                  /* if part 2 extends beyond the
                   * range end, split it  and
                   * remove the part before range
                   * end */
                  if (arranger_object_is_hit (
                        part2, &self->end_pos,
                        NULL))
                    {
                      ArrangerObject * part3,
                                     * part4;
                      arranger_object_split (
                        part2, &self->end_pos,
                        false, &part3, &part4,
                        false);
                      arranger_object_free (part2);
                      arranger_object_free (part3);
                      part2 = part4;
                    }
                  /* otherwise remove the whole
                   * part2 */
                  else
                    {
                      arranger_object_free (part2);
                      part2 = NULL;
                    }

                  /* if a part2 exists, move it
                   * back */
                  if (part2)
                    {
                      arranger_object_move (
                        part2, - range_size_ticks);
                    }

                  /* remove previous object */
                  arranger_object_remove_from_project (
                    prj_obj);

                  /* create clones and add to
                   * project */
                  ArrangerObject * prj_part1 =
                    arranger_object_clone (
                      part1,
                      ARRANGER_OBJECT_CLONE_COPY_MAIN);
                  arranger_object_add_to_project (
                    prj_part1, F_NO_PUBLISH_EVENTS);
                  ADD_AFTER (prj_part1, part1);

                  if (part2)
                    {
                      ArrangerObject * prj_part2 =
                        arranger_object_clone (
                          part2,
                          ARRANGER_OBJECT_CLONE_COPY_MAIN);
                      arranger_object_add_to_project (
                        prj_part2,
                        F_NO_PUBLISH_EVENTS);
                      g_message (
                        "object split to the "
                        "following:");
                      arranger_object_print (
                        prj_part1);
                      arranger_object_print (
                        prj_part2);

                      ADD_AFTER (prj_part2, part2);
                    }
                  else
                    {
                      g_message (
                        "object split to just:");
                      arranger_object_print (
                        prj_part1);
                    }
                }
              /* object starts before the range end
               * and ends after the range end -
               * split at range end */
              else if (arranger_object_is_hit (
                         prj_obj, &self->end_pos,
                         NULL) &&
                       position_is_after (
                         &prj_obj->end_pos,
                         &self->end_pos))
                {
                  /* split at range end */
                  ArrangerObject * part1, * part2;
                  arranger_object_split (
                    obj, &self->end_pos,
                    false, &part1, &part2, false);

                  /* move part2 by the range
                   * amount */
                  arranger_object_move (
                    part2, - range_size_ticks);

                  /* discard part before range end */
                  object_free_w_func_and_null (
                    arranger_object_free, part1);

                  /* remove previous object */
                  arranger_object_remove_from_project (
                    prj_obj);

                  /* create clones and add to
                   * project */
                  ArrangerObject * prj_part2 =
                    arranger_object_clone (
                      part2,
                      ARRANGER_OBJECT_CLONE_COPY_MAIN);
                  arranger_object_add_to_project (
                    prj_part2, F_NO_PUBLISH_EVENTS);
                  ADD_AFTER (prj_part2, part2);

                  g_message (
                    "object split to just:");
                  arranger_object_print (prj_part2);
                }
              /* object starts at or after range
               * start - only needs a move */
              else
                {
                  arranger_object_move (
                    prj_obj, - range_size_ticks);

                  /* clone and add to sel_after */
                  ArrangerObject * obj_clone =
                    arranger_object_clone (
                      prj_obj,
                      ARRANGER_OBJECT_CLONE_COPY_MAIN);
                  g_message (
                    "object moved to:");
                  arranger_object_print (prj_obj);

                  ADD_AFTER (prj_obj, obj_clone);
                }
            }
          self->first_run = false;
        }
      else /* not first run */
        {
          /* remove all matching project objects
           * from sel_before */
          for (int i = num_before_objs - 1; i >= 0;
               i--)
            {
              ArrangerObject * obj = before_objs[i];

              /* get project object and remove it
               * from the project */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);
              arranger_object_remove_from_project (
                prj_obj);
            }
          /* add all objects from sel_after */
          for (int i = 0; i < num_after_objs; i++)
            {
              ArrangerObject * obj = after_objs[i];

              /* clone object and add to project */
              ArrangerObject * prj_obj =
                arranger_object_clone (
                  obj,
                  ARRANGER_OBJECT_CLONE_COPY_MAIN);
              arranger_object_insert_to_project (
                prj_obj);
            }
        }

      /* move transport markers */
      UNMOVE_TRANSPORT_MARKERS (true);
      break;
    default:
      break;
    }
  free (before_objs);
  free (after_objs);

  for (int i = 0; i < num_prj_objs; i++)
    {
      g_message ("copying %d", i);
      arranger_object_print (prj_objs[i]);
      arranger_object_copy_identifier (
        after_objs_for_prj[i], prj_objs[i]);
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_ACTION_FINISHED, NULL);
  EVENTS_PUSH (
    ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);

  return 0;
}

int
range_action_undo (
  RangeAction * self)
{
  /* sort the selections in ascending order */
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_before, false);
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_after, false);

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

  /* remove all matching project objects from
   * sel_after */
  for (int i = num_objs_after - 1; i >= 0; i--)
    {
      ArrangerObject * obj = objs_after[i];

      /* get project object and remove it from
       * the project */
      ArrangerObject * prj_obj =
        arranger_object_find (obj);
      arranger_object_remove_from_project (
        prj_obj);

      g_message ("removing");
      arranger_object_print (obj);
    }
  /* add all objects from sel_before */
  for (int i = 0; i < num_objs_before; i++)
    {
      ArrangerObject * obj = objs_before[i];

      /* clone object and add to project */
      ArrangerObject * prj_obj =
        arranger_object_clone (
          obj, ARRANGER_OBJECT_CLONE_COPY_MAIN);
      arranger_object_insert_to_project (prj_obj);

      g_message ("adding");
      arranger_object_print (obj);
      g_message ("after adding");
      arranger_object_print (prj_obj);
    }

  /* move transport markers */
  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      UNMOVE_TRANSPORT_MARKERS (false);
      break;
    case RANGE_ACTION_REMOVE:
      MOVE_TRANSPORT_MARKERS (false);
      break;
    default:
      break;
    }
  free (objs_before);
  free (objs_after);

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_ACTION_FINISHED, NULL);
  EVENTS_PUSH (
    ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);

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
  if (self->sel_before)
    {
      arranger_selections_free_full (
        (ArrangerSelections *) self->sel_before);
      self->sel_before = NULL;
    }
  if (self->sel_after)
    {
      arranger_selections_free_full (
        (ArrangerSelections *) self->sel_after);
      self->sel_after = NULL;
    }

  object_zero_and_free (self);
}
