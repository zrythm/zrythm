// clang-format off
// SPDX-FileCopyrightText: © 2018-2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Musical scales.
 *
 * See https://pianoscales.org/
 */

#include <stdlib.h>

#include "dsp/chord_descriptor.h"
#include "dsp/scale.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Creates new scale using type and root note.
 */
MusicalScale *
musical_scale_new (MusicalScaleType type, MusicalNote root)
{
  MusicalScale * self = object_new (MusicalScale);

  self->type = type;
  self->root_key = root;

  return self;
}

/**
 * Returns the triads in the given scale.
 *
 * There will be as many chords are enabled notes
 * in the scale, and the rest of the array will be
 * filled with ChordType::CHORD_TYPE_NONE.
 *
 * @param ascending Whether to get the triads when
 *   ascending or descending (some scales have
 *   different triads when rising/falling).
 */
const ChordType *
musical_scale_get_triad_types (MusicalScaleType scale_type, bool ascending)
{
#define SET_TRIADS(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12) \
  { \
    static const ChordType chord_types[] = { \
      ChordType::CHORD_TYPE_##n1,  ChordType::CHORD_TYPE_##n2, \
      ChordType::CHORD_TYPE_##n3,  ChordType::CHORD_TYPE_##n4, \
      ChordType::CHORD_TYPE_##n5,  ChordType::CHORD_TYPE_##n6, \
      ChordType::CHORD_TYPE_##n7,  ChordType::CHORD_TYPE_##n8, \
      ChordType::CHORD_TYPE_##n9,  ChordType::CHORD_TYPE_##n10, \
      ChordType::CHORD_TYPE_##n11, ChordType::CHORD_TYPE_##n12 \
    }; \
    return chord_types; \
  }

#define SET_7_TRIADS(n1, n2, n3, n4, n5, n6, n7) \
  SET_TRIADS (n1, n2, n3, n4, n5, n6, n7, NONE, NONE, NONE, NONE, NONE)

  /* get the notes starting at C */
  switch (scale_type)
    {
    case MusicalScaleType::SCALE_CHROMATIC:
      SET_7_TRIADS (NONE, NONE, NONE, NONE, NONE, NONE, NONE);
      break;
    case MusicalScaleType::SCALE_MAJOR:
    case MusicalScaleType::SCALE_IONIAN:
      SET_7_TRIADS (MAJ, MIN, MIN, MAJ, MAJ, MIN, DIM);
      break;
    case MusicalScaleType::SCALE_DORIAN:
      SET_7_TRIADS (MIN, MIN, MAJ, MAJ, MIN, DIM, MAJ);
      break;
    case MusicalScaleType::SCALE_PHRYGIAN:
      SET_7_TRIADS (MIN, MAJ, MAJ, MIN, DIM, MAJ, MIN);
      break;
    case MusicalScaleType::SCALE_LYDIAN:
      SET_7_TRIADS (MAJ, MAJ, MIN, DIM, MAJ, MIN, MIN);
      break;
    case MusicalScaleType::SCALE_MIXOLYDIAN:
      SET_7_TRIADS (MAJ, MIN, DIM, MAJ, MIN, MIN, MAJ);
      break;
    case MusicalScaleType::SCALE_MINOR:
    case MusicalScaleType::SCALE_AEOLIAN:
      SET_7_TRIADS (MIN, DIM, MAJ, MIN, MIN, MAJ, MAJ);
      break;
    case MusicalScaleType::SCALE_LOCRIAN:
      SET_7_TRIADS (DIM, MAJ, MIN, MIN, MAJ, MAJ, MIN);
      break;
    case MusicalScaleType::SCALE_MELODIC_MINOR:
      SET_7_TRIADS (MIN, MIN, AUG, MAJ, MAJ, DIM, DIM);
      break;
    case MusicalScaleType::SCALE_HARMONIC_MINOR:
      SET_7_TRIADS (MIN, DIM, AUG, MIN, MAJ, MAJ, DIM);
      break;
    /* below need double check */
    case MusicalScaleType::SCALE_HARMONIC_MAJOR:
      break;
    case MusicalScaleType::SCALE_MAJOR_PENTATONIC:
      break;
    case MusicalScaleType::SCALE_MINOR_PENTATONIC:
      break;
    case MusicalScaleType::SCALE_PHRYGIAN_DOMINANT:
      break;
    case MusicalScaleType::SCALE_MAJOR_LOCRIAN:
      break;
    case MusicalScaleType::SCALE_ALGERIAN:
      break;
    case MusicalScaleType::SCALE_AUGMENTED:
      break;
    case MusicalScaleType::SCALE_DOUBLE_HARMONIC:
      break;
    case MusicalScaleType::SCALE_CHINESE:
      break;
    case MusicalScaleType::SCALE_DIMINISHED:
      break;
    case MusicalScaleType::SCALE_DOMINANT_DIMINISHED:
      break;
    case MusicalScaleType::SCALE_EGYPTIAN:
      break;
    case MusicalScaleType::SCALE_EIGHT_TONE_SPANISH:
      break;
    case MusicalScaleType::SCALE_ENIGMATIC:
      break;
    case MusicalScaleType::SCALE_GEEZ:
      break;
    case MusicalScaleType::SCALE_HINDU:
      break;
    case MusicalScaleType::SCALE_HIRAJOSHI:
      break;
    case MusicalScaleType::SCALE_HUNGARIAN_GYPSY:
      break;
    case MusicalScaleType::SCALE_INSEN:
      break;
    case MusicalScaleType::SCALE_NEAPOLITAN_MINOR:
      break;
    case MusicalScaleType::SCALE_NEAPOLITAN_MAJOR:
      break;
    case MusicalScaleType::SCALE_OCTATONIC_HALF_WHOLE:
      break;
    case MusicalScaleType::SCALE_OCTATONIC_WHOLE_HALF:
      break;
    case MusicalScaleType::SCALE_ORIENTAL:
      break;
    case MusicalScaleType::SCALE_WHOLE_TONE:
      break;
    case MusicalScaleType::SCALE_ROMANIAN_MINOR:
      break;
    case MusicalScaleType::SCALE_ALTERED:
      break;
    case MusicalScaleType::SCALE_MAQAM:
      break;
    case MusicalScaleType::SCALE_YO:
      break;
    default:
      break;
    }

  /* return no triads for unimplemented scales */
  SET_7_TRIADS (NONE, NONE, NONE, NONE, NONE, NONE, NONE);

#undef SET_TRIADS
#undef SET_7_TRIADS

  /*return NULL;*/
}

/**
 * Returns the notes in the given scale.
 *
 * @param ascending Whether to get the notes when
 *   ascending or descending (some scales have
 *   different notes when rising/falling).
 */
const bool *
musical_scale_get_notes (MusicalScaleType scale_type, bool ascending)
{
#define SET_NOTES(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12) \
  { \
    static const bool notes[] = { \
      n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12 \
    }; \
    return notes; \
  }

  /* get the notes starting at C */
  switch (scale_type)
    {
    case MusicalScaleType::SCALE_CHROMATIC:
      SET_NOTES (1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
      break;
    case MusicalScaleType::SCALE_MAJOR:
    case MusicalScaleType::SCALE_IONIAN:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_DORIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_PHRYGIAN:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_LYDIAN:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_MIXOLYDIAN:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_MINOR:
    case MusicalScaleType::SCALE_AEOLIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_LOCRIAN:
      SET_NOTES (1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_MELODIC_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_HARMONIC_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    /* below need double check */
    case MusicalScaleType::SCALE_HARMONIC_MAJOR:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_MAJOR_PENTATONIC:
      SET_NOTES (1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0);
      break;
    case MusicalScaleType::SCALE_MINOR_PENTATONIC:
      SET_NOTES (1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_PHRYGIAN_DOMINANT:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_MAJOR_LOCRIAN:
      SET_NOTES (1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_ACOUSTIC:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_ALTERED:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_ALGERIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_AUGMENTED:
      SET_NOTES (1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_DOUBLE_HARMONIC:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_CHINESE:
      SET_NOTES (1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_DIMINISHED:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_DOMINANT_DIMINISHED:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_EGYPTIAN:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_EIGHT_TONE_SPANISH:
      SET_NOTES (1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_ENIGMATIC:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1);
      break;
    case MusicalScaleType::SCALE_GEEZ:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_HINDU:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_HIRAJOSHI:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_HUNGARIAN_GYPSY:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_INSEN:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_NEAPOLITAN_MAJOR:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_NEAPOLITAN_MINOR:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_OCTATONIC_HALF_WHOLE:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_OCTATONIC_WHOLE_HALF:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_ORIENTAL:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_WHOLE_TONE:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_ROMANIAN_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_MAQAM:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_YO:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0);
      break;
    case MusicalScaleType::SCALE_BEBOP_LOCRIAN:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1);
      break;
    case MusicalScaleType::SCALE_BEBOP_DOMINANT:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1);
      break;
    case MusicalScaleType::SCALE_BEBOP_MAJOR:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1);
      break;
    case MusicalScaleType::SCALE_SUPER_LOCRIAN:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_ENIGMATIC_MINOR:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1);
      break;
    case MusicalScaleType::SCALE_COMPOSITE:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_BHAIRAV:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_HUNGARIAN_MINOR:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_PERSIAN:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1);
      break;
    case MusicalScaleType::SCALE_IWATO:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_KUMOI:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0);
      break;
    case MusicalScaleType::SCALE_PELOG:
      SET_NOTES (1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0);
      break;
    case MusicalScaleType::SCALE_PROMETHEUS:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_PROMETHEUS_NEAPOLITAN:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case MusicalScaleType::SCALE_PROMETHEUS_LISZT:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0);
      break;
    case MusicalScaleType::SCALE_BALINESE:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0);
      break;
    case MusicalScaleType::SCALE_RAGATODI:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0);
      break;
    case MusicalScaleType::SCALE_JAPANESE1:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0);
      break;
    case MusicalScaleType::SCALE_JAPANESE2:
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
musical_scale_contains_note (const MusicalScale * scale, MusicalNote note)
{
  if (scale->root_key == note)
    return 1;

  const bool * notes = musical_scale_get_notes (scale->type, false);

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
        && !musical_scale_contains_note (
          scale, ENUM_INT_TO_VALUE (MusicalNote, i % 12)))
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

  unsigned int min_seventh_sems = type == ChordType::CHORD_TYPE_DIM ? 9 : 10;

  int chord_root_int = ENUM_VALUE_TO_INT (chord_root);

  /* if 7th not in scale no need to check > 7th */
  if (
    chord_accent >= ChordAccent::CHORD_ACC_b9
    && !musical_scale_contains_note (
      scale,
      ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + min_seventh_sems) % 12)))
    return 0;

  switch (chord_accent)
    {
    case ChordAccent::CHORD_ACC_NONE:
      return 1;
    case ChordAccent::CHORD_ACC_7:
      return musical_scale_contains_note (
        scale,
        ENUM_INT_TO_VALUE (
          MusicalNote, (chord_root_int + min_seventh_sems) % 12));
    case ChordAccent::CHORD_ACC_j7:
      return musical_scale_contains_note (
        scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 11) % 12));
    case ChordAccent::CHORD_ACC_b9:
      return musical_scale_contains_note (
        scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 13) % 12));
    case ChordAccent::CHORD_ACC_9:
      return musical_scale_contains_note (
        scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 14) % 12));
    case ChordAccent::CHORD_ACC_S9:
      return musical_scale_contains_note (
        scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 15) % 12));
    case ChordAccent::CHORD_ACC_11:
      return musical_scale_contains_note (
        scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 17) % 12));
    case ChordAccent::CHORD_ACC_b5_S11:
      return musical_scale_contains_note (
               scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 6) % 12))
             && musical_scale_contains_note (
               scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 18) % 12));
    case ChordAccent::CHORD_ACC_S5_b13:
      return musical_scale_contains_note (
               scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 8) % 12))
             && musical_scale_contains_note (
               scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 16) % 12));
    case ChordAccent::CHORD_ACC_6_13:
      return musical_scale_contains_note (
               scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 9) % 12))
             && musical_scale_contains_note (
               scale, ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 21) % 12));
    default:
      return 0;
    }
}

const char *
musical_scale_type_to_string (const MusicalScaleType type)
{
  static const char * musical_scale_type_strings[] = {
    N_ ("Chromatic"),
    N_ ("Major"),
    N_ ("Minor"),
    N_ ("Ionian"),
    N_ ("Dorian"),
    N_ ("Phrygian"),
    N_ ("Lydian"),
    N_ ("Mixolydian"),
    N_ ("Aeolian"),
    N_ ("Locrian"),
    N_ ("Melodic Minor"),
    N_ ("Harmonic Minor"),
    N_ ("Whole Tone"),
    N_ ("Major Pentatonic"),
    N_ ("Minor Pentatonic"),
    N_ ("Octatonic Half Whole"),
    N_ ("Octatonic Whole Half"),
    N_ ("Acoustic"),
    N_ ("Harmonic Major"),
    N_ ("Phrygian Dominant"),
    N_ ("Major Locrian"),
    N_ ("Algerian"),
    N_ ("Augmented"),
    N_ ("Double Harmonic"),
    N_ ("Chinese"),
    N_ ("Diminished"),
    N_ ("Dominant Diminished"),
    N_ ("Egyptian"),
    N_ ("Eight Tone Spanish"),
    N_ ("Enigmatic"),
    N_ ("Geez"),
    N_ ("Hindu"),
    N_ ("Hirajoshi"),
    N_ ("Hungarian Gypsy"),
    N_ ("Insen"),
    N_ ("Neapolitan Major"),
    N_ ("Neapolitan Minor"),
    N_ ("Oriental"),
    N_ ("Romanian Minor"),
    N_ ("Altered"),
    N_ ("Maqam"),
    N_ ("Yo"),
    N_ ("Bebop Locrian"),
    N_ ("Bebop Dominant"),
    N_ ("Bebop Major"),
    N_ ("Super Locrian"),
    N_ ("Enigmatic Minor"),
    N_ ("Composite"),
    N_ ("Bhairav"),
    N_ ("Hungarian Minor"),
    N_ ("Persian"),
    N_ ("Iwato"),
    N_ ("Kumoi"),
    N_ ("Pelog"),
    N_ ("Prometheus"),
    N_ ("Prometheus Neapolitan"),
    N_ ("Prometheus Liszt"),
    N_ ("Balinese"),
    N_ ("RagaTodi"),
    N_ ("Japanese 1"),
    N_ ("Japanese 2"),
  };
  return _ (musical_scale_type_strings[ENUM_VALUE_TO_INT (type)]);
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
  case MusicalScaleType::SCALE_##uppercase: \
    sprintf ( \
      buf, "%s %s", chord_descriptor_note_to_string (scale->root_key), \
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
