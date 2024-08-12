// SPDX-FileCopyrightText: © 2018-2019, 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Musical scales.
 *
 * See https://pianoscales.org/
 */

#include <cstdlib>

#include "dsp/chord_descriptor.h"
#include "dsp/scale.h"
#include "utils/types.h"

#include <glib/gi18n.h>

const ChordType *
MusicalScale::get_triad_types_for_type (Type type, bool ascending)
{
#define SET_TRIADS(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12) \
  { \
    static const ChordType chord_types[] = { \
      n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12 \
    }; \
    return chord_types; \
  }

#define SET_7_TRIADS(n1, n2, n3, n4, n5, n6, n7) \
  SET_TRIADS ( \
    n1, n2, n3, n4, n5, n6, n7, ChordType::None, ChordType::None, \
    ChordType::None, ChordType::None, ChordType::None)

  /* get the notes starting at C */
  switch (type)
    {
    case Type::Chromatic:
      SET_7_TRIADS (
        ChordType::None, ChordType::None, ChordType::None, ChordType::None,
        ChordType::None, ChordType::None, ChordType::None);
      break;
    case Type::Major:
    case Type::Ionian:
      SET_7_TRIADS (
        ChordType::Major, ChordType::Minor, ChordType::Minor, ChordType::Major,
        ChordType::Major, ChordType::Minor, ChordType::Diminished);
      break;
    case Type::Dorian:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Minor, ChordType::Major, ChordType::Major,
        ChordType::Minor, ChordType::Diminished, ChordType::Major);
      break;
    case Type::Phrygian:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Major, ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Major, ChordType::Minor);
      break;
    case Type::Lydian:
      SET_7_TRIADS (
        ChordType::Major, ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Major, ChordType::Minor,
        ChordType::Minor);
      break;
    case Type::Mixolydian:
      SET_7_TRIADS (
        ChordType::Major, ChordType::Minor, ChordType::Diminished,
        ChordType::Major, ChordType::Minor, ChordType::Minor, ChordType::Major);
      break;
    case Type::Minor:
    case Type::Aeolian:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Diminished, ChordType::Major,
        ChordType::Minor, ChordType::Minor, ChordType::Major, ChordType::Major);
      break;
    case Type::Locrian:
      SET_7_TRIADS (
        ChordType::Diminished, ChordType::Major, ChordType::Minor,
        ChordType::Minor, ChordType::Major, ChordType::Major, ChordType::Minor);
      break;
    case Type::MelodicMinor:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Minor, ChordType::Augmented,
        ChordType::Major, ChordType::Major, ChordType::Diminished,
        ChordType::Diminished);
      break;
    case Type::HarmonicMinor:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Diminished, ChordType::Augmented,
        ChordType::Minor, ChordType::Major, ChordType::Major,
        ChordType::Diminished);
      break;
    default:
      break;
    }

  /* return no triads for unimplemented scales */
  SET_7_TRIADS (
    ChordType::None, ChordType::None, ChordType::None, ChordType::None,
    ChordType::None, ChordType::None, ChordType::None);

#undef SET_TRIADS
#undef SET_7_TRIADS
}

const bool *
MusicalScale::get_notes_for_type (Type type, bool ascending)
{
#define SET_NOTES(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12) \
  { \
    static const bool notes[] = { \
      n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12 \
    }; \
    return notes; \
  }

  /* get the notes starting at C */
  switch (type)
    {
    case Type::Chromatic:
      SET_NOTES (1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
      break;
    case Type::Major:
    case Type::Ionian:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1);
      break;
    case Type::Dorian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0);
      break;
    case Type::Phrygian:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case Type::Lydian:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1);
      break;
    case Type::Mixolydian:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0);
      break;
    case Type::Minor:
    case Type::Aeolian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case Type::Locrian:
      SET_NOTES (1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0);
      break;
    case Type::MelodicMinor:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case Type::HarmonicMinor:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    /* below need double check */
    case Type::HarmonicMajor:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1);
      break;
    case Type::MajorPentatonic:
      SET_NOTES (1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0);
      break;
    case Type::MinorPentatonic:
      SET_NOTES (1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case Type::PhrygianDominant:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case Type::MajorLocrian:
      SET_NOTES (1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case Type::Acoustic:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case Type::Altered:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case Type::Algerian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1);
      break;
    case Type::Augmented:
      SET_NOTES (1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1);
      break;
    case Type::DoubleHarmonic:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case Type::Chinese:
      SET_NOTES (1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1);
      break;
    case Type::Diminished:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case Type::DominantDiminished:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case Type::Egyptian:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case Type::EightToneSpanish:
      SET_NOTES (1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case Type::Enigmatic:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1);
      break;
    case Type::Geez:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case Type::Hindu:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case Type::Hirajoshi:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case Type::HungarianGypsy:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case Type::Insen:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case Type::NeapolitanMajor:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case Type::NeapolitanMinor:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    case Type::OctatonicHalfWhole:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case Type::OctatonicWholeHalf:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case Type::Oriental:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0);
      break;
    case Type::WholeTone:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case Type::RomanianMinor:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0);
      break;
    case Type::Maqam:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case Type::Yo:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0);
      break;
    case Type::BebopLocrian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1);
      break;
    case Type::BebopDominant:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1);
      break;
    case Type::BebopMajor:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1);
      break;
    case Type::SuperLocrian:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case Type::EnigmaticMinor:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1);
      break;
    case Type::Composite:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1);
      break;
    case Type::Bhairav:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case Type::HungarianMinor:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case Type::Persian:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1);
      break;
    case Type::Iwato:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case Type::Kumoi:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0);
      break;
    case Type::Pelog:
      SET_NOTES (1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0);
      break;
    case Type::Prometheus:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case Type::PrometheusNeapolitan:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case Type::PrometheusLiszt:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0);
      break;
    case Type::Balinese:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0);
      break;
    case Type::Ragatodi:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0);
      break;
    case Type::Japanese1:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0);
      break;
    case Type::Japanese2:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0);
      break;
    default:
      break;
    }

#undef SET_NOTES

  return NULL;
}

bool
MusicalScale::contains_note (MusicalNote note) const
{
  if (root_key_ == note)
    return 1;

  const bool * notes = get_notes_for_type (type_, false);

  return notes[((int) note - (int) root_key_ + 12) % 12];
}

bool
MusicalScale::contains_chord (const ChordDescriptor &chord) const
{
  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      if (
        chord.notes_[i]
        && !contains_note (ENUM_INT_TO_VALUE (MusicalNote, i % 12)))
        return 0;
    }
  return 1;
}

/**
 * Returns if the accent is in the scale.
 */
bool
MusicalScale::is_accent_in_scale (
  MusicalNote chord_root,
  ChordType   type,
  ChordAccent chord_accent) const
{
  if (!contains_note (chord_root))
    return false;

  int min_seventh_sems = type == ChordType::Diminished ? 9 : 10;

  int chord_root_int = (int) chord_root;

  /* if 7th not in scale no need to check > 7th */
  if (
    chord_accent >= ChordAccent::FlatNinth
    && !contains_note (ENUM_INT_TO_VALUE (
      MusicalNote, (chord_root_int + min_seventh_sems) % 12)))
    return false;

  switch (chord_accent)
    {
    case ChordAccent::None:
      return true;
    case ChordAccent::Seventh:
      return contains_note (ENUM_INT_TO_VALUE (
        MusicalNote, (chord_root_int + min_seventh_sems) % 12));
    case ChordAccent::MajorSeventh:
      return contains_note (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 11) % 12));
    case ChordAccent::FlatNinth:
      return contains_note (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 13) % 12));
    case ChordAccent::Ninth:
      return contains_note (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 14) % 12));
    case ChordAccent::SharpNinth:
      return contains_note (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 15) % 12));
    case ChordAccent::Eleventh:
      return contains_note (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 17) % 12));
    case ChordAccent::FlatFifthSharpEleventh:
      return contains_note (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 6) % 12))
             && contains_note (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 18) % 12));
    case ChordAccent::SharpFifthFlatThirteenth:
      return contains_note (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 8) % 12))
             && contains_note (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 16) % 12));
    case ChordAccent::SixthThirteenth:
      return contains_note (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 9) % 12))
             && contains_note (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 21) % 12));
    default:
      return false;
    }
}

const char *
MusicalScale::type_to_string (Type type)
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
  return _ (musical_scale_type_strings[(int) type]);
}

std::string
MusicalScale::to_string () const
{
  return std::string (ChordDescriptor::note_to_string (root_key_)) + " "
         + type_to_string (type_);
}