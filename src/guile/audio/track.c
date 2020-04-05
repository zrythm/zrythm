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
#include "audio/track.h"
#include "project.h"
#endif

#include "guile/modules.h"

SCM_DEFINE (
  get_name, "track-get-name", 1, 0, 0,
  (SCM track),
  "Returns the name of @var{track}.")
{
  scm_assert_foreign_object_type (
    track_type, track);
  Track * reftrack =
    scm_foreign_object_ref (track, 0);

  return
    scm_from_utf8_string (
      track_get_name (reftrack));
}

static void
init_module (void * data)
{
  init_guile_object_type (
    &track_type, "track");

#if !defined(SCM_MAGIC_SNARF_DOCS) && \
  !defined(SCM_MAGIC_SNARFER)
#include "audio_track.x"
#endif

  scm_c_export ("track-get-name", NULL);
}

void
guile_audio_track_define_module (void)
{
  scm_c_define_module (
    "audio track", init_module, NULL);
}
