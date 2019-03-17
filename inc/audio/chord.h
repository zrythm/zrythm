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
 * Chord related code.
 * See https://www.scales-chords.com
 * https://www.basicmusictheory.com/c-harmonic-minor-triad-chords
 */

#ifndef __AUDIO_CHORD_H__
#define __AUDIO_CHORD_H__

#include <stdint.h>

#include "audio/position.h"
#include "utils/yaml.h"

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
    "Db", \
    "D", \
    "Eb", \
    "E", \
    "F", \
    "F#", \
    "G", \
    "Ab", \
    "A", \
    "Bb", \
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
  CHORD_TYPE_AUG
} ChordType;

typedef struct _ChordWidget ChordWidget;

/**
 * Chords are to be generated on demand.
 */
typedef struct Chord
{
  int                   id;
  Position              pos; ///< chord object position (if used in chord track)
  int               has_bass; ///< has base note or not
  MusicalNote           root_note; ///< root note
  MusicalNote           bass_note; ///< bass note 1 octave below
  ChordType             type;
  int    notes[36]; ///< 3 octaves, 1st octave is for base note
                                  ///< starts at C always
  int                   inversion; ///< == 0 no inversion,
                                   ///< < 0 means highest note(s) drop an octave
                                   ///< > 0 means lowest note(s) receive an octave

  int                   selected; ///< selected in timeline
  int                   visible;
  ChordWidget *         widget;
} Chord;

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
  chord_fields_schema[] =
{
	CYAML_FIELD_INT (
    "id", CYAML_FLAG_DEFAULT,
    Chord, id),
  CYAML_FIELD_MAPPING (
    "pos", CYAML_FLAG_DEFAULT,
    Chord, pos, position_fields_schema),
	CYAML_FIELD_INT (
    "has_bass", CYAML_FLAG_DEFAULT,
    Chord, has_bass),
  CYAML_FIELD_ENUM (
    "root_note", CYAML_FLAG_DEFAULT,
    Chord, root_note, musical_note_strings,
    CYAML_ARRAY_LEN (musical_note_strings)),
  CYAML_FIELD_ENUM (
    "bass_note", CYAML_FLAG_DEFAULT,
    Chord, bass_note, musical_note_strings,
    CYAML_ARRAY_LEN (musical_note_strings)),
  CYAML_FIELD_SEQUENCE_FIXED (
    "notes", CYAML_FLAG_OPTIONAL,
    Chord, notes, &int_schema, 36),
	CYAML_FIELD_INT (
    "inversion", CYAML_FLAG_DEFAULT,
    Chord, inversion),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
chord_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			Chord, chord_fields_schema),
};

void
chord_init_loaded (Chord * self);

/**
 * Creates a chord.
 */
Chord *
chord_new (MusicalNote            root,
           uint8_t                has_bass,
           MusicalNote            bass,
           ChordType              type,
           int                    inversion);

/**
 * Returns the musical note as a string (eg. "C3").
 */
const char *
chord_note_to_string (MusicalNote note);

/**
 * Returns the chord in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
chord_as_string (Chord * chord);

void
chord_set_pos (Chord *    self,
               Position * pos);

void
chord_free (Chord * self);

#endif
