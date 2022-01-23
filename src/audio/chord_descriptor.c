/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include <string.h>

#include "audio/chord_descriptor.h"
#include "utils/midi.h"
#include "utils/objects.h"

/**
 * Updates the notes array based on the current
 * settings.
 */
void
chord_descriptor_update_notes (
  ChordDescriptor * self)
{
  if (self->type == CHORD_TYPE_CUSTOM)
    return;

  int root = self->root_note;
  int bass = self->bass_note;

  memset (self->notes, 0, sizeof (self->notes));

  /* add bass note */
  if (self->has_bass)
    {
      self->notes[bass] = 1;
    }

  /* add root note */
  self->notes[12 + root] = 1;

  /* add 2 more notes for triad */
  switch (self->type)
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

  unsigned int min_seventh_sems =
    self->type == CHORD_TYPE_DIM ? 9 : 10;

  /* add accents */
  switch (self->accent)
    {
    case CHORD_ACC_NONE:
      break;
    case CHORD_ACC_7:
      self->notes[12 + self->root_note + min_seventh_sems] = 1;
      break;
    case CHORD_ACC_j7:
      self->notes[12 + self->root_note + 11] = 1;
      break;
    case CHORD_ACC_b9:
      self->notes[12 + self->root_note + 13] = 1;
      break;
    case CHORD_ACC_9:
      self->notes[12 + self->root_note + 14] = 1;
      break;
    case CHORD_ACC_S9:
      self->notes[12 + self->root_note + 15] = 1;
      break;
    case CHORD_ACC_11:
      self->notes[12 + self->root_note + 17] = 1;
      break;
    case CHORD_ACC_b5_S11:
      self->notes[12 + self->root_note + 6] = 1;
      self->notes[12 + self->root_note + 18] = 1;
      break;
    case CHORD_ACC_S5_b13:
      self->notes[12 + self->root_note + 8] = 1;
      self->notes[12 + self->root_note + 16] = 1;
      break;
    case CHORD_ACC_6_13:
      self->notes[12 + self->root_note + 9] = 1;
      self->notes[12 + self->root_note + 21] = 1;
      break;
    default:
      g_warning ("chord unimplemented");
      break;
    }

  /* add the 7th to accents > 7 */
  if (self->accent >= CHORD_ACC_b9 &&
      self->accent <= CHORD_ACC_6_13)
    {
      self->notes[
        12 + self->root_note + min_seventh_sems] = 1;
    }

  /* TODO invert */
}

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
    object_new (ChordDescriptor);

  self->schema_version =
    CHORD_DESCRIPTOR_SCHEMA_VERSION;

  self->root_note = root;
  self->has_bass = has_bass;
  if (has_bass)
    self->bass_note = bass;
  self->type = type;
  self->accent = accent;
  self->inversion = inversion;

  chord_descriptor_update_notes (self);

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
  return midi_get_note_name ((midi_byte_t) note);
}

/**
 * Returns the chord type as a string (eg. "aug").
 */
const char *
chord_descriptor_chord_type_to_string (
  ChordType type)
{
  return chord_type_strings[type].str;
}

/**
 * Returns the chord accent as a string (eg. "j7").
 */
const char *
chord_descriptor_chord_accent_to_string (
  ChordAccent accent)
{
  return chord_accent_strings[accent].str;
}

/**
 * Returns if @ref key is the bass or root note of
 * @ref chord.
 *
 * @param key A note inside a single octave (0-11).
 */
bool
chord_descriptor_is_key_bass (
  ChordDescriptor * chord,
  MusicalNote       key)
{
  if (chord->has_bass)
    {
      return chord->bass_note == key;
    }
  else
    {
      return chord->root_note == key;
    }
}

/**
 * Returns if the given key is in the chord
 * represented by the given ChordDescriptor.
 *
 * @param key A note inside a single octave (0-11).
 */
bool
chord_descriptor_is_key_in_chord (
  ChordDescriptor * chord,
  MusicalNote       key)
{
  if (chord_descriptor_is_key_bass (chord, key))
    {
      return true;
    }

  for (int i = 0; i < 36; i++)
    {
      if (chord->notes[i] == 1 &&
          i % 12 == (int) key)
        return true;
    }
  return false;
}

/**
 * Returns the chord in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
chord_descriptor_to_new_string (
  const ChordDescriptor * chord)
{
  char tmp[100];
  chord_descriptor_to_string (
    chord, tmp);
  return g_strdup (tmp);
}

/**
 * Returns the chord in human readable string.
 */
void
chord_descriptor_to_string (
  const ChordDescriptor * chord,
  char *                  str)
{
  sprintf (
    str, "%s%s",
    chord_descriptor_note_to_string (
      chord->root_note),
    chord_descriptor_chord_type_to_string (
      chord->type));

  if (chord->accent > CHORD_ACC_NONE)
    {
      strcat (str, " ");
      strcat (
        str,
        chord_descriptor_chord_accent_to_string (
          chord->accent));
    }
  if (chord->has_bass &&
      (chord->bass_note != chord->root_note))
    {
      strcat (str, "/");
      strcat (
        str,
        chord_descriptor_note_to_string (
          chord->bass_note));
    }
}

void
chord_descriptor_free (ChordDescriptor * self)
{
  free (self);
}
