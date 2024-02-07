/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
  (SCM start_pos, SCM end_pos, SCM track_idx, SCM lane_idx, SCM idx_inside_lane),
  "Returns a new midi region.")
#define FUNC_NAME s_
{
  ZRegion * region = midi_region_new (
    scm_to_pointer (start_pos), scm_to_pointer (end_pos),
    scm_to_int (track_idx), scm_to_int (lane_idx), scm_to_int (idx_inside_lane));

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

  scm_c_export ("midi-region-new", "midi-region-add-midi-note", NULL);
}

void
guile_audio_midi_region_define_module (void)
{
  scm_c_define_module ("audio midi-region", init_module, NULL);
}
