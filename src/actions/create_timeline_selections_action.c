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

#include "actions/create_timeline_selections_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
create_timeline_selections_action_new ()
{
  CreateTimelineSelectionsAction * self =
    calloc (1, sizeof (
                 CreateTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_CREATE_TL_SELECTIONS;
  self->ts = timeline_selections_clone ();
  return ua;
}

void
create_timeline_selections_action_do (
  CreateTimelineSelectionsAction * self)
{
  Region * r, * rc;
	for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* this is a clone, must not be used */
      r = self->ts->regions[i];

      /* check if the region already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      if (project_get_region (r->actual_id))
        continue;

      /* clone the clone */
      rc = region_clone (r, REGION_CLONE_COPY);

      /* move it to the original id */
      project_move_region (rc, r->actual_id);

      /* add the new clone */
      track_add_region (
        project_get_track (rc->track_id),
        rc);
    }
  /* TODO chords */

  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);
}

void
create_timeline_selections_action_undo (
  CreateTimelineSelectionsAction * self)
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
        _r);
      free_later (_r, region_free);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);
}

void
create_timeline_selections_action_free (
  CreateTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}

