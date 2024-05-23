// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ACTION_PORT_CONNECTION_ACTION_H__
#define __ACTION_PORT_CONNECTION_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/port_connection.h"

/**
 * @addtogroup actions
 *
 * @{
 */

enum class PortConnectionActionType
{
  PORT_CONNECTION_CONNECT,
  PORT_CONNECTION_DISCONNECT,
  PORT_CONNECTION_ENABLE,
  PORT_CONNECTION_DISABLE,
  PORT_CONNECTION_CHANGE_MULTIPLIER,
};

typedef struct PortConnectionAction
{
  UndoableAction parent_instance;

  PortConnectionActionType type;

  PortConnection * connection;

  /**
   * Value before/after the change.
   *
   * To be swapped on undo/redo.
   */
  float val;
} PortConnectionAction;

void
port_connection_action_init_loaded (PortConnectionAction * self);

/**
 * Create a new action.
 */
WARN_UNUSED_RESULT UndoableAction *
port_connection_action_new (
  PortConnectionActionType type,
  PortIdentifier *         src_id,
  PortIdentifier *         dest_id,
  float                    new_val,
  GError **                error);

#define port_connection_action_new_connect(src_id, dest_id, error) \
  port_connection_action_new ( \
    PortConnectionActionType::PORT_CONNECTION_CONNECT, src_id, dest_id, 0.f, \
    error)

#define port_connection_action_new_disconnect(src_id, dest_id, error) \
  port_connection_action_new ( \
    PortConnectionActionType::PORT_CONNECTION_DISCONNECT, src_id, dest_id, \
    0.f, error)

#define port_connection_action_new_enable(src_id, dest_id, enable, error) \
  port_connection_action_new ( \
    enable \
      ? PortConnectionActionType::PORT_CONNECTION_ENABLE \
      : PortConnectionActionType::PORT_CONNECTION_DISABLE, \
    src_id, dest_id, 0.f, error)

#define port_connection_action_new_change_multiplier( \
  src_id, dest_id, new_multiplier, error) \
  port_connection_action_new ( \
    PortConnectionActionType::PORT_CONNECTION_CHANGE_MULTIPLIER, src_id, \
    dest_id, new_multiplier, error)

NONNULL PortConnectionAction *
port_connection_action_clone (const PortConnectionAction * src);

bool
port_connection_action_perform (
  PortConnectionActionType type,
  PortIdentifier *         src_id,
  PortIdentifier *         dest_id,
  float                    new_val,
  GError **                error);

#define port_connection_action_perform_connect(src_id, dest_id, error) \
  port_connection_action_perform ( \
    PortConnectionActionType::PORT_CONNECTION_CONNECT, src_id, dest_id, 0.f, \
    error)

#define port_connection_action_perform_disconnect(src_id, dest_id, error) \
  port_connection_action_perform ( \
    PortConnectionActionType::PORT_CONNECTION_DISCONNECT, src_id, dest_id, \
    0.f, error)

#define port_connection_action_perform_enable(src_id, dest_id, enable, error) \
  port_connection_action_perform ( \
    enable \
      ? PortConnectionActionType::PORT_CONNECTION_ENABLE \
      : PortConnectionActionType::PORT_CONNECTION_DISABLE, \
    src_id, dest_id, 0.f, error)

#define port_connection_action_perform_change_multiplier( \
  src_id, dest_id, new_multiplier, error) \
  port_connection_action_perform ( \
    PortConnectionActionType::PORT_CONNECTION_CHANGE_MULTIPLIER, src_id, \
    dest_id, new_multiplier, error)

int
port_connection_action_do (PortConnectionAction * self, GError ** error);

int
port_connection_action_undo (PortConnectionAction * self, GError ** error);

char *
port_connection_action_stringize (PortConnectionAction * self);

void
port_connection_action_free (PortConnectionAction * self);

/**
 * @}
 */

#endif
