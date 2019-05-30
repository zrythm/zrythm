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
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
duplicate_timeline_selections_action_new (
  TimelineSelections * ts,
  long                 ticks,
  int                  delta)
{
  DuplicateTimelineSelectionsAction * self =
    calloc (1, sizeof (
                 DuplicateTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DUPLICATE_TL_SELECTIONS;

  self->ts = timeline_selections_clone (ts);
  self->ticks = ticks;
  self->delta = delta;

  return ua;
}

int
duplicate_timeline_selections_action_do (
  DuplicateTimelineSelectionsAction * self)
{
  Region * region;
	for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* clone the clone */
      region =
        region_clone (
          self->ts->regions[i], REGION_CLONE_COPY);

      /* add and shift it */
      track_add_region (
        TRACKLIST->tracks[region->track_pos],
        region, 0, F_GEN_NAME);
      region_shift (
        region, self->ticks, self->delta);

      /* select it */
      region_widget_select (region->widget,
                            F_SELECT,
                            F_NO_TRANSIENTS);

      /* remember its name */
      g_free (self->ts->regions[i]->name);
      self->ts->regions[i]->name =
        g_strdup (region->name);

    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
duplicate_timeline_selections_action_undo (
  DuplicateTimelineSelectionsAction * self)
{
  Region * region;
  for (int i = 0; i < self->ts->num_regions; i++)
    {
      /* find the actual region */
      region =
        region_find_by_name (
          self->ts->regions[i]->name);

      /* remove it */
      track_remove_region (
        region->lane->track,
        region);
      free_later (region, region_free);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
duplicate_timeline_selections_action_stringize (
  DuplicateTimelineSelectionsAction * self)
{
  return g_strdup (
    _("Duplicate Object(s)"));
}

void
duplicate_timeline_selections_action_free (
  DuplicateTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
