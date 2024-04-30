// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Undoable actions.
 */

#ifndef __UNDO_UNDOABLE_ACTION_H__
#define __UNDO_UNDOABLE_ACTION_H__

#include <stdbool.h>

#include "utils/types.h"
#include "utils/yaml.h"

typedef struct AudioClip              AudioClip;
typedef struct PortConnectionsManager PortConnectionsManager;

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

  UA_CHORD,

} UndoableActionType;

/**
 * Base struct to be inherited by implementing
 * undoable actions.
 */
typedef struct UndoableAction
{
  /** Undoable action type. */
  UndoableActionType type;

  /** A snapshot of AudioEngine.frames_per_tick when the
   * action is executed. */
  double frames_per_tick;

  /**
   * Sample rate of this action.
   *
   * Used to recalculate UndoableAction.frames_per_tick when
   * the project is loaded under a new samplerate.
   */
  sample_rate_t sample_rate;

  /**
   * Index in the stack.
   *
   * Used during deserialization.
   */
  int stack_idx;

  /**
   * Number of actions to perform.
   *
   * This is used to group multiple actions into
   * one logical action (eg, create a group track
   * and route multiple tracks to it).
   *
   * To be set on the last action being performed.
   */
  int num_actions;
} UndoableAction;

NONNULL void
undoable_action_init_loaded (UndoableAction * self);

/**
 * Initializer to be used by implementing actions.
 */
NONNULL void
undoable_action_init (UndoableAction * self, UndoableActionType type);

/**
 * Returns whether the action requires pausing
 * the engine.
 */
NONNULL bool
undoable_action_needs_pause (UndoableAction * self);

/**
 * Checks whether the action can contain an audio
 * clip.
 *
 * No attempt is made to remove unused files from
 * the pool for actions that can't contain audio
 * clips.
 */
NONNULL bool
undoable_action_can_contain_clip (UndoableAction * self);

/**
 * Checks whether the action actually contains or
 * refers to the given audio clip.
 */
NONNULL bool
undoable_action_contains_clip (UndoableAction * self, AudioClip * clip);

NONNULL void
undoable_action_get_plugins (UndoableAction * self, GPtrArray * arr);

/**
 * Sets the number of actions for this action.
 *
 * This should be set on the last action to be
 * performed.
 */
NONNULL void
undoable_action_set_num_actions (UndoableAction * self, int num_actions);

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
void undoable_action_save_or_load_port_connections (
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
int undoable_action_do (UndoableAction * self, GError ** error);

/**
 * Undoes the action.
 *
 * @return Non-zero if errors occurred.
 */
NONNULL_ARGS (1)
int undoable_action_undo (UndoableAction * self, GError ** error);

void
undoable_action_free (UndoableAction * self);

/**
 * Stringizes the action to be used in Undo/Redo
 * buttons.
 *
 * The string MUST be free'd using g_free().
 */
NONNULL char *
undoable_action_to_string (UndoableAction * ua);

/**
 * @}
 */

#endif
