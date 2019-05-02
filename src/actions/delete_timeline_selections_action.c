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

#include "actions/delete_timeline_selections_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
delete_timeline_selections_action_new (
  TimelineSelections * ts)
{
  DeleteTimelineSelectionsAction * self =
    calloc (
      1, sizeof (DeleteTimelineSelectionsAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DELETE_TL_SELECTIONS;

  self->ts = timeline_selections_clone (ts);

  return ua;
}

int
delete_timeline_selections_action_do (
  DeleteTimelineSelectionsAction * self)
{
  Region * region;

  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* find actual region */
      region =
        project_get_region (
          self->ts->regions[i]->id);

      /* remove it */
      track_remove_region (
        project_get_track (region->track_id),
        region);
      free_later (region, region_free);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
delete_timeline_selections_action_undo (
  DeleteTimelineSelectionsAction * self)
{
  Region * region, * orig_region;
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* get the clone */
      orig_region = self->ts->regions[i];

      /* clone the clone */
      region =
        region_clone (
          orig_region, REGION_CLONE_COPY);

      /* add to project to get unique ID */
      project_add_region (region);

      /* add it to track */
      track_add_region (
        project_get_track (region->track_id),
        region);

      /* remember the ID */
      orig_region->id = region->id;
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
delete_timeline_selections_action_stringize (
  DeleteTimelineSelectionsAction * self)
{
  return g_strdup (
    _("Delete Object(s)"));
}

void
delete_timeline_selections_action_free (
  DeleteTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
