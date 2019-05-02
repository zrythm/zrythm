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

#ifndef __UNDO_EDIT_TIMELINE_SELECTIONS_ACTION_H__
#define __UNDO_EDIT_TIMELINE_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct TimelineSelections
  TimelineSelections;

typedef struct EditTimelineSelectionsAction
{
  UndoableAction              parent_instance;

  /** Timeline selections clone. */
  TimelineSelections * ts;
} EditTimelineSelectionsAction;

UndoableAction *
edit_timeline_selections_action_new (
  TimelineSelections * ts,
  long                 ticks,
  int                  delta);

int
edit_timeline_selections_action_do (
	EditTimelineSelectionsAction * self);

int
edit_timeline_selections_action_undo (
	EditTimelineSelectionsAction * self);

char *
edit_timeline_selections_action_stringize (
	EditTimelineSelectionsAction * self);

void
edit_timeline_selections_action_free (
	EditTimelineSelectionsAction * self);

#endif
