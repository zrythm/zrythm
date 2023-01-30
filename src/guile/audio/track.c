// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "audio/track.h"
#  include "project.h"
#  include "utils/error.h"
#  include "utils/flags.h"
#endif

SCM_DEFINE (
  s_midi_track_new,
  "midi-track-new",
  2,
  0,
  0,
  (SCM idx, SCM name),
  "Returns a new track with name @var{name} to be "
  "placed at position @var{idx} in the tracklist.")
#define FUNC_NAME s_
{
  Track * track = track_new (
    TRACK_TYPE_MIDI, scm_to_int (idx),
    scm_to_locale_string (name), F_WITH_LANE);

  return scm_from_pointer (track, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_track_get_name,
  "track-get-name",
  1,
  0,
  0,
  (SCM track),
  "Returns the name of @var{track}.")
#define FUNC_NAME s_
{
  Track * reftrack = scm_to_pointer (track);

  return scm_from_utf8_string (track_get_name (reftrack));
}
#undef FUNC_NAME

SCM_DEFINE (
  s_track_get_processor,
  "track-get-processor",
  1,
  0,
  0,
  (SCM track),
  "Returns the processor of @var{track}.")
#define FUNC_NAME s_
{
  Track * reftrack = scm_to_pointer (track);

  return scm_from_pointer (reftrack->processor, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_track_get_channel,
  "track-get-channel",
  1,
  0,
  0,
  (SCM track),
  "Returns the channel of @var{track}.")
#define FUNC_NAME s_
{
  Track * reftrack = scm_to_pointer (track);

  return scm_from_pointer (reftrack->channel, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_track_set_soloed,
  "track-set-soloed",
  2,
  0,
  0,
  (SCM track, SCM solo),
  "Sets whether @var{track} is soloed or not. This creates an undoable action and performs it.")
#define FUNC_NAME s_
{
  Track * reftrack = scm_to_pointer (track);

  track_set_soloed (
    reftrack, scm_to_bool (solo), F_TRIGGER_UNDO,
    F_AUTO_SELECT, F_PUBLISH_EVENTS);

  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE (
  s_track_set_muted,
  "track-set-muted",
  2,
  0,
  0,
  (SCM track, SCM muted),
  "Sets whether @var{track} is muted or not. This creates an undoable action and performs it.")
#define FUNC_NAME s_
{
  Track * reftrack = scm_to_pointer (track);

  track_set_muted (
    reftrack, scm_to_bool (muted), F_TRIGGER_UNDO,
    F_AUTO_SELECT, F_PUBLISH_EVENTS);

  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE (
  s_add_region,
  "track-add-lane-region",
  3,
  0,
  0,
  (SCM track, SCM region, SCM lane_pos),
  "Adds @var{region} to track @var{track}. To "
  "be used for regions with lanes (midi/audio)")
#define FUNC_NAME s_
{
  GError * err = NULL;
  bool     success = track_add_region (
    scm_to_pointer (track), scm_to_pointer (region), NULL,
    scm_to_int (lane_pos), true, true, &err);
  if (!success)
    {
      HANDLE_ERROR (err, "%s", "Failed to add region");
    }

  return SCM_BOOL_T;
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_track.x"
#endif

  scm_c_export (
    "midi-track-new", "track-add-lane-region",
    "track-get-name", "track-get-channel",
    "track-get-processor", "track-set-muted", NULL);
}

void
guile_audio_track_define_module (void)
{
  scm_c_define_module ("audio track", init_module, NULL);
}
