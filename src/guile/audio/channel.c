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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#include "audio/channel.h"
#include "project.h"
#endif

SCM_DEFINE (
  s_channel_get_send, "channel-get-send", 2, 0, 0,
  (SCM channel, SCM send_slot),
  "Returns the send instance at the given slot")
{
  Channel * ch = (Channel *) scm_to_pointer (channel);

  return
    scm_from_pointer (
      &ch->sends[scm_to_int (send_slot)], NULL);
}

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#include "audio_channel.x"
#endif

  scm_c_export (
    "channel-get-send", NULL);
}

void
guile_audio_channel_define_module (void)
{
  scm_c_define_module (
    "audio channel", init_module, NULL);
}
