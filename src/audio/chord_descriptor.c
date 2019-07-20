/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "audio/chord_descriptor.h"

/* see chord_desriptor.h */
NOTE_LABELS;
CHORD_TYPES;
CHORD_ACCENTS;

/**
 * Creates a ChordDescriptor.
 */
ChordDescriptor *
chord_descriptor_new (
  MusicalNote            root,
  int                    has_bass,
  MusicalNote            bass,
  ChordType              type,
  ChordAccent            accent,
  int                    inversion)
{
  ChordDescriptor * self =
    calloc (1, sizeof (ChordDescriptor));

  self->root_note = root;
  self->has_bass = has_bass;
  if (has_bass)
    self->bass_note = bass;
  self->type = type;
  self->accent = accent;
  self->inversion = inversion;

  /* add bass note */
  if (has_bass)
    {
      self->notes[bass] = 1;
    }

  /* add root note */
  self->notes[12 + root] = 1;

  /* add 2 more notes for triad */
  switch (type)
    {
    case CHORD_TYPE_MAJ:
      self->notes[12 + root + 4] = 1;
      self->notes[12 + root + 4 + 3] = 1;
      break;
    case CHORD_TYPE_MIN:
      self->notes[12 + root + 3] = 1;
      self->notes[12 + root + 3 + 4] = 1;
      break;
    case CHORD_TYPE_DIM:
      self->notes[12 + root + 3] = 1;
      self->notes[12 + root + 3 + 3] = 1;
      break;
    case CHORD_TYPE_AUG:
      self->notes[12 + root + 4] = 1;
      self->notes[12 + root + 4 + 4] = 1;
      break;
    case CHORD_TYPE_SUS2:
      self->notes[12 + root + 2] = 1;
      self->notes[12 + root + 2 + 5] = 1;
      break;
    case CHORD_TYPE_SUS4:
      self->notes[12 + root + 5] = 1;
      self->notes[12 + root + 5 + 2] = 1;
      break;
    default:
      g_warning ("chord unimplemented");
      break;
    }

  int min_seventh_sems =
    type == CHORD_TYPE_DIM ? 9 : 10;

  /* add accents */
  switch (accent)
    {
    case CHORD_ACC_NONE:
      break;
    case CHORD_ACC_7:
      self->notes[12 + root + min_seventh_sems] = 1;
      break;
    case CHORD_ACC_j7:
      self->notes[12 + root + 11] = 1;
      break;
    case CHORD_ACC_b9:
      self->notes[12 + root + 13] = 1;
      break;
    case CHORD_ACC_9:
      self->notes[12 + root + 14] = 1;
      break;
    case CHORD_ACC_S9:
      self->notes[12 + root + 15] = 1;
      break;
    case CHORD_ACC_11:
      self->notes[12 + root + 17] = 1;
      break;
    case CHORD_ACC_b5_S11:
      self->notes[12 + root + 6] = 1;
      self->notes[12 + root + 18] = 1;
      break;
    case CHORD_ACC_S5_b13:
      self->notes[12 + root + 8] = 1;
      self->notes[12 + root + 16] = 1;
      break;
    case CHORD_ACC_6_13:
      self->notes[12 + root + 9] = 1;
      self->notes[12 + root + 21] = 1;
      break;
    default:
      g_warning ("chord unimplemented");
      break;
    }

  /* add the 7th to accents > 7 */
  if (accent >= CHORD_ACC_b9 &&
      accent <= CHORD_ACC_6_13)
    self->notes[12 + root + min_seventh_sems] = 1;

  /* TODO invert */

  return self;
}

/**
 * Clones the given ChordDescriptor.
 */
ChordDescriptor *
chord_descriptor_clone (
  ChordDescriptor * src)
{
  ChordDescriptor * cd =
    chord_descriptor_new (
      src->root_note, src->has_bass, src->bass_note,
      src->type, src->accent, src->inversion);

  return cd;
}

/**
 * Returns the musical note as a string (eg. "C3").
 */
const char *
chord_descriptor_note_to_string (
MusicalNote note)
{
  (void) note_labels;
  return note_labels[note];
}

/**
 * Returns the chord type as a string (eg. "aug").
 */
const char *
chord_descriptor_chord_type_to_string (
  ChordType type)
{
  (void) chord_type_labels;
  return chord_type_labels[type];
}

/**
 * Returns the chord accent as a string (eg. "j7").
 */
const char *
chord_descriptor_chord_accent_to_string (
  ChordAccent accent)
{
  (void) chord_accent_labels;
  return chord_accent_labels[accent];
}

/**
 * Returns if the given key is in the chord
 * represented by the given ChordDescriptor.
 *
 * @param key A note inside a single octave (0-11).
 */
int
chord_descriptor_is_key_in_chord (
  ChordDescriptor * chord,
  MusicalNote       key)
{
  if ((chord->has_bass &&
      chord->bass_note == key) ||
      chord->root_note == key)
    return 1;

  for (int i = 0; i < 36; i++)
    {
      if (chord->notes[i] == 1 &&
          i % 12 == key)
        return 1;
    }
  return 0;
}

/**
 * Returns the chord in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
chord_descriptor_to_string (
  ChordDescriptor * chord)
{
  char * str = g_strdup_printf (
    "%s%s",
    chord_descriptor_note_to_string (
      chord->root_note),
    chord_descriptor_chord_type_to_string (
      chord->type));

  if (chord->accent > CHORD_ACC_NONE)
    {
      char * str2 =
        g_strdup_printf (
          "%s %s",
          str,
          chord_descriptor_chord_accent_to_string (
            chord->accent));
      g_free (str);
      str = str2;
    }
  if (chord->has_bass &&
      (chord->bass_note != chord->root_note))
    {
      char * str2 =
        g_strdup_printf (
          "%s/%s",
          str,
          chord_descriptor_note_to_string (
            chord->bass_note));
      g_free (str);
      str = str2;
    }

  return str;
}

void
chord_descriptor_free (ChordDescriptor * self)
{
  free (self);
}
