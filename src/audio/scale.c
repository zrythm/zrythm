/*
 * Copyright (C) 2018-2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Musical scales.
 *
 * See https://pianoscales.org/
 */

#include <stdlib.h>

#include "audio/chord_descriptor.h"
#include "audio/scale.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Creates new scale using type and root note.
 */
MusicalScale *
musical_scale_new (MusicalScaleType type, MusicalNote root)
{
  MusicalScale * self = object_new (MusicalScale);

  self->schema_version = SCALE_SCHEMA_VERSION;
  self->type = type;
  self->root_key = root;

  return self;
}

/**
 * Returns the triads in the given scale.
 *
 * There will be as many chords are enabled notes
 * in the scale, and the rest of the array will be
 * filled with CHORD_TYPE_NONE.
 *
 * @param ascending Whether to get the triads when
 *   ascending or descending (some scales have
 *   different triads when rising/falling).
 */
const ChordType *
musical_scale_get_triad_types (
  MusicalScaleType scale_type,
  bool             ascending)
{
#define SET_TRIADS( \
  n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12) \
  { \
    static const ChordType chord_types[] = { \
      CHORD_TYPE_##n1,  CHORD_TYPE_##n2,  CHORD_TYPE_##n3, \
      CHORD_TYPE_##n4,  CHORD_TYPE_##n5,  CHORD_TYPE_##n6, \
      CHORD_TYPE_##n7,  CHORD_TYPE_##n8,  CHORD_TYPE_##n9, \
      CHORD_TYPE_##n10, CHORD_TYPE_##n11, CHORD_TYPE_##n12 \
    }; \
    return chord_types; \
  }

#define SET_7_TRIADS(n1, n2, n3, n4, n5, n6, n7) \
  SET_TRIADS ( \
    n1, n2, n3, n4, n5, n6, n7, NONE, NONE, NONE, NONE, NONE)

  /* get the notes starting at C */
  switch (scale_type)
    {
    case SCALE_CHROMATIC:
      SET_7_TRIADS (NONE, NONE, NONE, NONE, NONE, NONE, NONE);
      break;
    case SCALE_MAJOR:
    case SCALE_IONIAN:
      SET_7_TRIADS (MAJ, MIN, MIN, MAJ, MAJ, MIN, DIM);
      break;
    case SCALE_DORIAN:
      SET_7_TRIADS (MIN, MIN, MAJ, MAJ, MIN, DIM, MAJ);
      break;
    case SCALE_PHRYGIAN:
      SET_7_TRIADS (MIN, MAJ, MAJ, MIN, DIM, MAJ, MIN);
      break;
    case SCALE_LYDIAN:
      SET_7_TRIADS (MAJ, MAJ, MIN, DIM, MAJ, MIN, MIN);
      break;
    case SCALE_MIXOLYDIAN:
      SET_7_TRIADS (MAJ, MIN, DIM, MAJ, MIN, MIN, MAJ);
      break;
    case SCALE_MINOR:
    case SCALE_AEOLIAN:
      SET_7_TRIADS (MIN, DIM, MAJ, MIN, MIN, MAJ, MAJ);
      break;
    case SCALE_LOCRIAN:
      SET_7_TRIADS (DIM, MAJ, MIN, MIN, MAJ, MAJ, MIN);
      break;
    case SCALE_MELODIC_MINOR:
      SET_7_TRIADS (MIN, MIN, AUG, MAJ, MAJ, DIM, DIM);
      break;
    case SCALE_HARMONIC_MINOR:
      SET_7_TRIADS (MIN, DIM, AUG, MIN, MAJ, MAJ, DIM);
      break;
    /* below need double check */
    case SCALE_HARMONIC_MAJOR:
      break;
    case SCALE_MAJOR_PENTATONIC:
      break;
    case SCALE_MINOR_PENTATONIC:
      break;
    case SCALE_PHRYGIAN_DOMINANT:
      break;
    case SCALE_MAJOR_LOCRIAN:
      break;
    case SCALE_ALGERIAN:
      break;
    case SCALE_AUGMENTED:
      break;
    case SCALE_DOUBLE_HARMONIC:
      break;
    case SCALE_CHINESE:
      break;
    case SCALE_DIMINISHED:
      break;
    case SCALE_DOMINANT_DIMINISHED:
      break;
    case SCALE_EGYPTIAN:
      break;
    case SCALE_EIGHT_TONE_SPANISH:
      break;
    case SCALE_ENIGMATIC:
      break;
    case SCALE_GEEZ:
      break;
    case SCALE_HINDU:
      break;
    case SCALE_HIRAJOSHI:
      break;
    case SCALE_HUNGARIAN_GYPSY:
      break;
    case SCALE_INSEN:
      break;
    case SCALE_NEAPOLITAN_MINOR:
      break;
    case SCALE_NEAPOLITAN_MAJOR:
      break;
    case SCALE_OCTATONIC_HALF_WHOLE:
      break;
    case SCALE_OCTATONIC_WHOLE_HALF:
      break;
    case SCALE_ORIENTAL:
      break;
    case SCALE_WHOLE_TONE:
      break;
    case SCALE_ROMANIAN_MINOR:
      break;
    case SCALE_ALTERED:
      break;
    case SCALE_MAQAM:
      break;
    case SCALE_YO:
      break;
    default:
      break;
    }

  /* return no triads for unimplemented scales */
  SET_7_TRIADS (NONE, NONE, NONE, NONE, NONE, NONE, NONE);

#undef SET_TRIADS
#undef SET_7_TRIADS

  return NULL;
}

/**
 * Returns the notes in the given scale.
 *
 * @param ascending Whether to get the notes when
 *   ascending or descending (some scales have
 *   different notes when rising/falling).
 */
const bool *
musical_scale_get_notes (
  MusicalScaleType scale_type,
  bool             ascending)
{
#define SET_NOTES( \
  n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12) \
  { \
    static const bool notes[] = { \
      n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12 \
    }; \
    return notes; \
  }

  /* get the notes starting at C */
  switch (scale_type)
    {
    case SCALE_CHROMATIC:
      SET_NOTES (1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
      break;
    case SCALE_MAJOR:
    case SCALE_IONIAN:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1);
      break;
    case SCALE_DORIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0);
      break;
    case SCALE_PHRYGIAN:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case SCALE_LYDIAN:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1);
      break;
    case SCALE_MIXOLYDIAN:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0);
      break;
    case SCALE_MINOR:
    case SCALE_AEOLIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case SCALE_LOCRIAN:
      SET_NOTES (1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0);
      break;
    case SCALE_MELODIC_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case SCALE_HARMONIC_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    /* below need double check */
    case SCALE_HARMONIC_MAJOR:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1);
      break;
    case SCALE_MAJOR_PENTATONIC:
      SET_NOTES (1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0);
      break;
    case SCALE_MINOR_PENTATONIC:
      SET_NOTES (1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case SCALE_PHRYGIAN_DOMINANT:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case SCALE_MAJOR_LOCRIAN:
      SET_NOTES (1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case SCALE_ACOUSTIC:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case SCALE_ALTERED:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case SCALE_ALGERIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1);
      break;
    case SCALE_AUGMENTED:
      SET_NOTES (1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1);
      break;
    case SCALE_DOUBLE_HARMONIC:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case SCALE_CHINESE:
      SET_NOTES (1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1);
      break;
    case SCALE_DIMINISHED:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case SCALE_DOMINANT_DIMINISHED:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case SCALE_EGYPTIAN:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case SCALE_EIGHT_TONE_SPANISH:
      SET_NOTES (1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case SCALE_ENIGMATIC:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1);
      break;
    case SCALE_GEEZ:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case SCALE_HINDU:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case SCALE_HIRAJOSHI:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case SCALE_HUNGARIAN_GYPSY:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case SCALE_INSEN:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case SCALE_NEAPOLITAN_MAJOR:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case SCALE_NEAPOLITAN_MINOR:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    case SCALE_OCTATONIC_HALF_WHOLE:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case SCALE_OCTATONIC_WHOLE_HALF:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case SCALE_ORIENTAL:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0);
      break;
    case SCALE_WHOLE_TONE:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case SCALE_ROMANIAN_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0);
      break;
    case SCALE_MAQAM:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case SCALE_YO:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0);
      break;
    case SCALE_BEBOP_LOCRIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1);
      break;
    case SCALE_BEBOP_DOMINANT:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1);
      break;
    case SCALE_BEBOP_MAJOR:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1);
      break;
    case SCALE_SUPER_LOCRIAN:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case SCALE_ENIGMATIC_MINOR:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1);
      break;
    case SCALE_COMPOSITE:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1);
      break;
    case SCALE_BHAIRAV:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case SCALE_HUNGARIAN_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case SCALE_PERSIAN:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1);
      break;
    case SCALE_IWATO:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case SCALE_KUMOI:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0);
      break;
    case SCALE_PELOG:
      SET_NOTES (1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0);
      break;
    case SCALE_PROMETHEUS:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case SCALE_PROMETHEUS_NEAPOLITAN:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case SCALE_PROMETHEUS_LISZT:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0);
      break;
    case SCALE_BALINESE:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0);
      break;
    case SCALE_RAGATODI:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0);
      break;
    case SCALE_JAPANESE1:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0);
      break;
    case SCALE_JAPANESE2:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0);
      break;
    default:
      break;
    }

#undef SET_NOTES

  return NULL;
}

/**
 * Clones the scale.
 */
MusicalScale *
musical_scale_clone (MusicalScale * src)
{
  /* TODO */
  return musical_scale_new (src->type, src->root_key);
}

/**
 * Returns if the given key is in the given
 * MusicalScale.
 *
 * @param note A note inside a single octave (0-11).
 */
bool
musical_scale_contains_note (
  const MusicalScale * scale,
  MusicalNote          note)
{
  if (scale->root_key == note)
    return 1;

  const bool * notes =
    musical_scale_get_notes (scale->type, false);

  return notes[((int) note - (int) scale->root_key + 12) % 12];
}

/**
 * Returns if all of the chord's notes are in
 * the scale.
 */
bool
musical_scale_contains_chord (
  const MusicalScale * const    scale,
  const ChordDescriptor * const chord)
{
  for (int i = 0; i < 48; i++)
    {
      if (
        chord->notes[i]
        && !musical_scale_contains_note (scale, i % 12))
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
  if (!musical_scale_contains_note (scale, chord_root))
    return 0;

  unsigned int min_seventh_sems =
    type == CHORD_TYPE_DIM ? 9 : 10;

  /* if 7th not in scale no need to check > 7th */
  if (
    chord_accent >= CHORD_ACC_b9
    && !musical_scale_contains_note (
      scale, (chord_root + min_seventh_sems) % 12))
    return 0;

  switch (chord_accent)
    {
    case CHORD_ACC_NONE:
      return 1;
    case CHORD_ACC_7:
      return musical_scale_contains_note (
        scale, (chord_root + min_seventh_sems) % 12);
    case CHORD_ACC_j7:
      return musical_scale_contains_note (
        scale, (chord_root + 11) % 12);
    case CHORD_ACC_b9:
      return musical_scale_contains_note (
        scale, (chord_root + 13) % 12);
    case CHORD_ACC_9:
      return musical_scale_contains_note (
        scale, (chord_root + 14) % 12);
    case CHORD_ACC_S9:
      return musical_scale_contains_note (
        scale, (chord_root + 15) % 12);
    case CHORD_ACC_11:
      return musical_scale_contains_note (
        scale, (chord_root + 17) % 12);
    case CHORD_ACC_b5_S11:
      return musical_scale_contains_note (
               scale, (chord_root + 6) % 12)
             && musical_scale_contains_note (
               scale, (chord_root + 18) % 12);
    case CHORD_ACC_S5_b13:
      return musical_scale_contains_note (
               scale, (chord_root + 8) % 12)
             && musical_scale_contains_note (
               scale, (chord_root + 16) % 12);
    case CHORD_ACC_6_13:
      return musical_scale_contains_note (
               scale, (chord_root + 9) % 12)
             && musical_scale_contains_note (
               scale, (chord_root + 21) % 12);
    default:
      return 0;
    }
}

const char *
musical_scale_type_to_string (const MusicalScaleType type)
{
  return _ (musical_scale_type_strings[type].str);
}

/**
 * Prints the MusicalScale to a string.
 *
 * MUST be free'd.
 */
char *
musical_scale_to_string (const MusicalScale * const self)
{
  return g_strdup_printf (
    "%s %s", chord_descriptor_note_to_string (self->root_key),
    musical_scale_type_to_string (self->type));
}

/**
 * Same as above but uses a buffer instead of
 * allocating.
 */
void
musical_scale_strcpy (MusicalScale * scale, char * buf)
{
#define SET_SCALE_STR(uppercase, str) \
  case SCALE_##uppercase: \
    sprintf ( \
      buf, "%s %s", \
      chord_descriptor_note_to_string (scale->root_key), \
      _ (str)); \
    return;

  switch (scale->type)
    {
      SET_SCALE_STR (CHROMATIC, "Chromatic");
      SET_SCALE_STR (IONIAN, "Ionian (Major)");
      SET_SCALE_STR (AEOLIAN, "Aeolian (Natural Minor)");
      SET_SCALE_STR (HARMONIC_MINOR, "Harmonic Minor");
    default:
      /* TODO */
      strcpy (buf, _ ("Unimplemented"));
    }
}

/**
 * Frees the MusicalScale.
 */
void
musical_scale_free (MusicalScale * scale)
{
  /* TODO */
}
