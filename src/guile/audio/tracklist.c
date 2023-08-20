// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "dsp/tracklist.h"
#  include "project.h"
#  include "zrythm_app.h"
#endif

SCM_DEFINE (
  s_tracklist_insert_track,
  "tracklist-insert-track",
  3,
  0,
  0,
  (SCM tracklist, SCM track, SCM idx),
  "Inserts track @var{track} at index @var{idx} in "
  "the tracklist.")
#define FUNC_NAME s_
{
  Tracklist * tl = (Tracklist *) scm_to_pointer (tracklist);

  tracklist_insert_track (
    tl, scm_to_pointer (track), scm_to_int (idx), true, true);

  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE (
  s_tracklist_get_track_at_pos,
  "tracklist-get-track-at-pos",
  2,
  0,
  0,
  (SCM tracklist, SCM pos),
  "Returns the track at @var{pos} in the tracklist.")
#define FUNC_NAME s_
{
  Tracklist * tl = (Tracklist *) scm_to_pointer (tracklist);

  return scm_from_pointer (tl->tracks[scm_to_int (pos)], NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_tracklist_get_num_tracks,
  "tracklist-get-num-tracks",
  1,
  0,
  0,
  (SCM tracklist),
  "Returns the number of tracks in the tracklist.")
#define FUNC_NAME s_
{
  Tracklist * tl = (Tracklist *) scm_to_pointer (tracklist);

  return scm_from_int (tl->num_tracks);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_tracklist.x"
#endif
  scm_c_export (
    "tracklist-insert-track", "tracklist-get-track-at-pos",
    "tracklist-get-num-tracks", NULL);
}

void
guile_audio_tracklist_define_module (void)
{
  scm_c_define_module ("audio tracklist", init_module, NULL);
}
