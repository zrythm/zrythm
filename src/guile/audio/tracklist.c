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
#include "audio/tracklist.h"
#include "project.h"
#include "zrythm_app.h"
#endif

SCM_DEFINE (
  s_tracklist_insert_track,
  "tracklist-insert-track", 2, 0, 0,
  (SCM track, SCM idx),
  "Inserts track @var{track} at index @var{idx} in "
  "the tracklist.")
{
  tracklist_insert_track (
    TRACKLIST, scm_to_pointer (track),
    scm_to_int (idx), true, true);

  return SCM_BOOL_T;
}

SCM_DEFINE (
  get_track_at_pos, "tracklist-get-track-at-pos",
  1, 0, 0,
  (SCM pos),
  "Returns the track at @var{pos} in the tracklist.")
{
  return
    scm_from_pointer (
      TRACKLIST->tracks[scm_to_int (pos)], NULL);
}

SCM_DEFINE (
  get_num_tracks, "tracklist-get-num-tracks", 0, 0, 0,
  (),
  "Returns the number of tracks in the tracklist.")
{
  return
    scm_from_int (TRACKLIST->num_tracks);
}

SCM_DEFINE (
  get_tracklist, "tracklist-get", 0, 0, 0,
  (),
  "Returns the tracklist for the current project.")
{
  return
    scm_from_pointer (TRACKLIST, NULL);
}

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#include "audio_tracklist.x"
#endif
  scm_c_export (
    "tracklist-insert-track",
    "tracklist-get-track-at-pos",
    "tracklist-get-num-tracks",
    "tracklist-get",
    NULL);
}

void
guile_audio_tracklist_define_module (void)
{
  scm_c_define_module (
    "audio tracklist", init_module, NULL);
}
