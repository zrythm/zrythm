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
