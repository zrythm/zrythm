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

#include "actions/port_connection_action.h"
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
port_connection_action_init_loaded (
  PortConnectionAction * self)
{
  /* no need */
}

/**
 * Create a new action.
 */
UndoableAction *
port_connection_action_new (
  PortConnectionActionType type,
  PortIdentifier *         src_id,
  PortIdentifier *         dest_id,
  float                    new_val)
{
  PortConnectionAction * self =
    object_new (PortConnectionAction);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_PORT_CONNECTION;

  self->src_port_id = *src_id;
  self->dest_port_id = *dest_id;
  self->type = type;
  self->val = new_val;

  return ua;
}

static int
port_connection_action_do_or_undo (
  PortConnectionAction * self,
  bool                   _do)
{
  Port * src =
    port_find_from_identifier (&self->src_port_id);
  Port * dest =
    port_find_from_identifier (&self->dest_port_id);

  switch (self->type)
    {
    case PORT_CONNECTION_CONNECT:
    case PORT_CONNECTION_DISCONNECT:
      if ((self->type ==
             PORT_CONNECTION_CONNECT && _do) ||
          (self->type ==
             PORT_CONNECTION_DISCONNECT && !_do))
        {
          if (!ports_can_be_connected (src, dest))
            {
              g_warning (
                "ports cannot be connected");
              return -1;
            }
          port_connect (src, dest, false);
        }
      else
        {
          port_disconnect (src, dest);
        }
      router_recalc_graph (ROUTER, F_NOT_SOFT);
      break;
    case PORT_CONNECTION_ENABLE:
      port_set_enabled (
        src, dest, _do ? true : false);
      break;
    case PORT_CONNECTION_DISABLE:
      port_set_enabled (
        src, dest, _do ? false : true);
      break;
    case PORT_CONNECTION_CHANGE_MULTIPLIER:
      {
        float val_before =
          port_get_multiplier (src, dest);
        port_set_multiplier (src, dest, self->val);
        self->val = val_before;
      }
      break;
    default:
      break;
    }

  EVENTS_PUSH (ET_PORT_CONNECTION_CHANGED, NULL);

  return 0;
}

int
port_connection_action_do (
  PortConnectionAction * self)
{
  return
    port_connection_action_do_or_undo (self, true);
}

int
port_connection_action_undo (
  PortConnectionAction * self)
{
  return
    port_connection_action_do_or_undo (self, false);
}

char *
port_connection_action_stringize (
  PortConnectionAction * self)
{
  switch (self->type)
    {
    case PORT_CONNECTION_CONNECT:
      return g_strdup (_("Connect ports"));
      break;
    case PORT_CONNECTION_DISCONNECT:
      return g_strdup (_("Disconnect ports"));
      break;
    case PORT_CONNECTION_ENABLE:
      return g_strdup (_("Enable port connection"));
      break;
    case PORT_CONNECTION_DISABLE:
      return g_strdup (_("Disable port connection"));
      break;
    case PORT_CONNECTION_CHANGE_MULTIPLIER:
      return g_strdup (_("Change port connection"));
      break;
    default:
      g_warn_if_reached ();
    }
  g_return_val_if_reached (NULL);
}

void
port_connection_action_free (
  PortConnectionAction * self)
{
  object_zero_and_free (self);
}
