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

#if !defined(SCM_MAGIC_SNARF_DOCS) && \
  !defined(SCM_MAGIC_SNARFER)
#include "audio/tracklist.h"
#include "guile/modules.h"
#include "project.h"
#endif

#include <libguile.h>

SCM_DEFINE (
  get_track_at_pos, "tracklist-get-track-at-pos",
  1, 0, 0,
  (SCM pos),
  "Returns the track at @var{pos} in the tracklist.")
#define FUNC_NAME s_
{
  return
    scm_make_foreign_object_1 (
      track_type,
      TRACKLIST->tracks[scm_to_int (pos)]);
}
#undef FUNC_NAME

SCM_DEFINE (
  get_num_tracks, "tracklist-get-num-tracks", 0, 0, 0,
  (),
  "Returns the number of tracks in the tracklist.")
#define FUNC_NAME s_
{
  return
    scm_from_int (TRACKLIST->num_tracks);
}
#undef FUNC_NAME

SCM_DEFINE (
  get_tracklist, "tracklist-get", 0, 0, 0,
  (),
  "Returns the tracklist for the current project.")
#define FUNC_NAME s_
{
  return
    scm_make_foreign_object_1 (
      tracklist_type, TRACKLIST);
}
#undef FUNC_NAME

void
init_module (void * data)
{
  init_guile_object_type (
    &tracklist_type, "tracklist");

  scm_c_define_gsubr (
    "tracklist-get-track-at-pos", 1, 0, 0,
    get_track_at_pos);
  scm_c_define_gsubr (
    "tracklist-get-num-tracks", 0, 0, 0,
    get_num_tracks);
  scm_c_define_gsubr (
    "tracklist-get", 0, 0, 0, get_tracklist);
  scm_c_export (
    "tracklist-get-track-at-pos",
    "tracklist-get-num-tracks",
    "tracklist-get", NULL);
}

void
guile_audio_tracklist_define_module (void)
{
  scm_c_define_module (
    "audio tracklist", init_module, NULL);
}
