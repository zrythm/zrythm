/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "actions/tracklist_selections.h"
#  include "project.h"
#endif

#if 0
SCM_DEFINE (
  s_tracklist_selections_action_new_create_instrument,
  "tracklist-selections-action-new-create-instrument",
  3, 0, 0,
  (SCM pl_descr, SCM track_pos, SCM num_tracks),
  "Returns a new action for creating an instrument track.")
#  define FUNC_NAME s_
{
  UndoableAction * ua =
    tracklist_selections_action_new_create_instrument (
      scm_to_pointer (pl_descr),
      scm_to_int (track_pos),
      scm_to_int (num_tracks));

  return
    scm_from_pointer (ua, NULL);
}
#  undef FUNC_NAME

SCM_DEFINE (
  s_tracklist_selections_action_new_audio_fx_with_plugin,
  "tracklist-selections-action-new-create-audio-fx-with-plugin",
  3, 0, 0,
  (SCM plugin_descriptor, SCM track_pos,
   SCM num_tracks),
  "Returns a new action for creating one or more Audio FX tracks with a plugin.")
#  define FUNC_NAME s_
{
  UndoableAction * ua =
    tracklist_selections_action_new_create_audio_fx (
      scm_to_pointer (plugin_descriptor),
      scm_to_int (track_pos),
      scm_to_int (num_tracks));

  return
    scm_from_pointer (ua, NULL);
}
#  undef FUNC_NAME

SCM_DEFINE (
  s_tracklist_selections_action_new_audio_fx,
  "tracklist-selections-action-new-create-audio-fx",
  2, 0, 0,
  (SCM track_pos, SCM num_tracks),
  "Returns a new action for creating one or more empty Audio FX tracks.")
#  define FUNC_NAME s_
{
  UndoableAction * ua =
    tracklist_selections_action_new_create_audio_fx (
      NULL, scm_to_int (track_pos),
      scm_to_int (num_tracks));

  return
    scm_from_pointer (ua, NULL);
}
#  undef FUNC_NAME

SCM_DEFINE (
  s_tracklist_selections_action_new_midi,
  "tracklist-selections-action-new-midi",
  2, 0, 0,
  (SCM track_pos, SCM num_tracks),
  "Returns an action for creating one or more empty MIDI tracks.")
#  define FUNC_NAME s_
{
  UndoableAction * ua =
    tracklist_selections_action_new_create_midi (
      scm_to_int (track_pos),
      scm_to_int (num_tracks));

  return
    scm_from_pointer (ua, NULL);
}
#  undef FUNC_NAME
#endif

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "actions_tracklist_selections_action.x"
#endif

#if 0
  scm_c_export (
    "tracklist-selections-action-new-create-instrument",
    "tracklist-selections-action-new-create-audio-fx-with-plugin",
    "tracklist-selections-action-new-create-audio-fx",
    "tracklist-selections-action-new-midi",
    NULL);
#endif
}

void
guile_actions_tracklist_selections_action_define_module (void)
{
  scm_c_define_module ("actions tracklist-selections-action", init_module, NULL);
}
