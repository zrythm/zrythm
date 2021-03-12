/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
  UA_TRACKLIST_SELECTIONS,

  UA_CHANNEL_SEND,

  /* ---- end ---- */

  UA_MIXER_SELECTIONS,
  UA_ARRANGER_SELECTIONS,

  /* ---- connections ---- */

  UA_MIDI_MAPPING,
  UA_PORT_CONNECTION,
  UA_PORT,

  /* ---- end ---- */

  /* ---- range ---- */

  UA_RANGE,

  /* ---- end ---- */

  UA_TRANSPORT,

} UndoableActionType;

static const cyaml_strval_t
undoable_action_type_strings[] =
{
  { "Tracklist selections",
    UA_TRACKLIST_SELECTIONS },
  { "Channel send", UA_CHANNEL_SEND },
  { "Mixer selections", UA_MIXER_SELECTIONS },
  { "Arranger selections", UA_ARRANGER_SELECTIONS },
  { "MIDI mapping", UA_MIDI_MAPPING },
  { "Port connection", UA_PORT_CONNECTION },
  { "Port", UA_PORT },
  { "Transport", UA_TRANSPORT },
  { "Range", UA_RANGE },
};

typedef struct UndoableAction
{
  UndoableActionType  type;

  /**
   * Index in the stack.
   *
   * Used during deserialization.
   */
  int                 stack_idx;
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
undoable_action_to_string (
  UndoableAction * ua);

/**
 * @}
 */

#endif
