/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/port_action.h"
#include "audio/control_port.h"
#include "audio/port.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
port_action_init_loaded (
  PortAction * self)
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
  bool             is_normalized)
{
  PortAction * self =
    object_new (PortAction);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_PORT;

  self->port_id = *port_id;
  self->type = type;
  if (is_normalized)
    {
      Port * port =
        port_find_from_identifier (port_id);
      self->val =
        control_port_normalized_val_to_real (
          port, val);
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
port_action_new_reset_control (
  PortIdentifier * port_id)
{
  Port * port =
    port_find_from_identifier (port_id);

  return
    port_action_new (
      PORT_ACTION_SET_CONTROL_VAL, port_id,
      port->deff, F_NOT_NORMALIZED);
}

static int
port_action_do_or_undo (
  PortAction * self,
  bool                   _do)
{
  Port * port =
    port_find_from_identifier (&self->port_id);

  switch (self->type)
    {
    case PORT_ACTION_SET_CONTROL_VAL:
      {
        float val_before =
          control_port_get_val (port);
        port_set_control_value (
          port, self->val, F_NOT_NORMALIZED,
          F_PUBLISH_EVENTS);
        self->val = val_before;
      }
      break;
    default:
      break;
    }

  return 0;
}

int
port_action_do (
  PortAction * self)
{
  return
    port_action_do_or_undo (self, true);
}

int
port_action_undo (
  PortAction * self)
{
  return
    port_action_do_or_undo (self, false);
}

char *
port_action_stringize (
  PortAction * self)
{
  switch (self->type)
    {
    case PORT_ACTION_SET_CONTROL_VAL:
      return g_strdup (_("Set control value"));
      break;
    default:
      g_warn_if_reached ();
    }
  g_return_val_if_reached (NULL);
}

void
port_action_free (
  PortAction * self)
{
  object_zero_and_free (self);
}
