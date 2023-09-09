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
#  include "actions/channel_send_action.h"
#  include "project.h"
#  include "utils/error.h"

#  include <glib/gi18n.h>
#endif

SCM_DEFINE (
  s_channel_send_action_perform_connect_audio,
  "channel-send-action-perform-connect-audio",
  2,
  0,
  0,
  (SCM send, SCM stereo_ports),
  "Creates an audio send connection as an undoable action.")
#define FUNC_NAME s_
{
  GError * err = NULL;
  bool     ret = channel_send_action_perform_connect_audio (
    scm_to_pointer (send), scm_to_pointer (stereo_ports), &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to create audio send"));
    }

  return scm_from_bool (ret);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "actions_channel_send_action.x"
#endif

  scm_c_export ("channel-send-action-perform-connect-audio", NULL);
}

void
guile_actions_channel_send_action_define_module (void)
{
  scm_c_define_module ("actions channel-send-action", init_module, NULL);
}
