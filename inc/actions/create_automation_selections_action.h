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

#ifndef __UNDO_CREATE_AUTOMATION_SELECTIONS_ACTION_H__
#define __UNDO_CREATE_AUTOMATION_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct AutomationSelections
  AutomationSelections;

/**
 * It currently "creates" the current selection so
 * that it's undoable.
 *
 * It does nothing on perform due to how the
 * arrangers currently work. This might change in
 * the future.
 */
typedef struct CreateAutomationSelectionsAction
{
  UndoableAction              parent_instance;

  /**
   * A clone of the automation selections at the time.
   */
  AutomationSelections *        ts;
} CreateAutomationSelectionsAction;

/**
 * The given AutomationSelections must already
 * contain the created selections in the transient
 * arrays.
 */
UndoableAction *
create_automation_selections_action_new (
  AutomationSelections * ts);

int
create_automation_selections_action_do (
  CreateAutomationSelectionsAction * self);

int
create_automation_selections_action_undo (
  CreateAutomationSelectionsAction * self);

char *
create_automation_selections_action_stringize (
  CreateAutomationSelectionsAction * self);

void
create_automation_selections_action_free (
  CreateAutomationSelectionsAction * self);

#endif
