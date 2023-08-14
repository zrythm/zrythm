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
#  include "dsp/track_processor.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_track_processor_get_stereo_in,
  "track-processor-get-stereo-in",
  1,
  0,
  0,
  (SCM track_processor),
  "Returns the stereo in ports instance of the track processor.")
#define FUNC_NAME s_
{
  TrackProcessor * tp =
    (TrackProcessor *) scm_to_pointer (track_processor);

  return scm_from_pointer (tp->stereo_in, NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_track_processor.x"
#endif

  scm_c_export ("track-processor-get-stereo-in", NULL);
}

void
guile_audio_track_processor_define_module (void)
{
  scm_c_define_module (
    "audio track-processor", init_module, NULL);
}
