// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Undo stack.
 */

#ifndef __UNDO_UNDO_STACK_H__
#define __UNDO_UNDO_STACK_H__

#include "actions/arranger_selections.h"
#include "actions/channel_send_action.h"
#include "actions/chord_action.h"
#include "actions/midi_mapping_action.h"
#include "actions/mixer_selections_action.h"
#include "actions/port_action.h"
#include "actions/port_connection_action.h"
#include "actions/range_action.h"
#include "actions/tracklist_selections.h"
#include "actions/transport_action.h"
#include "utils/stack.h"
#include "utils/yaml.h"

typedef struct AudioClip AudioClip;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Serializable stack for undoable actions.
 *
 * This is used for both undo and redo.
 */
typedef struct UndoStack
{
  /** Actual stack used at runtime. */
  Stack * stack;

  /* the following are for serialization purposes only */

  ArrangerSelectionsAction ** as_actions;
  size_t                      num_as_actions;
  size_t                      as_actions_size;

  MixerSelectionsAction ** mixer_selections_actions;
  size_t                   num_mixer_selections_actions;
  size_t                   mixer_selections_actions_size;

  TracklistSelectionsAction ** tracklist_selections_actions;
  size_t                       num_tracklist_selections_actions;
  size_t                       tracklist_selections_actions_size;

  ChannelSendAction ** channel_send_actions;
  size_t               num_channel_send_actions;
  size_t               channel_send_actions_size;

  PortConnectionAction ** port_connection_actions;
  size_t                  num_port_connection_actions;
  size_t                  port_connection_actions_size;

  PortAction ** port_actions;
  size_t        num_port_actions;
  size_t        port_actions_size;

  MidiMappingAction ** midi_mapping_actions;
  size_t               num_midi_mapping_actions;
  size_t               midi_mapping_actions_size;

  RangeAction ** range_actions;
  size_t         num_range_actions;
  size_t         range_actions_size;

  TransportAction ** transport_actions;
  size_t             num_transport_actions;
  size_t             transport_actions_size;

  ChordAction ** chord_actions;
  size_t         num_chord_actions;
  size_t         chord_actions_size;

} UndoStack;

void
undo_stack_init_loaded (UndoStack * self);

/**
 * Creates a new stack for undoable actions.
 */
UndoStack *
undo_stack_new (void);

NONNULL UndoStack *
undo_stack_clone (const UndoStack * src);

/**
 * Gets the list of actions as a string.
 */
char *
undo_stack_get_as_string (UndoStack * self, int limit);

/**
 * Returns the total cached actions.
 *
 * Used when loading projects and for error checking.
 */
NONNULL size_t
undo_stack_get_total_cached_actions (UndoStack * self);

/* --- start wrappers --- */

#define undo_stack_size(x) (stack_size ((x)->stack))

#define undo_stack_is_empty(x) (stack_is_empty ((x)->stack))

#define undo_stack_is_full(x) (stack_is_full ((x)->stack))

#define undo_stack_peek(x) ((UndoableAction *) stack_peek ((x)->stack))

#define undo_stack_peek_last(x) \
  ((UndoableAction *) stack_peek_last ((x)->stack))

void
undo_stack_push (UndoStack * self, UndoableAction * action);

UndoableAction *
undo_stack_pop (UndoStack * self);

/**
 * Pops the last element and moves everything back.
 */
UndoableAction *
undo_stack_pop_last (UndoStack * self);

/* --- end wrappers --- */

bool
undo_stack_contains_clip (UndoStack * self, AudioClip * clip);

/**
 * Checks if the undo stack contains the given
 * action pointer.
 */
bool
undo_stack_contains_action (UndoStack * self, UndoableAction * ua);

/**
 * Returns the plugins referred to in the undo stack.
 */
NONNULL void
undo_stack_get_plugins (UndoStack * self, GPtrArray * arr);

/**
 * Clears the stack, optionally freeing all the
 * elements.
 */
void
undo_stack_clear (UndoStack * self, bool free);

void
undo_stack_free (UndoStack * self);

/**
 * @}
 */

#endif
