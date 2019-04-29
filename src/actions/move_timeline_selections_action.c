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

#include "actions/move_timeline_selections_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"

/**
 * Note: this action will add delta beats
 * to start and end pos of all selected midi_notes */
UndoableAction *
move_timeline_selections_action_new (
  long ticks,
  int  delta)
{
	MoveTimelineSelectionsAction * self =
    calloc (1, sizeof (
    	MoveTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_TL_SELECTIONS;
  self->ts = timeline_selections_clone ();
  self->delta = delta;
  self->ticks = ticks;
  return ua;
}

int
move_timeline_selections_action_do (
	MoveTimelineSelectionsAction * self)
{
  Region * _r, * r;
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* this is a clone */
      _r = self->ts->regions[i];

      /* find actual region */
      r =
        project_get_region (_r->actual_id);

      /* shift it */
      region_shift (
        r,
        self->ticks,
        self->delta);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
move_timeline_selections_action_undo (
	MoveTimelineSelectionsAction * self)
{
  Region * _r, * r;
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* this is a clone */
      _r = self->ts->regions[i];

      /* find actual region */
      r =
        project_get_region (_r->actual_id);

      /* shift it */
      region_shift (
        r,
        - self->ticks,
        - self->delta);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

void
move_timeline_selections_action_free (
	MoveTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
