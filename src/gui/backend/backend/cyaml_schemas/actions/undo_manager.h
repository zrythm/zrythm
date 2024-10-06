/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Undo manager schema.
 */

#ifndef __SCHEMAS_UNDO_UNDO_MANAGER_H__
#define __SCHEMAS_UNDO_UNDO_MANAGER_H__

#include "zrythm-config.h"

#ifdef HAVE_CYAML

#  include "gui/backend/backend/cyaml_schemas/actions/undo_stack.h"

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

#endif /* HAVE_CYAML */

#endif
