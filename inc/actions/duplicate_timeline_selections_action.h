/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UNDO_DUPLICATE_TIMELINE_SELECTIONS_ACTION_H__
#define __UNDO_DUPLICATE_TIMELINE_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct TimelineSelections
  TimelineSelections;

typedef struct DuplicateTimelineSelectionsAction
{
  UndoableAction              parent_instance;

  /**
   * A clone of the timeline selections at the time.
   */
  TimelineSelections *        ts;

  /** Ticks diff. */
  long   ticks;

  /** Value (# of Tracks) diff. */
  int    delta;
} DuplicateTimelineSelectionsAction;

UndoableAction *
duplicate_timeline_selections_action_new (
  TimelineSelections * ts,
  long                 ticks,
  int                  delta);

int
duplicate_timeline_selections_action_do (
  DuplicateTimelineSelectionsAction * self);

int
duplicate_timeline_selections_action_undo (
  DuplicateTimelineSelectionsAction * self);

char *
duplicate_timeline_selections_action_stringize (
  DuplicateTimelineSelectionsAction * self);

void
duplicate_timeline_selections_action_free (
  DuplicateTimelineSelectionsAction * self);

#endif
