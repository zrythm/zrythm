/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Undo manager schema.
 */

#ifndef __SCHEMAS_UNDO_UNDO_MANAGER_H__
#define __SCHEMAS_UNDO_UNDO_MANAGER_H__

#include "schemas/actions/undo_stack.h"

typedef struct UndoManager_v1
{
  int            schema_version;
  UndoStack_v1 * undo_stack;
  UndoStack_v1 * redo_stack;
} UndoManager_v1;

static const cyaml_schema_field_t undo_manager_fields_schema[] = {
  YAML_FIELD_INT (UndoManager_v1, schema_version),
  YAML_FIELD_MAPPING_PTR (UndoManager_v1, undo_stack, undo_stack_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (UndoManager_v1, redo_stack, undo_stack_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t undo_manager_schema_v1 = {
  YAML_VALUE_PTR (UndoManager_v1, undo_manager_fields_schema_v1),
};

#endif
