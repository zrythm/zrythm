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

#include <stdbool.h>

#include "utils/yaml.h"

typedef struct AudioClip AudioClip;
typedef struct PortConnectionsManager
  PortConnectionsManager;

/**
 * @addtogroup actions
 *
 * @{
 */

#define UNDOABLE_ACTION_SCHEMA_VERSION 1

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
  { "Range", UA_RANGE },
  { "Transport", UA_TRANSPORT },
};

/**
 * Base struct to be inherited by implementing
 * undoable actions.
 */
typedef struct UndoableAction
{
  int                schema_version;

  /** Undoable action type. */
  UndoableActionType  type;

  /**
   * Index in the stack.
   *
   * Used during deserialization.
   */
  int                 stack_idx;

  /**
   * Number of actions to perform.
   *
   * This is used to group multiple actions into
   * one logical action (eg, create a group track
   * and route multiple tracks to it).
   *
   * To be set on the last action being performed.
   */
  int                 num_actions;
} UndoableAction;

static const cyaml_schema_field_t
  undoable_action_fields_schema[] =
{
  YAML_FIELD_INT (UndoableAction, schema_version),
  YAML_FIELD_ENUM (
    UndoableAction, type,
    undoable_action_type_strings),
  YAML_FIELD_INT (
    UndoableAction, stack_idx),
  YAML_FIELD_INT (
    UndoableAction, num_actions),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  undoable_action_schema =
{
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    UndoableAction, undoable_action_fields_schema),
};

NONNULL
void
undoable_action_init_loaded (
  UndoableAction * self);

/**
 * Initializer to be used by implementing actions.
 */
NONNULL
void
undoable_action_init (
  UndoableAction *   self,
  UndoableActionType type);

/**
 * Returns whether the action requires pausing
 * the engine.
 */
NONNULL
bool
undoable_action_needs_pause (
  UndoableAction * self);

/**
 * Checks whether the action can contain an audio
 * clip.
 *
 * No attempt is made to remove unused files from
 * the pool for actions that can't contain audio
 * clips.
 */
NONNULL
bool
undoable_action_can_contain_clip (
  UndoableAction * self);


/**
 * Checks whether the action actually contains or
 * refers to the given audio clip.
 */
NONNULL
bool
undoable_action_contains_clip (
  UndoableAction * self,
  AudioClip *      clip);

/**
 * Sets the number of actions for this action.
 *
 * This should be set on the last action to be
 * performed.
 */
NONNULL
void
undoable_action_set_num_actions (
  UndoableAction * self,
  int              num_actions);

/**
 * To be used by actions that save/load port
 * connections.
 *
 * @param _do True if doing/performing, false if
 *   undoing.
 * @param before Pointer to the connections before.
 * @param after Pointer to the connections after.
 */
NONNULL_ARGS (1)
void
undoable_action_save_or_load_port_connections (
  UndoableAction *          self,
  bool                      _do,
  PortConnectionsManager ** before,
  PortConnectionsManager ** after);

/**
 * Performs the action.
 *
 * @note Only to be called by undo manager.
 *
 * @return Non-zero if errors occurred.
 */
NONNULL_ARGS (1)
int
undoable_action_do (
  UndoableAction * self,
  GError **        error);

/**
 * Undoes the action.
 *
 * @return Non-zero if errors occurred.
 */
NONNULL_ARGS (1)
int
undoable_action_undo (
  UndoableAction * self,
  GError **        error);

void
undoable_action_free (
  UndoableAction * self);

/**
 * Stringizes the action to be used in Undo/Redo
 * buttons.
 *
 * The string MUST be free'd using g_free().
 */
NONNULL
char *
undoable_action_to_string (
  UndoableAction * ua);

/**
 * @}
 */

#endif
