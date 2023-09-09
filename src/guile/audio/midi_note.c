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
