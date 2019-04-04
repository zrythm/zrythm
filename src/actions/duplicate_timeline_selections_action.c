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

#include "actions/duplicate_timeline_selections_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/flags.h"

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
duplicate_timeline_selections_action_new (
  long ticks,
  int  delta)
{
  DuplicateTimelineSelectionsAction * self =
    calloc (1, sizeof (
                 DuplicateTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DUPLICATE_TL_SELECTIONS;
  self->ts = timeline_selections_clone ();
  self->ticks = ticks;
  self->delta = delta;
  return ua;
}

void
duplicate_timeline_selections_action_do (
  DuplicateTimelineSelectionsAction * self)
{
  Region * r, * _r;
	for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* this is a clone, must not be used */
      r = self->ts->regions[i];

      /* clone the clone */
      _r = region_clone (r, REGION_CLONE_COPY);
      _r->actual_id = _r->id;

      /* add and shift the new clone */
      track_add_region (
        project_get_track (_r->track_id),
        _r);
      region_shift (
        _r, self->ticks, self->delta);

      /* remember the new clone's id */
      r->actual_id = _r->id;
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);
}

void
duplicate_timeline_selections_action_undo (
  DuplicateTimelineSelectionsAction * self)
{
  Region * r, * _r;
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* this is a clone */
      r = self->ts->regions[i];

      /* find the region with the actual id */
      _r = project_get_region (r->actual_id);

      /* remove it */
      track_remove_region (
        project_get_track (_r->track_id),
        _r,
        F_FREE);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);
}

void
duplicate_timeline_selections_action_free (
  DuplicateTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
