/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Undoable actions.
 */

#ifndef __UNDO_UNDOABLE_ACTION_H__
#define __UNDO_UNDOABLE_ACTION_H__

#include "utils/yaml.h"

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Type of UndoableAction.
 */
typedef enum UndoableActionType
{
  /* ---- Track/Channel ---- */
  UA_CREATE_TRACKS,
  UA_MOVE_TRACKS,
  /** Edit track/channel parameters. */
  UA_EDIT_TRACKS,
  UA_COPY_TRACKS,
  UA_DELETE_TRACKS,
  UA_CHANNEL_SEND,

  /* ---- end ---- */

  /* ---- Plugin ---- */

  UA_CREATE_PLUGINS,
  UA_MOVE_PLUGINS,
  UA_EDIT_PLUGINS,
  UA_COPY_PLUGINS,
  UA_DELETE_PLUGINS,

  /* ---- end ---- */

  /* ---- ARRANGER SELECTIONS ---- */

  UA_CREATE_ARRANGER_SELECTIONS,
  UA_MOVE_ARRANGER_SELECTIONS,
  UA_LINK_ARRANGER_SELECTIONS,
  UA_RECORD_ARRANGER_SELECTIONS,
  UA_RESIZE_ARRANGER_SELECTIONS,
  UA_SPLIT_ARRANGER_SELECTIONS,
  UA_EDIT_ARRANGER_SELECTIONS,
  UA_DUPLICATE_ARRANGER_SELECTIONS,
  UA_DELETE_ARRANGER_SELECTIONS,
  UA_QUANTIZE_ARRANGER_SELECTIONS,

  /* ---- end ---- */

  /* ---- port connection ---- */

  UA_PORT_CONNECTION,

  /* ---- end ---- */

  UA_TRANSPORT,

} UndoableActionType;

static const cyaml_strval_t
undoable_action_type_strings[] =
{
  { "Create tracks",
    UA_CREATE_TRACKS },
  { "Move tracks",
    UA_MOVE_TRACKS },
  { "Edit tracks",
    UA_EDIT_TRACKS },
  { "Copy tracks",
    UA_COPY_TRACKS },
  { "Delete tracks",
    UA_DELETE_TRACKS },
  { "Channel send",
    UA_CHANNEL_SEND },
  { "Create plugins",
    UA_CREATE_PLUGINS },
  { "Move plugins",
    UA_MOVE_PLUGINS },
  { "Edit plugins",
    UA_EDIT_PLUGINS },
  { "Copy plugins",
    UA_COPY_PLUGINS },
  { "Delete plugins",
    UA_DELETE_PLUGINS },
  { "Create arranger selections",
    UA_CREATE_ARRANGER_SELECTIONS },
  { "Move arranger selections",
    UA_MOVE_ARRANGER_SELECTIONS },
  { "Link arranger selections",
    UA_LINK_ARRANGER_SELECTIONS },
  { "Record arranger selections",
    UA_RECORD_ARRANGER_SELECTIONS },
  { "Resize arranger selections",
    UA_RESIZE_ARRANGER_SELECTIONS },
  { "Split arranger selections",
    UA_SPLIT_ARRANGER_SELECTIONS },
  { "Edit arranger selections",
    UA_EDIT_ARRANGER_SELECTIONS },
  { "Duplicate arranger selections",
    UA_DUPLICATE_ARRANGER_SELECTIONS },
  { "Delete arranger selections",
    UA_DELETE_ARRANGER_SELECTIONS },
  { "Quantize arranger selections",
    UA_QUANTIZE_ARRANGER_SELECTIONS },
  { "Port connection", UA_PORT_CONNECTION },
  { "Transport", UA_TRANSPORT },
};

typedef struct UndoableAction
{
  UndoableActionType         type;

  /**
   * Index in the stack.
   *
   * Used during deserialization.
   */
  int                        stack_idx;
} UndoableAction;

static const cyaml_schema_field_t
  undoable_action_fields_schema[] =
{
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    UndoableAction, type,
    undoable_action_type_strings,
    CYAML_ARRAY_LEN (undoable_action_type_strings)),
  YAML_FIELD_INT (
    UndoableAction, stack_idx),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  undoable_action_schema =
{
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    UndoableAction, undoable_action_fields_schema),
};

void
undoable_action_init_loaded (
  UndoableAction * self);

/**
 * Performs the action.
 *
 * @note Only to be called by undo manager.
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_do (
  UndoableAction * self);

/**
 * Undoes the action.
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_undo (
  UndoableAction * self);

void
undoable_action_free (
  UndoableAction * self);

/**
 * Stringizes the action to be used in Undo/Redo
 * buttons.
 *
 * The string MUST be free'd using g_free().
 */
char *
undoable_action_stringize (
  UndoableAction * ua);

/**
 * @}
 */

#endif
