// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/port_action.h"
#include "dsp/control_port.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
port_action_init_loaded (PortAction * self)
{
  /* no need */
}

/**
 * Create a new action.
 */
UndoableAction *
port_action_new (
  PortActionType   type,
  PortIdentifier * port_id,
  float            val,
  bool             is_normalized,
  GError **        error)
{
  PortAction *     self = object_new (PortAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UndoableActionType::UA_PORT);

  self->port_id = *port_id;
  self->type = type;
  if (is_normalized)
    {
      Port * port = Port::find_from_identifier (port_id);
      self->val = control_port_normalized_val_to_real (port, val);
    }
  else
    {
      self->val = val;
    }

  return ua;
}

/**
 * Create a new action.
 */
UndoableAction *
port_action_new_reset_control (PortIdentifier * port_id, GError ** error)
{
  Port * port = Port::find_from_identifier (port_id);

  return port_action_new (
    PortActionType::PORT_ACTION_SET_CONTROL_VAL, port_id, port->deff,
    F_NOT_NORMALIZED, error);
}

PortAction *
port_action_clone (const PortAction * src)
{
  PortAction * self = object_new (PortAction);
  self->parent_instance = src->parent_instance;

  self->type = src->type;
  self->port_id = src->port_id;
  self->val = src->val;

  return self;
}

bool
port_action_perform (
  PortActionType   type,
  PortIdentifier * port_id,
  float            val,
  bool             is_normalized,
  GError **        error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    port_action_new, error, type, port_id, val, is_normalized, error);
}

bool
port_action_perform_reset_control (PortIdentifier * port_id, GError ** error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    port_action_new_reset_control, error, port_id, error);
}

static int
port_action_do_or_undo (PortAction * self, bool _do)
{
  Port * port = Port::find_from_identifier (&self->port_id);

  switch (self->type)
    {
    case PortActionType::PORT_ACTION_SET_CONTROL_VAL:
      {
        float val_before = control_port_get_val (port);
        port_set_control_value (
          port, self->val, F_NOT_NORMALIZED, F_PUBLISH_EVENTS);
        self->val = val_before;
      }
      break;
    default:
      break;
    }

  return 0;
}

int
port_action_do (PortAction * self, GError ** error)
{
  return port_action_do_or_undo (self, true);
}

int
port_action_undo (PortAction * self, GError ** error)
{
  return port_action_do_or_undo (self, false);
}

char *
port_action_stringize (PortAction * self)
{
  switch (self->type)
    {
    case PortActionType::PORT_ACTION_SET_CONTROL_VAL:
      return g_strdup (_ ("Set control value"));
      break;
    default:
      g_warn_if_reached ();
    }
  g_return_val_if_reached (NULL);
}

void
port_action_free (PortAction * self)
{
  object_zero_and_free (self);
}
