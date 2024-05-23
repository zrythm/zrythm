// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ACTION_PORT_ACTION_H__
#define __ACTION_PORT_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/port_identifier.h"

/**
 * @addtogroup actions
 *
 * @{
 */

enum class PortActionType
{
  /** Set control port value. */
  PORT_ACTION_SET_CONTROL_VAL,
};

typedef struct PortAction
{
  UndoableAction parent_instance;

  PortActionType type;

  PortIdentifier port_id;

  /**
   * Real (not normalized) value before/after the
   * change.
   *
   * To be swapped on undo/redo.
   */
  float val;
} PortAction;

void
port_action_init_loaded (PortAction * self);

/**
 * Create a new action.
 */
WARN_UNUSED_RESULT UndoableAction *
port_action_new (
  PortActionType   type,
  PortIdentifier * port_id,
  float            val,
  bool             is_normalized,
  GError **        error);

/**
 * Create a new action.
 */
WARN_UNUSED_RESULT UndoableAction *
port_action_new_reset_control (PortIdentifier * port_id, GError ** error);

NONNULL PortAction *
port_action_clone (const PortAction * src);

bool
port_action_perform (
  PortActionType   type,
  PortIdentifier * port_id,
  float            val,
  bool             is_normalized,
  GError **        error);

bool
port_action_perform_reset_control (PortIdentifier * port_id, GError ** error);

int
port_action_do (PortAction * self, GError ** error);

int
port_action_undo (PortAction * self, GError ** error);

char *
port_action_stringize (PortAction * self);

void
port_action_free (PortAction * self);

/**
 * @}
 */

#endif
