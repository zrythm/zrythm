/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/chord_descriptor.h"
#include "audio/scale.h"

#include <glib/gi18n.h>

/**
 * Creates new scale using type and root note.
 */
MusicalScale *
musical_scale_new (
  MusicalScaleType      type,
  MusicalNote           root)
{
  MusicalScale * self =
    calloc (1, sizeof (MusicalScale));

  self->type = type;
  self->root_key = root;

  /* add basic triads for each scale */
  int * notes = self->notes;
  ChordDescriptor ** chords = self->default_chords;
  MusicalNote note;
  switch (self->type)
    {
    case SCALE_CHROMATIC:
      self->has_asc_desc = 0;
      *notes++ = 1; // C
      *notes++ = 1; // C#
      *notes++ = 1; // D
      *notes++ = 1; // D#
      *notes++ = 1; // E
      *notes++ = 1; // F
      *notes++ = 1; // F#
      *notes++ = 1; // G
      *notes++ = 1; // G#
      *notes++ = 1; // A
      *notes++ = 1; // A#
      *notes++ = 1; // B
      self->num_notes = 12;
      break;
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
      *chords++ =
        chord_descriptor_new (
          note, // root
          1, // has bass
          note, // bass note
          CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
          0); // inversion

      /* Ddim */
      note += 2;
      *chords++ =
        chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* D# */
      note += 1;
      *chords++ =
        chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Fm */
      note += 1;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Gm */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* G# */
      note += 1;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* A# */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
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
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Dm */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Em */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* F */
      note += 1;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* G */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Am */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Am */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
          CHORD_ACC_NONE, // accent
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
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Ddim */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* D# aug */
      note += 1;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_AUG, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Fm */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* G */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* G# */
      note += 1;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Bdim */
      note += 3;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
          CHORD_ACC_NONE, // accent
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
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Dm */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MIN, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* D# aug */
      note += 1;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_AUG, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* F */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* G */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_MAJ, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Adim */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      /* Bdim */
      note += 2;
      *chords++ = chord_descriptor_new (note, // root
                            1, // has bass
                            note, // bass note
                            CHORD_TYPE_DIM, // type
          CHORD_ACC_NONE, // accent
                            0); // inversion

      break;
    default:
      g_warning ("scale unimplemented");
      break;
    }

  return self;
}

/**
 * Clones the scale.
 */
MusicalScale *
musical_scale_clone (
  MusicalScale * src)
{
  /* TODO */
  return musical_scale_new (
    src->type, src->root_key);
}

/**
 * Returns if the given key is in the given
 * MusicalScale.
 *
 * @param key A note inside a single octave (0-11).
 */
int
musical_scale_is_key_in_scale (
  MusicalScale * scale,
  MusicalNote    key)
{
  if (scale->root_key  == key)
    return 1;

  for (int i = 0; i < 12; i++)
    {
      /* check given key - scale root key, add
       * 12 to make sure the value is positive, and
       * mod 12 to stay in 1 octave */
      if (scale->notes[i] == 1 &&
          i == ((key - scale->root_key) + 12) % 12)
        return 1;
    }
  return 0;
}

/**
 * Returns if all of the chord's notes are in
 * the scale.
 */
int
musical_scale_is_chord_in_scale (
  MusicalScale * scale,
  ChordDescriptor * chord)
{
  for (int i = 0; i < 48; i++)
    {
      if (chord->notes[i] &&
          !musical_scale_is_key_in_scale (
            scale, i % 12))
        return 0;
    }
  return 1;
}

/**
 * Returns if the accent is in the scale.
 */
int
musical_scale_is_accent_in_scale (
  MusicalScale * scale,
  MusicalNote    chord_root,
  ChordType      type,
  ChordAccent    chord_accent)
{
  if (!musical_scale_is_key_in_scale (
        scale, chord_root))
    return 0;

  int min_seventh_sems =
    type == CHORD_TYPE_DIM ? 9 : 10;

  /* if 7th not in scale no need to check > 7th */
  if (chord_accent >= CHORD_ACC_b9 &&
      !musical_scale_is_key_in_scale (
        scale, (chord_root + min_seventh_sems) % 12))
    return 0;

  switch (chord_accent)
    {
    case CHORD_ACC_NONE:
      return 1;
    case CHORD_ACC_7:
      return
        musical_scale_is_key_in_scale (
          scale,
          (chord_root + min_seventh_sems) % 12);
    case CHORD_ACC_j7:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 11) % 12);
    case CHORD_ACC_b9:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 13) % 12);
    case CHORD_ACC_9:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 14) % 12);
    case CHORD_ACC_S9:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 15) % 12);
    case CHORD_ACC_11:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 17) % 12);
    case CHORD_ACC_b5_S11:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 6) % 12) &&
        musical_scale_is_key_in_scale (
          scale, (chord_root + 18) % 12);
    case CHORD_ACC_S5_b13:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 8) % 12) &&
        musical_scale_is_key_in_scale (
          scale, (chord_root + 16) % 12);
    case CHORD_ACC_6_13:
      return
        musical_scale_is_key_in_scale (
          scale, (chord_root + 9) % 12) &&
        musical_scale_is_key_in_scale (
          scale, (chord_root + 21) % 12);
    default:
      return 0;
    }
}

/**
 * Prints the MusicalScale to a string.
 *
 * MUST be free'd.
 */
char *
musical_scale_to_string (
  MusicalScale * scale)
{
#define RETURN_SCALE_STR(uppercase,str) \
  case SCALE_##uppercase: \
    return g_strdup_printf ( \
      "%s %s", \
      chord_descriptor_note_to_string ( \
        scale->root_key), \
      _(str))

  switch (scale->type)
    {
      RETURN_SCALE_STR (
        CHROMATIC, "Chromatic");
      RETURN_SCALE_STR (
        IONIAN, "Ionian (Major)");
      RETURN_SCALE_STR (
        AEOLIAN, "Aeolian (Natural Minor)");
      RETURN_SCALE_STR (
        HARMONIC_MINOR, "Harmonic Minor");
    default:
      /* TODO */
      return g_strdup (_("Unimplemented"));
    }
}

/**
 * Frees the MusicalScale.
 */
void
musical_scale_free (
  MusicalScale * scale)
{
  /* TODO */
}
