/*
 * audio/scale.c - Scale
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Musical scales.
 *
 * See https://pianoscales.org/
 */

#include <stdlib.h>

#include "audio/chord.h"
#include "audio/scale.h"

/**
 * Creates new scale using type and root note.
 */
MusicalScale *
musical_scale_new (MusicalScaleType      type,
                   MusicalNote           root)
{
  MusicalScale * self = calloc (1, sizeof (MusicalScale));

  self->type = type;
  self->root_key = root;

  /* add basic triads for each scale */
  int * notes = self->notes;
  Chord ** chords = self->default_chords;
  MusicalNote note;
  switch (self->type)
    {
    case SCALE_AEOLIAN: // natural minor
      self->has_asc_desc = 0;
      *notes++ = 1; // C
      *notes++ = 0; // C#
      *notes++ = 1; // D
      *notes++ = 1; // D#
      *notes++ = 0; // E
      *notes++ = 1; // F
      *notes++ = 0; // F#
      *notes++ = 1; // G
      *notes++ = 1; // G#
      *notes++ = 0; // A
      *notes++ = 1; // A#
      *notes++ = 0; // B
      self->num_notes = 7;

      /* Cm */
      note = root;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* Ddim */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
                            0); // inversion

      /* D# */
      note += 1;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* Fm */
      note += 1;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* Gm */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* G# */
      note += 1;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* A# */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      break;
    case SCALE_IONIAN: // major
      self->has_asc_desc = 0;
      *notes++ = 1; // C
      *notes++ = 0; // C#
      *notes++ = 1; // D
      *notes++ = 0; // D#
      *notes++ = 1; // E
      *notes++ = 1; // F
      *notes++ = 0; // F#
      *notes++ = 1; // G
      *notes++ = 0; // G#
      *notes++ = 1; // A
      *notes++ = 0; // A#
      *notes++ = 1; // B
      self->num_notes = 7;

      /* C */
      note = root;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* Dm */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* Em */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* F */
      note += 1;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* G */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* Am */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* Am */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
                            0); // inversion
      break;
    case SCALE_HARMONIC_MINOR:
      self->has_asc_desc = 0;
      *notes++ = 1; // C
      *notes++ = 0; // C#
      *notes++ = 1; // D
      *notes++ = 1; // D#
      *notes++ = 0; // E
      *notes++ = 1; // F
      *notes++ = 0; // F#
      *notes++ = 1; // G
      *notes++ = 1; // G#
      *notes++ = 0; // A
      *notes++ = 0; // A#
      *notes++ = 1; // B
      self->num_notes = 7;

      /* Cm */
      note = root;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* Ddim */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
                            0); // inversion

      /* D# aug */
      note += 1;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_AUG, // type
                            0); // inversion

      /* Fm */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* G */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* G# */
      note += 1;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* Bdim */
      note += 3;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
                            0); // inversion

      break;
    case SCALE_MELODIC_MINOR:
      self->has_asc_desc = 0;
      *notes++ = 1; // C
      *notes++ = 0; // C#
      *notes++ = 1; // D
      *notes++ = 1; // D#
      *notes++ = 0; // E
      *notes++ = 1; // F
      *notes++ = 0; // F#
      *notes++ = 1; // G
      *notes++ = 0; // G#
      *notes++ = 1; // A
      *notes++ = 0; // A#
      *notes++ = 1; // B
      self->num_notes = 7;

      /* Cm */
      note = root;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* Dm */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
                            0); // inversion

      /* D# aug */
      note += 1;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_AUG, // type
                            0); // inversion

      /* F */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* G */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
                            0); // inversion

      /* Adim */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
                            0); // inversion

      /* Bdim */
      note += 2;
      *chords++ = chord_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
                            0); // inversion

      break;
    }

  return self;
}
