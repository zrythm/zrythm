/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
range_action_init_loaded (RangeAction * self)
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

  transport_init_loaded (
    self->transport, NULL, NULL);
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
  Position *      end_pos,
  GError **       error)
{
  RangeAction *    self = object_new (RangeAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_RANGE);

  g_return_val_if_fail (
    position_validate (start_pos)
      && position_validate (end_pos),
    NULL);

  self->type = type;
  position_set_to_pos (&self->start_pos, start_pos);
  self->end_pos = *end_pos;
  self->first_run = true;

  /* create selections for overlapping objects */
  Position inf;
  position_set_to_bar (&inf, POSITION_MAX_BAR);
  self->sel_before =
    timeline_selections_new_for_range (
      start_pos, &inf, F_CLONE);
  self->sel_after = (TimelineSelections *)
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_TIMELINE);

  self->transport = transport_clone (TRANSPORT);

  return ua;
}

RangeAction *
range_action_clone (const RangeAction * src)
{
  RangeAction * self = object_new (RangeAction);
  self->parent_instance = src->parent_instance;

  self->start_pos = src->start_pos;
  self->end_pos = src->end_pos;
  self->type = src->type;
  self->sel_before = (TimelineSelections *)
    arranger_selections_clone (
      (ArrangerSelections *) src->sel_before);
  self->sel_after = (TimelineSelections *)
    arranger_selections_clone (
      (ArrangerSelections *) src->sel_after);
  self->first_run = src->first_run;
  self->transport =
    transport_clone (src->transport);

  return self;
}

bool
range_action_perform (
  RangeActionType type,
  Position *      start_pos,
  Position *      end_pos,
  GError **       error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    range_action_new, error, type, start_pos,
    end_pos, error);
}

#define _MOVE_TRANSPORT_MARKER(x, _ticks, _do) \
  if ( \
    self->type == RANGE_ACTION_REMOVE \
    && position_is_after_or_equal ( \
      &TRANSPORT->x, &self->start_pos) \
    && position_is_before_or_equal ( \
      &TRANSPORT->x, &self->end_pos)) \
    { \
      /* move position to range start or back to
       * original pos */ \
      position_set_to_pos ( \
        &TRANSPORT->x, \
        _do ? &self->start_pos \
            : &self->transport->x); \
    } \
  else if (position_is_after_or_equal ( \
             &TRANSPORT->x, &self->start_pos)) \
    { \
      position_add_ticks (&TRANSPORT->x, _ticks); \
    }

#define MOVE_TRANSPORT_MARKER(x, _do) \
  _MOVE_TRANSPORT_MARKER (x, range_size_ticks, _do)

#define UNMOVE_TRANSPORT_MARKER(x, _do) \
  _MOVE_TRANSPORT_MARKER ( \
    x, -range_size_ticks, _do)

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

static void
add_to_sel_after (
  RangeAction *     self,
  ArrangerObject ** prj_objs,
  ArrangerObject ** after_objs_for_prj,
  int *             num_prj_objs,
  ArrangerObject *  prj_obj,
  ArrangerObject *  after_obj)
{
  prj_objs[*num_prj_objs] = prj_obj;
  g_debug (
    "adding to sel after (num prj objs %d)",
    *num_prj_objs);
  arranger_object_print (prj_obj);
  after_objs_for_prj[(*num_prj_objs)++] = after_obj;
  arranger_selections_add_object (
    (ArrangerSelections *) self->sel_after,
    after_obj);
}

int
range_action_do (RangeAction * self, GError ** error)
{
  /* sort the selections in ascending order */
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_before, false);
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_after, false);

  GPtrArray * before_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    (ArrangerSelections *) self->sel_before,
    before_objs_arr);
  GPtrArray * after_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    (ArrangerSelections *) self->sel_after,
    after_objs_arr);
  double range_size_ticks =
    position_to_ticks (&self->end_pos)
    - position_to_ticks (&self->start_pos);

  /* temporary place to store project objects, so
   * we can get their final identifiers at the end */
  ArrangerObject *
      prj_objs[before_objs_arr->len * 2];
  int num_prj_objs = 0;

  /* after objects corresponding to the above */
  ArrangerObject *
    after_objs_for_prj[before_objs_arr->len * 2];

#define ADD_AFTER(_prj_obj, _after_obj) \
  add_to_sel_after ( \
    self, prj_objs, after_objs_for_prj, \
    &num_prj_objs, _prj_obj, _after_obj)

  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      if (self->first_run)
        {
          for (int i = (int) before_objs_arr->len - 1;
               i >= 0; i--)
            {
              ArrangerObject * obj =
                (ArrangerObject *) g_ptr_array_index (
                  before_objs_arr, i);
              g_message (
                "looping backwards. "
                "current object %d:",
                i);
              arranger_object_print (obj);

              obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* get project object */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);

              /* object starts before the range and
               * ends after the range start -
               * split at range start */
              if (
                arranger_object_is_hit (
                  prj_obj, &self->start_pos, NULL)
                && position_is_before (
                  &prj_obj->pos, &self->start_pos))
                {
                  /* split at range start */
                  ArrangerObject *part1, *part2;
                  arranger_object_split (
                    obj, &self->start_pos, false,
                    &part1, &part2, false);

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
                    arranger_object_clone (part1);
                  arranger_object_add_to_project (
                    prj_part1, F_NO_PUBLISH_EVENTS);

                  ArrangerObject * prj_part2 =
                    arranger_object_clone (part2);
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
                    arranger_object_clone (prj_obj);

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
          for (int i = (int) before_objs_arr->len - 1;
               i >= 0; i--)
            {
              ArrangerObject * obj =
                (ArrangerObject *) g_ptr_array_index (
                  before_objs_arr, i);

              /* get project object and remove it
               * from the project */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);
              arranger_object_remove_from_project (
                prj_obj);
            }
          /* add all objects from sel_after */
          for (size_t i = 0;
               i < after_objs_arr->len; i++)
            {
              ArrangerObject * obj =
                g_ptr_array_index (
                  after_objs_arr, i);

              /* clone object and add to project */
              ArrangerObject * prj_obj =
                arranger_object_clone (obj);
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
          for (int i = (int) before_objs_arr->len - 1;
               i >= 0; i--)
            {
              ArrangerObject * obj =
                (ArrangerObject *) g_ptr_array_index (
                  before_objs_arr, i);
              g_message (
                "looping backwards. "
                "current object %d:",
                i);
              arranger_object_print (obj);

              obj->flags |=
                ARRANGER_OBJECT_FLAG_NON_PROJECT;

              /* get project object */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);

              bool ends_inside_range = false;
              if (arranger_object_type_has_length (
                    prj_obj->type))
                {
                  ends_inside_range =
                    position_is_after_or_equal (
                      &prj_obj->pos,
                      &self->start_pos)
                    && position_is_before (
                      &prj_obj->end_pos,
                      &self->end_pos);
                }
              else
                {
                  ends_inside_range =
                    position_is_after_or_equal (
                      &prj_obj->pos,
                      &self->start_pos)
                    && position_is_before (
                      &prj_obj->pos, &self->end_pos);
                }

              /* object starts before the range and
               * ends after the range start -
               * split at range start */
              if (
                arranger_object_is_hit (
                  prj_obj, &self->start_pos, NULL)
                && position_is_before (
                  &prj_obj->pos, &self->start_pos))
                {
                  /* split at range start */
                  ArrangerObject *part1, *part2;
                  arranger_object_split (
                    obj, &self->start_pos, false,
                    &part1, &part2, false);

                  /* if part 2 extends beyond the
                   * range end, split it  and
                   * remove the part before range
                   * end */
                  if (arranger_object_is_hit (
                        part2, &self->end_pos, NULL))
                    {
                      ArrangerObject *part3, *part4;
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
                        part2, -range_size_ticks);
                    }

                  /* remove previous object */
                  arranger_object_remove_from_project (
                    prj_obj);

                  /* create clones and add to
                   * project */
                  ArrangerObject * prj_part1 =
                    arranger_object_clone (part1);
                  arranger_object_add_to_project (
                    prj_part1, F_NO_PUBLISH_EVENTS);
                  ADD_AFTER (prj_part1, part1);

                  if (part2)
                    {
                      ArrangerObject * prj_part2 =
                        arranger_object_clone (
                          part2);
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
              else if (
                arranger_object_is_hit (
                  prj_obj, &self->end_pos, NULL)
                && position_is_after (
                  &prj_obj->end_pos, &self->end_pos))
                {
                  /* split at range end */
                  ArrangerObject *part1, *part2;
                  arranger_object_split (
                    obj, &self->end_pos, false,
                    &part1, &part2, false);

                  /* move part2 by the range
                   * amount */
                  arranger_object_move (
                    part2, -range_size_ticks);

                  /* discard part before range end */
                  object_free_w_func_and_null (
                    arranger_object_free, part1);

                  /* remove previous object */
                  arranger_object_remove_from_project (
                    prj_obj);

                  /* create clones and add to
                   * project */
                  ArrangerObject * prj_part2 =
                    arranger_object_clone (part2);
                  arranger_object_add_to_project (
                    prj_part2, F_NO_PUBLISH_EVENTS);
                  ADD_AFTER (prj_part2, part2);

                  g_message (
                    "object split to just:");
                  arranger_object_print (prj_part2);
                }
              /* object starts and ends inside range
               * and not marker start/end - delete */
              else if (
                ends_inside_range
                &&
                !(prj_obj->type ==
                    ARRANGER_OBJECT_TYPE_MARKER
                  &&
                  (((Marker *) prj_obj)->type ==
                     MARKER_TYPE_START
                   ||
                   (((Marker *) prj_obj)->type ==
                     MARKER_TYPE_END))))
                {
                  g_debug ("removing object:");
                  arranger_object_print (prj_obj);
                  arranger_object_remove_from_project (
                    prj_obj);
                }
              /* object starts at or after range
               * start - only needs a move */
              else
                {
                  arranger_object_move (
                    prj_obj, -range_size_ticks);

                  /* move objects to bar 1 if
                   * negative pos */
                  Position init_pos;
                  position_init (&init_pos);
                  if (position_is_before (
                        &prj_obj->pos, &init_pos))
                    {
                      g_debug (
                        "moving object back");
                      arranger_object_move (
                        prj_obj,
                        -prj_obj->pos.ticks);
                    }

                  /* clone and add to sel_after */
                  ArrangerObject * obj_clone =
                    arranger_object_clone (prj_obj);
                  g_debug ("object moved to:");
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
          for (int i = (int) before_objs_arr->len - 1;
               i >= 0; i--)
            {
              ArrangerObject * obj =
                (ArrangerObject *) g_ptr_array_index (
                  before_objs_arr, i);

              /* get project object and remove it
               * from the project */
              ArrangerObject * prj_obj =
                arranger_object_find (obj);
              arranger_object_remove_from_project (
                prj_obj);
            }
          /* add all objects from sel_after */
          for (size_t i = 0;
               i < after_objs_arr->len; i++)
            {
              ArrangerObject * obj =
                g_ptr_array_index (
                  after_objs_arr, i);

              /* clone object and add to project */
              ArrangerObject * prj_obj =
                arranger_object_clone (obj);
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
  g_ptr_array_unref (before_objs_arr);
  g_ptr_array_unref (after_objs_arr);

#undef ADD_AFTER

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
  RangeAction * self,
  GError **     error)
{
  /* sort the selections in ascending order */
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_before, false);
  arranger_selections_sort_by_indices (
    (ArrangerSelections *) self->sel_after, false);

  GPtrArray * before_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    (ArrangerSelections *) self->sel_before,
    before_objs_arr);
  GPtrArray * after_objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    (ArrangerSelections *) self->sel_after,
    after_objs_arr);
  double range_size_ticks =
    position_to_ticks (&self->end_pos)
    - position_to_ticks (&self->start_pos);

  /* remove all matching project objects from
   * sel_after */
  for (int i = (int) after_objs_arr->len - 1;
       i >= 0; i--)
    {
      ArrangerObject * obj = (ArrangerObject *)
        g_ptr_array_index (after_objs_arr, i);

      /* get project object and remove it from
       * the project */
      ArrangerObject * prj_obj =
        arranger_object_find (obj);
      arranger_object_remove_from_project (prj_obj);

      g_message ("removing");
      arranger_object_print (obj);
    }
  /* add all objects from sel_before */
  for (size_t i = 0; i < before_objs_arr->len; i++)
    {
      ArrangerObject * obj = (ArrangerObject *)
        g_ptr_array_index (before_objs_arr, i);

      /* clone object and add to project */
      ArrangerObject * prj_obj =
        arranger_object_clone (obj);
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
  g_ptr_array_unref (before_objs_arr);
  g_ptr_array_unref (after_objs_arr);

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_ACTION_FINISHED, NULL);
  EVENTS_PUSH (
    ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);

  return 0;
}

char *
range_action_stringize (RangeAction * self)
{
  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      return g_strdup (_ ("Insert silence"));
      break;
    case RANGE_ACTION_REMOVE:
      return g_strdup (_ ("Delete range"));
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_reached (NULL);
}

void
range_action_free (RangeAction * self)
{
  object_free_w_func_and_null_cast (
    arranger_selections_free_full,
    ArrangerSelections *, self->sel_before);
  object_free_w_func_and_null_cast (
    arranger_selections_free_full,
    ArrangerSelections *, self->sel_after);

  object_free_w_func_and_null (
    transport_free, self->transport);

  object_zero_and_free (self);
}
