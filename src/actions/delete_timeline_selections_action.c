/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "actions/delete_timeline_selections_action.h"

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
delete_timeline_selections_action_new ()
{
  DeleteTimelineSelectionsAction * self =
    calloc (1, sizeof (DeleteTimelineSelectionsAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DELETE_TL_SELECTIONS;

  self->ts = timeline_selections_clone ();

  return ua;
}

void
delete_timeline_selections_action_do (
  DeleteTimelineSelectionsAction * self)
{
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* this is a clone */
      Region * _r = self->ts->regions[i];

      /* find actual region */
      Region * r =
        project_get_region (_r->actual_id);

      /* remove it */
      track_remove_region (r->track,
                           r,
                           1);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);
}

void
delete_timeline_selections_action_undo (
  DeleteTimelineSelectionsAction * self)
{
  Region * r, * _r;
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* this is a clone, must never be used */
      r = self->ts->regions[i];

      /* clone the clone */
      _r = region_clone (
        r, REGION_CLONE_COPY);

      /* move the new clone to the original id */
      project_move_region (_r, r->actual_id);

      /* add it to track */
      track_add_region (_r->track,
                        _r);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);
}

void
delete_timeline_selections_action_free (
  DeleteTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
