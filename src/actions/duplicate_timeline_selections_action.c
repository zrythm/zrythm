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
    UNDOABLE_ACTION_TYPE_DUPLICATE_MIDI_NOTES;
  self->ts = timeline_selections_clone ();
  self->ticks = ticks;
  self->delta = delta;
  return ua;
}

void
duplicate_timeline_selections_action_do (
  DuplicateTimelineSelectionsAction * self)
{
  Region * r;
	for (int i = 0; i < self->ts->num_regions; i++)
    {
      r = self->ts->regions[i];
      track_add_region (
        r->track,
        r);
      region_shift (
        r, self->ticks, self->delta);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);
}

void
duplicate_timeline_selections_action_undo (
  DuplicateTimelineSelectionsAction * self)
{
  Region * r;
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      r = self->ts->regions[i];
      track_remove_region (
        r->track,
        r,
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
