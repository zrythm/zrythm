/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "dsp/midi_note.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_midi_note_new,
  "midi-note-new",
  5,
  0,
  0,
  (SCM region, SCM start_pos, SCM end_pos, SCM pitch, SCM velocity),
  "Returns a new midi note.")
#define FUNC_NAME s_
{
  ZRegion *  midi_region = scm_to_pointer (region);
  MidiNote * mn = midi_note_new (
    &midi_region->id, scm_to_pointer (start_pos), scm_to_pointer (end_pos),
    scm_to_uint8 (pitch), scm_to_uint8 (velocity));

  return scm_from_pointer (mn, NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_midi_note.x"
#endif

  scm_c_export ("midi-note-new", NULL);
}

void
guile_audio_midi_note_define_module (void)
{
  scm_c_define_module ("audio midi-note", init_module, NULL);
}
