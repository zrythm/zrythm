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
#  include "dsp/midi_region.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_midi_region_new,
  "midi-region-new",
  5,
  0,
  0,
  (SCM start_pos,
   SCM end_pos,
   SCM track_idx,
   SCM lane_idx,
   SCM idx_inside_lane),
  "Returns a new midi region.")
#define FUNC_NAME s_
{
  ZRegion * region = midi_region_new (
    scm_to_pointer (start_pos), scm_to_pointer (end_pos),
    scm_to_int (track_idx), scm_to_int (lane_idx),
    scm_to_int (idx_inside_lane));

  return scm_from_pointer (region, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_add_note,
  "midi-region-add-midi-note",
  2,
  0,
  0,
  (SCM region, SCM midi_note),
  "Adds a midi note to @var{region}.")
#define FUNC_NAME s_
{
  midi_region_add_midi_note (
    scm_to_pointer (region), scm_to_pointer (midi_note), true);

  return SCM_BOOL_T;
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_midi_region.x"
#endif

  scm_c_export (
    "midi-region-new", "midi-region-add-midi-note", NULL);
}

void
guile_audio_midi_region_define_module (void)
{
  scm_c_define_module ("audio midi-region", init_module, NULL);
}
