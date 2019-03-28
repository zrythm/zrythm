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

#ifndef __UNDO_MOVE_TIMELINE_SELECTIONS_ACTION_H__
#define __UNDO_MOVE_TIMELINE_SELECTIONS_POS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct TimelineSelections
  TimelineSelections;

typedef struct MoveTimelineSelectionsAction
{
  UndoableAction              parent_instance;

  /** Ticks moved. */
  long        ticks;

  /** Tracks moved. */
  int         delta;

  /** Timeline selections clone. */
  TimelineSelections * ts;
} MoveTimelineSelectionsAction;

UndoableAction *
move_timeline_selections_action_new (
  long ticks,
  int  delta);

void
move_timeline_selections_action_do (
	MoveTimelineSelectionsAction * self);

void
move_timeline_selections_action_undo (
	MoveTimelineSelectionsAction * self);

void
move_timeline_selections_action_free (
	MoveTimelineSelectionsAction * self);

#endif
