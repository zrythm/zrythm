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
#include "actions/create_tracks_action.h"
#include "project.h"
#endif

SCM_DEFINE (
  s_create_tracks_action_new_with_plugin,
  "create-tracks-action-new-with-plugin", 4, 0, 0,
  (SCM type, SCM pl_descr,
   SCM track_pos, SCM num_tracks),
  "Returns a new Create Tracks action for a plugin.")
{
  UndoableAction * ua =
    create_tracks_action_new (
      scm_to_int (type),
      scm_to_pointer (pl_descr),
      NULL,
      scm_to_int (track_pos),
      NULL,
      scm_to_int (num_tracks));

  return
    scm_from_pointer (ua, NULL);
}

SCM_DEFINE (
  s_create_tracks_action_new_audio_fx,
  "create-tracks-action-new-audio-fx", 2, 0, 0,
  (SCM track_pos, SCM num_tracks),
  "Returns a new Create Tracks action for one or more empty Audio FX tracks.")
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS, NULL, NULL,
      scm_to_int (track_pos), NULL,
      scm_to_int (num_tracks));

  return
    scm_from_pointer (ua, NULL);
}

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#include "actions_create_tracks_action.x"
#endif

  scm_c_export (
    "create-tracks-action-new-with-plugin",
    "create-tracks-action-new-audio-fx",
    NULL);
}

void
guile_actions_create_tracks_action_define_module (void)
{
  scm_c_define_module (
    "actions create-tracks-action", init_module, NULL);
}
