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

/**
 * \file
 *
 * Musical scale schema.
 */

#ifndef __SCHEMAS_AUDIO_SCALE_H__
#define __SCHEMAS_AUDIO_SCALE_H__

#include <stdint.h>

#include "schemas/audio/chord_descriptor.h"

typedef enum MusicalScaleType_v1
{
  SCALE_CHROMATIC_v1,
  SCALE_IONIAN_v1,
  SCALE_AEOLIAN_v1,
  SCALE_HARMONIC_MINOR_v1,
  SCALE_ACOUSTIC_v1,
  SCALE_ALGERIAN_v1,
  SCALE_ALTERED_v1,
  SCALE_AUGMENTED_v1,
  SCALE_BEBOP_DOMINANT_v1,
  SCALE_BLUES_v1,
  SCALE_CHINESE_v1,
  SCALE_DIMINISHED_v1,
  SCALE_DOMINANT_DIMINISHED_v1,
  SCALE_DORIAN_v1,
  SCALE_DOUBLE_HARMONIC_v1,
  SCALE_EIGHT_TONE_SPANISH_v1,
  SCALE_ENIGMATIC_v1,
  SCALE_EGYPTIAN_v1,
  SCALE_FLAMENCO_v1,
  SCALE_GEEZ_v1,
  SCALE_GYPSY_v1,
  SCALE_HALF_DIMINISHED_v1,
  SCALE_HARMONIC_MAJOR_v1,
  SCALE_HINDU_v1,
  SCALE_HIRAJOSHI_v1,
  SCALE_HUNGARIAN_GYPSY_v1,
  SCALE_HUNGARIAN_MINOR_v1,
  SCALE_IN_v1,
  SCALE_INSEN_v1,
  SCALE_ISTRIAN_v1,
  SCALE_IWATO_v1,
  SCALE_LOCRIAN_v1,
  SCALE_LYDIAN_AUGMENTED_v1,
  SCALE_LYDIAN_v1,
  SCALE_MAJOR_LOCRIAN_v1,
  SCALE_MAJOR_PENTATONIC_v1,
  SCALE_MAQAM_v1,
  SCALE_MELODIC_MINOR_v1,
  SCALE_MINOR_PENTATONIC_v1,
  SCALE_MIXOLYDIAN_v1,
  SCALE_NEAPOLITAN_MAJOR_v1,
  SCALE_NEAPOLITAN_MINOR_v1,
  SCALE_OCTATONIC_HALF_WHOLE_v1,
  SCALE_OCTATONIC_WHOLE_HALF_v1,
  SCALE_ORIENTAL_v1,
  SCALE_PERSIAN_v1,
  SCALE_PHRYGIAN_DOMINANT_v1,
  SCALE_PHRYGIAN_v1,
  SCALE_PROMETHEUS_v1,
  SCALE_ROMANIAN_MINOR_v1,
  SCALE_TRITONE_v1,
  SCALE_UKRANIAN_DORIAN_v1,
  SCALE_WHOLE_TONE_v1,
  SCALE_YO_v1,
} MusicalScaleType_v1;

static const cyaml_strval_t musical_scale_type_strings_v1[] = {
  {"Acoustic",   SCALE_ACOUSTIC_v1 },
  { "Aeolian",   SCALE_AEOLIAN_v1  },
  { "Algerian",  SCALE_ALGERIAN_v1 },
  { "Altered",   SCALE_ALTERED_v1  },
  { "Augmented", SCALE_AUGMENTED_v1},
};

typedef struct MusicalScale_v1
{
  int                  schema_version;
  MusicalScaleType_v1  type;
  MusicalNote_v1       root_key;
  int                  has_asc_desc;
  int                  notes[12];
  int                  notes_asc[12];
  int                  notes_desc[12];
  ChordDescriptor_v1 * default_chords[12];
  int                  num_notes;
} MusicalScale_v1;

static const cyaml_schema_field_t musical_scale_fields_schema_v1[] = {
  YAML_FIELD_INT (MusicalScale_v1, schema_version),
  YAML_FIELD_ENUM (
    MusicalScale_v1,
    type,
    musical_scale_type_strings_v1),
  CYAML_FIELD_ENUM (
    MusicalScale_v1,
    root_key,
    musical_note_strings_v1),
  CYAML_FIELD_SEQUENCE_FIXED (
    "notes",
    CYAML_FLAG_OPTIONAL,
    MusicalScale_v1,
    notes,
    &int_schema,
    12),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t musical_scale_schema_v1 = {
  YAML_VALUE_PTR (
    MusicalScale_v1,
    musical_scale_fields_schema_v1),
};

#endif
