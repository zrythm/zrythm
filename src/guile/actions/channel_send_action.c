/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
