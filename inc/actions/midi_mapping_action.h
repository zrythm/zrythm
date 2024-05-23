// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_MIDI_MAPPING_ACTION_H__
#define __UNDO_MIDI_MAPPING_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/ext_port.h"
#include "dsp/midi_mapping.h"
#include "dsp/port_identifier.h"
#include "utils/types.h"

/**
 * @addtogroup actions
 *
 * @{
 */

enum class MidiMappingActionType
{
  MIDI_MAPPING_ACTION_BIND,
  MIDI_MAPPING_ACTION_UNBIND,
  MIDI_MAPPING_ACTION_ENABLE,
  MIDI_MAPPING_ACTION_DISABLE,
};

/**
 * MIDI mapping action.
 */
typedef struct MidiMappingAction
{
  UndoableAction parent_instance;

  /** Index of mapping, if enable/disable. */
  int idx;

  /** Action type. */
  MidiMappingActionType type;

  PortIdentifier * dest_port_id;

  ExtPort * dev_port;

  midi_byte_t buf[3];

} MidiMappingAction;

void
midi_mapping_action_init_loaded (MidiMappingAction * self);

/**
 * Creates a new action.
 */
WARN_UNUSED_RESULT UndoableAction *
midi_mapping_action_new_enable (int idx, bool enable, GError ** error);

/**
 * Creates a new action.
 */
WARN_UNUSED_RESULT UndoableAction *
midi_mapping_action_new_bind (
  midi_byte_t * buf,
  ExtPort *     device_port,
  Port *        dest_port,
  GError **     error);

/**
 * Creates a new action.
 */
WARN_UNUSED_RESULT UndoableAction *
midi_mapping_action_new_unbind (int idx, GError ** error);

NONNULL MidiMappingAction *
midi_mapping_action_clone (const MidiMappingAction * src);

/**
 * Wrapper of midi_mapping_action_new_enable().
 */
bool
midi_mapping_action_perform_enable (int idx, bool enable, GError ** error);

/**
 * Wrapper of midi_mapping_action_new_bind().
 */
bool
midi_mapping_action_perform_bind (
  midi_byte_t * buf,
  ExtPort *     device_port,
  Port *        dest_port,
  GError **     error);

/**
 * Wrapper of midi_mapping_action_new_unbind().
 */
bool
midi_mapping_action_perform_unbind (int idx, GError ** error);

int
midi_mapping_action_do (MidiMappingAction * self, GError ** error);

int
midi_mapping_action_undo (MidiMappingAction * self, GError ** error);

char *
midi_mapping_action_stringize (MidiMappingAction * self);

void
midi_mapping_action_free (MidiMappingAction * self);

/**
 * @}
 */

#endif
