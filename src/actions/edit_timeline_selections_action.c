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

#include "actions/edit_timeline_selections_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
edit_timeline_selections_action_new (
  TimelineSelections * ts,
  long                 ticks,
  int                  delta)
{
	EditTimelineSelectionsAction * self =
    calloc (1, sizeof (
    	EditTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_TL_SELECTIONS;

  self->ts = timeline_selections_clone (ts);

  return ua;
}

int
edit_timeline_selections_action_do (
	EditTimelineSelectionsAction * self)
{
  return 0;
}

int
edit_timeline_selections_action_undo (
	EditTimelineSelectionsAction * self)
{
  return 0;
}

char *
edit_timeline_selections_action_stringize (
	EditTimelineSelectionsAction * self)
{
  return g_strdup (
    _("Edit Object(s)"));
}

void
edit_timeline_selections_action_free (
	EditTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
