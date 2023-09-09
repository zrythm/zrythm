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

typedef enum PortActionType
{
  /** Set control port value. */
  PORT_ACTION_SET_CONTROL_VAL,
} PortActionType;

static const cyaml_strval_t port_action_type_strings[] = {
  {"Set control val", PORT_ACTION_SET_CONTROL_VAL},
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

static const cyaml_schema_field_t port_action_fields_schema[] = {
  YAML_FIELD_MAPPING_EMBEDDED (
    PortAction,
    parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (PortAction, type, port_action_type_strings),
  YAML_FIELD_MAPPING_EMBEDDED (PortAction, port_id, port_identifier_fields_schema),
  YAML_FIELD_FLOAT (PortAction, val),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t port_action_schema = {
  YAML_VALUE_PTR (PortAction, port_action_fields_schema),
};

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
