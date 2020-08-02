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

#define MOVE_TRANSPORT_MARKER(x) \
if (position_is_after_or_equal ( \
      &TRANSPORT->x, \
      &self->start_pos)) \
  { \
    position_add_ticks ( \
      &TRANSPORT->x, range_size_ticks); \
  }

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

          /* get project object */
          ArrangerObject * prj_obj =
            arranger_object_find (obj);

          /* object needs adjustment */
          if (position_is_before (
                &obj->pos, &self->end_pos))
            {
            }
          else /* object only needs a move */
            {
              /* move */
              arranger_object_move (
                prj_obj, range_size_ticks);

              /* clone and add to sel_after */
              ArrangerObject * obj_clone =
                arranger_object_clone (
                  prj_obj,
                  ARRANGER_OBJECT_CLONE_COPY_MAIN);
              arranger_selections_add_object (
                (ArrangerSelections *) self->sel_after,
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
  switch (self->type)
    {
    case RANGE_ACTION_INSERT_SILENCE:
      break;
    case RANGE_ACTION_REMOVE:
      break;
    default:
      break;
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_ACTION_FINISHED,
    self->sel_before);

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
