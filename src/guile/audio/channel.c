/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "dsp/channel.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_channel_get_insert,
  "channel-get-insert",
  2,
  0,
  0,
  (SCM channel, SCM insert_slot),
  "Returns the plugin at the given insert slot")
#define FUNC_NAME s_
{
  Channel * ch = (Channel *) scm_to_pointer (channel);

  return scm_from_pointer (ch->inserts[scm_to_int (insert_slot)], NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_channel_get_instrument,
  "channel-get-instrument",
  1,
  0,
  0,
  (SCM channel),
  "Returns the instrument at the given channel")
#define FUNC_NAME s_
{
  Channel * ch = (Channel *) scm_to_pointer (channel);

  return scm_from_pointer (ch->instrument, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_channel_get_send,
  "channel-get-send",
  2,
  0,
  0,
  (SCM channel, SCM send_slot),
  "Returns the send instance at the given slot")
#define FUNC_NAME s_
{
  Channel * ch = (Channel *) scm_to_pointer (channel);

  return scm_from_pointer (&ch->sends[scm_to_int (send_slot)], NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_channel.x"
#endif

  scm_c_export (
    "channel-get-insert", "channel-get-instrument", "channel-get-send", NULL);
}

void
guile_audio_channel_define_module (void)
{
  scm_c_define_module ("audio channel", init_module, NULL);
}
