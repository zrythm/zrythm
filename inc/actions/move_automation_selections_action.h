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

#ifndef __UNDO_MOVE_AUTOMATION_SELECTIONS_ACTION_H__
#define __UNDO_MOVE_AUTOMATION_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct AutomationSelections
  AutomationSelections;

typedef struct MoveAutomationSelectionsAction
{
  UndoableAction              parent_instance;

  /** Ticks moved. */
  long        ticks;

  /** Tracks moved. */
  int         delta;

  int         first_call;

  /** Automation selections clone. */
  AutomationSelections * ts;
} MoveAutomationSelectionsAction;

UndoableAction *
move_automation_selections_action_new (
  AutomationSelections * ts,
  long                 ticks,
  int                  delta);

int
move_automation_selections_action_do (
	MoveAutomationSelectionsAction * self);

int
move_automation_selections_action_undo (
	MoveAutomationSelectionsAction * self);

char *
move_automation_selections_action_stringize (
	MoveAutomationSelectionsAction * self);

void
move_automation_selections_action_free (
	MoveAutomationSelectionsAction * self);

#endif
