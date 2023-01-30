/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "actions/port_connection_action.h"
#  include "project.h"
#  include "utils/error.h"

#  include <glib/gi18n.h>
#endif

SCM_DEFINE (
  s_port_connection_action_perform_connect,
  "port-connection-action-perform-connect",
  2,
  0,
  0,
  (SCM src_port_id, SCM dest_port_id),
  "Connects 2 ports as an undoable action.")
#define FUNC_NAME s_
{
  PortIdentifier * src_id =
    (PortIdentifier *) scm_to_pointer (src_port_id);
  PortIdentifier * dest_id =
    (PortIdentifier *) scm_to_pointer (dest_port_id);

  GError * err = NULL;
  bool     ret = port_connection_action_perform_connect (
    src_id, dest_id, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to connect ports"));
    }

  return scm_from_bool (ret);
}
#undef FUNC_NAME

void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "actions_port_connection_action.x"
#endif
  scm_c_export (
    "port-connection-action-perform-connect", NULL);
}

void
guile_actions_port_connection_action_define_module (void)
{
  scm_c_define_module (
    "actions port-connection-action", init_module, NULL);
}
