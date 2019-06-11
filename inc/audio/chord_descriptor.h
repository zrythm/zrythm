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
 * Descriptors for chords.
 */

#ifndef __AUDIO_CHORD_DESCRIPTOR_H__
#define __AUDIO_CHORD_DESCRIPTOR_H__

#include <stdint.h>

#include "audio/position.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum MusicalNote
{
  NOTE_C,
  NOTE_CS,
  NOTE_D,
  NOTE_DS,
  NOTE_E,
  NOTE_F,
  NOTE_FS,
  NOTE_G,
  NOTE_GS,
  NOTE_A,
  NOTE_AS,
  NOTE_B
} MusicalNote;

#define NOTE_LABELS \
static const char * note_labels[12] = { \
  "C", \
  "D\u266D", \
  "D", \
  "E\u266D", \
  "E", \
  "F", \
  "F\u266F", \
  "G", \
  "A\u266D", \
  "A", \
  "B\u266D", \
  "B" }

/**
 * Chord type.
 */
typedef enum ChordType
{
  CHORD_TYPE_MAJ,
  CHORD_TYPE_MIN,
  CHORD_TYPE_7,
  CHORD_TYPE_MIN_7,
  CHORD_TYPE_MAJ_7,
  CHORD_TYPE_6,
  CHORD_TYPE_MIN_6,
  CHORD_TYPE_6_9,
  CHORD_TYPE_5,
  CHORD_TYPE_9,
  CHORD_TYPE_MIN_9,
  CHORD_TYPE_MAJ_9,
  CHORD_TYPE_11,
  CHORD_TYPE_MIN_11,
  CHORD_TYPE_13,
  CHORD_TYPE_ADD_2,
  CHORD_TYPE_ADD_9,
  CHORD_TYPE_7_MINUS_5,
  CHORD_TYPE_7_PLUS_5,
  CHORD_TYPE_SUS2,
  CHORD_TYPE_SUS4,
  CHORD_TYPE_DIM,
  CHORD_TYPE_AUG,
  NUM_CHORD_TYPES,
} ChordType;

#define CHORD_TYPES \
static const char * chord_type_labels[NUM_CHORD_TYPES] = { \
  "Maj", \
  "min", \
  "7", \
  "min7", \
  "Maj7", \
  "CHORD_TYPE_6", \
  "CHORD_TYPE_MIN_6", \
  "CHORD_TYPE_6_9", \
  "CHORD_TYPE_5", \
  "9", \
  "CHORD_TYPE_MIN_9", \
  "CHORD_TYPE_MAJ_9", \
  "11", \
  "min11", \
  "CHORD_TYPE_13", \
  "CHORD_TYPE_ADD_2", \
  "CHORD_TYPE_ADD_9", \
  "CHORD_TYPE_7_MINUS_5", \
  "CHORD_TYPE_7_PLUS_5", \
  "sus2", \
  "sus4", \
  "dim", \
  "aug" }

/**
 * A ChordDescriptor describes a chord and is not
 * linked to any specific object by itself.
 *
 * Chord objects should include a ChordDescriptor.
 */
typedef struct ChordDescriptor
{
  /** Has bass note or not. */
  int            has_bass;

  /** Root note. */
  MusicalNote    root_note;

  /** Bass note 1 octave below. */
  MusicalNote    bass_note;

  ChordType      type;

  /**
   * 3 octaves, 1st octave is for bass note.
   *
   * Starts at C always.
   */
  int            notes[36];

  /**
   * 0 no inversion,
   * less than 0 highest note(s) drop an octave,
   * greater than 0 lwest note(s) receive an octave.
   */
  int                   inversion;
} ChordDescriptor;

static const cyaml_strval_t musical_note_strings[] = {
	{ "C",          NOTE_C    },
	{ "C#",         NOTE_CS   },
	{ "D",          NOTE_D   },
	{ "D#",         NOTE_DS   },
	{ "E",          NOTE_E   },
	{ "F",          NOTE_F   },
	{ "F#",         NOTE_FS   },
	{ "G",          NOTE_G   },
	{ "G#",         NOTE_GS   },
	{ "A",          NOTE_A   },
	{ "A#",         NOTE_AS   },
	{ "B",          NOTE_B   },
};

static const cyaml_schema_field_t
  chord_descriptor_fields_schema[] =
{
	CYAML_FIELD_INT (
    "has_bass", CYAML_FLAG_DEFAULT,
    ChordDescriptor, has_bass),
  CYAML_FIELD_ENUM (
    "root_note", CYAML_FLAG_DEFAULT,
    ChordDescriptor, root_note, musical_note_strings,
    CYAML_ARRAY_LEN (musical_note_strings)),
  CYAML_FIELD_ENUM (
    "bass_note", CYAML_FLAG_DEFAULT,
    ChordDescriptor, bass_note, musical_note_strings,
    CYAML_ARRAY_LEN (musical_note_strings)),
  CYAML_FIELD_SEQUENCE_FIXED (
    "notes", CYAML_FLAG_OPTIONAL,
    ChordDescriptor, notes, &int_schema, 36),
	CYAML_FIELD_INT (
    "inversion", CYAML_FLAG_DEFAULT,
    ChordDescriptor, inversion),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
chord_descriptor_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordDescriptor, chord_descriptor_fields_schema),
};

/**
 * Creates a ChordDescriptor.
 */
ChordDescriptor *
chord_descriptor_new (
  MusicalNote            root,
  uint8_t                has_bass,
  MusicalNote            bass,
  ChordType              type,
  int                    inversion);

static inline int
chord_descriptor_are_notes_equal (
  int * notes_a,
  int * notes_b)
{
  /* 36 notes in Chord */
  for (int i = 0; i < 36; i++)
    {
      if (notes_a[i] != notes_b[i])
        return 0;
    }
  return 1;
}

static inline int
chord_descriptor_is_equal (
  ChordDescriptor * a,
  ChordDescriptor * b)
{
  return
    a->has_bass == b->has_bass &&
    a->root_note == b->root_note &&
    a->bass_note == b->bass_note &&
    a->type == b->type &&
    chord_descriptor_are_notes_equal (
      a->notes, b->notes) &&
    a->inversion == b->inversion;
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
  MusicalNote       key);

/**
 * Clones the given ChordDescriptor.
 */
ChordDescriptor *
chord_descriptor_clone (
  ChordDescriptor * src);

/**
 * Returns the chord type as a string (eg. "aug").
 */
const char *
chord_descriptor_chord_type_to_string (
  ChordType type);

/**
 * Returns the musical note as a string (eg. "C3").
 */
const char *
chord_descriptor_note_to_string (
  MusicalNote note);

/**
 * Returns the chord in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
chord_descriptor_to_string (
  ChordDescriptor * chord);

/**
 * Frees the ChordDescriptor.
 */
void
chord_descriptor_free (
  ChordDescriptor * self);

/**
 * @}
 */

#endif
