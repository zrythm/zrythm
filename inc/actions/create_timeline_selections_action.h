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

#ifndef __UNDO_CREATE_TIMELINE_SELECTIONS_ACTION_H__
#define __UNDO_CREATE_TIMELINE_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct TimelineSelections
  TimelineSelections;

/**
 * It currently "creates" the current selection so
 * that it's undoable.
 *
 * It does nothing on perform due to how the
 * arrangers currently work. This might change in
 * the future.
 */
typedef struct CreateTimelineSelectionsAction
{
  UndoableAction              parent_instance;

  /**
   * A clone of the timeline selections at the time.
   */
  TimelineSelections *        ts;
} CreateTimelineSelectionsAction;

/**
 * The given TimelineSelections must already
 * contain the created selections.
 */
UndoableAction *
create_timeline_selections_action_new (
  TimelineSelections * ts);

int
create_timeline_selections_action_do (
  CreateTimelineSelectionsAction * self);

int
create_timeline_selections_action_undo (
  CreateTimelineSelectionsAction * self);

char *
create_timeline_selections_action_stringize (
  CreateTimelineSelectionsAction * self);

void
create_timeline_selections_action_free (
  CreateTimelineSelectionsAction * self);

#endif
