/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
  PortIdentifier * src_id = (PortIdentifier *) scm_to_pointer (src_port_id);
  PortIdentifier * dest_id = (PortIdentifier *) scm_to_pointer (dest_port_id);

  GError * err = NULL;
  bool     ret = port_connection_action_perform_connect (src_id, dest_id, &err);
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
  scm_c_export ("port-connection-action-perform-connect", NULL);
}

void
guile_actions_port_connection_action_define_module (void)
{
  scm_c_define_module ("actions port-connection-action", init_module, NULL);
}
