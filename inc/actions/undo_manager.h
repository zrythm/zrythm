/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Undo manager.
 */

#ifndef __UNDO_UNDO_MANAGER_H__
#define __UNDO_UNDO_MANAGER_H__

#include "actions/undo_stack.h"

/**
 * @addtogroup actions
 *
 * @{
 */

#define UNDO_MANAGER (&PROJECT->undo_manager)

/**
 * Undo manager.
 */
typedef struct UndoManager
{
  UndoStack *   undo_stack;
  UndoStack *   redo_stack;
} UndoManager;

static const cyaml_schema_field_t
  undo_manager_fields_schema[] =
{
  YAML_FIELD_MAPPING_PTR (
    UndoManager, undo_stack,
    undo_stack_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    UndoManager, redo_stack,
    undo_stack_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  undo_manager_schema =
{
  YAML_VALUE_PTR (
    UndoManager, undo_manager_fields_schema),
};

/**
 * Inits the undo manager by creating or
 * populating the undo/redo stacks.
 *
 * @param loading True if this is a loaded project,
 *   false if a new project.
 */
void
undo_manager_init (
  UndoManager * self,
  int           loading);

/**
 * Undo last action.
 */
void
undo_manager_undo (
  UndoManager * self);

/**
 * Redo last undone action.
 */
void
undo_manager_redo (
  UndoManager * self);

/**
 * Performs the action and pushes it to the undo
 * stack.
 */
void
undo_manager_perform (
  UndoManager *    self,
  UndoableAction * action);

void
undo_manager_prepare_for_serialization (
  UndoManager * self);

/**
 * @}
 */

#endif
