// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
  float                    new_val,
  GError **                error)
{
  PortConnectionAction * self =
    object_new (PortConnectionAction);
  UndoableAction * ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_PORT_CONNECTION);

  /* check for existing connection to get values */
  const PortConnection * conn =
    port_connections_manager_find_connection (
      PORT_CONNECTIONS_MGR, src_id, dest_id);
  if (conn)
    self->connection = port_connection_clone (conn);
  else
    self->connection = port_connection_new (
      src_id, dest_id, 1.f, F_NOT_LOCKED, F_ENABLE);
  self->type = type;
  self->val = new_val;

  return ua;
}

PortConnectionAction *
port_connection_action_clone (const PortConnectionAction * src)
{
  PortConnectionAction * self =
    object_new (PortConnectionAction);
  self->parent_instance = src->parent_instance;

  self->type = src->type;
  self->connection = port_connection_clone (src->connection);
  self->val = src->val;

  return self;
}

bool
port_connection_action_perform (
  PortConnectionActionType type,
  PortIdentifier *         src_id,
  PortIdentifier *         dest_id,
  float                    new_val,
  GError **                error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    port_connection_action_new, error, type, src_id, dest_id,
    new_val, error);
}

static int
port_connection_action_do_or_undo (
  PortConnectionAction * self,
  bool                   _do,
  GError **              error)
{
  Port * src =
    port_find_from_identifier (self->connection->src_id);
  Port * dest =
    port_find_from_identifier (self->connection->dest_id);
  g_return_val_if_fail (src && dest, -1);
  PortConnection * prj_connection =
    port_connections_manager_find_connection (
      PORT_CONNECTIONS_MGR, self->connection->src_id,
      self->connection->dest_id);

  switch (self->type)
    {
    case PORT_CONNECTION_CONNECT:
    case PORT_CONNECTION_DISCONNECT:
      if (
        (self->type == PORT_CONNECTION_CONNECT && _do)
        || (self->type == PORT_CONNECTION_DISCONNECT && !_do))
        {
          if (!ports_can_be_connected (src, dest))
            {
              g_warning ("ports cannot be connected");
              return -1;
            }
          port_connections_manager_ensure_connect (
            PORT_CONNECTIONS_MGR, &src->id, &dest->id, 1.f,
            F_NOT_LOCKED, F_ENABLE);

          /* set base value if cv -> control */
          if (
            src->id.type == TYPE_CV
            && dest->id.type == TYPE_CONTROL)
            dest->base_value = dest->control;
        }
      else
        {
          port_connections_manager_ensure_disconnect (
            PORT_CONNECTIONS_MGR, &src->id, &dest->id);
        }
      router_recalc_graph (ROUTER, F_NOT_SOFT);
      break;
    case PORT_CONNECTION_ENABLE:
      prj_connection->enabled = _do ? true : false;
      break;
    case PORT_CONNECTION_DISABLE:
      prj_connection->enabled = _do ? false : true;
      break;
    case PORT_CONNECTION_CHANGE_MULTIPLIER:
      {
        float val_before = prj_connection->multiplier;
        prj_connection->multiplier = self->val;
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
  PortConnectionAction * self,
  GError **              error)
{
  return port_connection_action_do_or_undo (self, true, error);
}

int
port_connection_action_undo (
  PortConnectionAction * self,
  GError **              error)
{
  return port_connection_action_do_or_undo (
    self, false, error);
}

char *
port_connection_action_stringize (PortConnectionAction * self)
{
  switch (self->type)
    {
    case PORT_CONNECTION_CONNECT:
      return g_strdup (_ ("Connect ports"));
      break;
    case PORT_CONNECTION_DISCONNECT:
      return g_strdup (_ ("Disconnect ports"));
      break;
    case PORT_CONNECTION_ENABLE:
      return g_strdup (_ ("Enable port connection"));
      break;
    case PORT_CONNECTION_DISABLE:
      return g_strdup (_ ("Disable port connection"));
      break;
    case PORT_CONNECTION_CHANGE_MULTIPLIER:
      return g_strdup (_ ("Change port connection"));
      break;
    default:
      g_warn_if_reached ();
    }
  g_return_val_if_reached (NULL);
}

void
port_connection_action_free (PortConnectionAction * self)
{
  object_free_w_func_and_null (
    port_connection_free, self->connection);

  object_zero_and_free (self);
}
