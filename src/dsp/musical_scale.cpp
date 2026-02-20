// SPDX-FileCopyrightText: © 2018-2019, 2021-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/musical_scale.h"

namespace zrythm::dsp
{

std::array<ChordType, 12>
MusicalScale::get_triad_types_for_type (ScaleType type, bool ascending)
{
#define SET_TRIADS(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12) \
  { \
    const std::array<ChordType, 12> chord_types = { \
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
    case ScaleType::Chromatic:
      SET_7_TRIADS (
        ChordType::None, ChordType::None, ChordType::None, ChordType::None,
        ChordType::None, ChordType::None, ChordType::None);
      break;
    case ScaleType::Major:
    case ScaleType::Ionian:
      SET_7_TRIADS (
        ChordType::Major, ChordType::Minor, ChordType::Minor, ChordType::Major,
        ChordType::Major, ChordType::Minor, ChordType::Diminished);
      break;
    case ScaleType::Dorian:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Minor, ChordType::Major, ChordType::Major,
        ChordType::Minor, ChordType::Diminished, ChordType::Major);
      break;
    case ScaleType::Phrygian:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Major, ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Major, ChordType::Minor);
      break;
    case ScaleType::Lydian:
      SET_7_TRIADS (
        ChordType::Major, ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Major, ChordType::Minor,
        ChordType::Minor);
      break;
    case ScaleType::Mixolydian:
      SET_7_TRIADS (
        ChordType::Major, ChordType::Minor, ChordType::Diminished,
        ChordType::Major, ChordType::Minor, ChordType::Minor, ChordType::Major);
      break;
    case ScaleType::Minor:
    case ScaleType::Aeolian:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Diminished, ChordType::Major,
        ChordType::Minor, ChordType::Minor, ChordType::Major, ChordType::Major);
      break;
    case ScaleType::Locrian:
      SET_7_TRIADS (
        ChordType::Diminished, ChordType::Major, ChordType::Minor,
        ChordType::Minor, ChordType::Major, ChordType::Major, ChordType::Minor);
      break;
    case ScaleType::MelodicMinor:
      SET_7_TRIADS (
        ChordType::Minor, ChordType::Minor, ChordType::Augmented,
        ChordType::Major, ChordType::Major, ChordType::Diminished,
        ChordType::Diminished);
      break;
    case ScaleType::HarmonicMinor:
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
MusicalScale::get_notes_for_type (ScaleType type, bool ascending)
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
    case ScaleType::Chromatic:
      SET_NOTES (1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
      break;
    case ScaleType::Major:
    case ScaleType::Ionian:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1);
      break;
    case ScaleType::Dorian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0);
      break;
    case ScaleType::Phrygian:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case ScaleType::Lydian:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1);
      break;
    case ScaleType::Mixolydian:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0);
      break;
    case ScaleType::Minor:
    case ScaleType::Aeolian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case ScaleType::Locrian:
      SET_NOTES (1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0);
      break;
    case ScaleType::MelodicMinor:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case ScaleType::HarmonicMinor:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    /* below need double check */
    case ScaleType::HarmonicMajor:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1);
      break;
    case ScaleType::MajorPentatonic:
      SET_NOTES (1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0);
      break;
    case ScaleType::MinorPentatonic:
      SET_NOTES (1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case ScaleType::PhrygianDominant:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case ScaleType::MajorLocrian:
      SET_NOTES (1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case ScaleType::Acoustic:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case ScaleType::Altered:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case ScaleType::Algerian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1);
      break;
    case ScaleType::Augmented:
      SET_NOTES (1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1);
      break;
    case ScaleType::DoubleHarmonic:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case ScaleType::Chinese:
      SET_NOTES (1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1);
      break;
    case ScaleType::Diminished:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case ScaleType::DominantDiminished:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case ScaleType::Egyptian:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case ScaleType::EightToneSpanish:
      SET_NOTES (1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0);
      break;
    case ScaleType::Enigmatic:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1);
      break;
    case ScaleType::Geez:
      SET_NOTES (1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0);
      break;
    case ScaleType::Hindu:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0);
      break;
    case ScaleType::Hirajoshi:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case ScaleType::HungarianGypsy:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case ScaleType::Insen:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0);
      break;
    case ScaleType::NeapolitanMajor:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
      break;
    case ScaleType::NeapolitanMinor:
      SET_NOTES (1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1);
      break;
    case ScaleType::OctatonicHalfWhole:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0);
      break;
    case ScaleType::OctatonicWholeHalf:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1);
      break;
    case ScaleType::Oriental:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0);
      break;
    case ScaleType::WholeTone:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case ScaleType::RomanianMinor:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0);
      break;
    case ScaleType::Maqam:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case ScaleType::Yo:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0);
      break;
    case ScaleType::BebopLocrian:
      SET_NOTES (1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1);
      break;
    case ScaleType::BebopDominant:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1);
      break;
    case ScaleType::BebopMajor:
      SET_NOTES (1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1);
      break;
    case ScaleType::SuperLocrian:
      SET_NOTES (1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0);
      break;
    case ScaleType::EnigmaticMinor:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1);
      break;
    case ScaleType::Composite:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1);
      break;
    case ScaleType::Bhairav:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1);
      break;
    case ScaleType::HungarianMinor:
      SET_NOTES (1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1);
      break;
    case ScaleType::Persian:
      SET_NOTES (1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1);
      break;
    case ScaleType::Iwato:
      SET_NOTES (1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0);
      break;
    case ScaleType::Kumoi:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0);
      break;
    case ScaleType::Pelog:
      SET_NOTES (1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0);
      break;
    case ScaleType::Prometheus:
      SET_NOTES (1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case ScaleType::PrometheusNeapolitan:
      SET_NOTES (1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0);
      break;
    case ScaleType::PrometheusLiszt:
      SET_NOTES (1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0);
      break;
    case ScaleType::Balinese:
      SET_NOTES (1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0);
      break;
    case ScaleType::Ragatodi:
      SET_NOTES (1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0);
      break;
    case ScaleType::Japanese1:
      SET_NOTES (1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0);
      break;
    case ScaleType::Japanese2:
      SET_NOTES (1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0);
      break;
    default:
      break;
    }

#undef SET_NOTES

  return nullptr;
}

bool
MusicalScale::containsNote (MusicalNote note) const
{
  if (root_key_ == note)
    return true;

  // return false if out of bounds
  const auto note_int = std::to_underlying (note);
  if (note_int >= 12) [[unlikely]]
    {
      return false;
    }

  const bool * notes = get_notes_for_type (type_, false);

  return notes[(note_int - std::to_underlying (root_key_) + 12) % 12];
}

bool
MusicalScale::contains_chord (const ChordDescriptor &chord) const
{
  for (size_t i = 0; i < ChordDescriptor::MAX_NOTES; i++)
    {
      if (
        chord.notes_[i]
        && !containsNote (ENUM_INT_TO_VALUE (MusicalNote, i % 12)))
        return false;
    }
  return true;
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
  if (!containsNote (chord_root))
    return false;

  int min_seventh_sems = type == ChordType::Diminished ? 9 : 10;

  int chord_root_int = (int) chord_root;

  /* if 7th not in scale no need to check > 7th */
  if (
    chord_accent >= ChordAccent::FlatNinth
    && !containsNote (ENUM_INT_TO_VALUE (
      MusicalNote, (chord_root_int + min_seventh_sems) % 12)))
    return false;

  switch (chord_accent)
    {
    case ChordAccent::None:
      return true;
    case ChordAccent::Seventh:
      return containsNote (ENUM_INT_TO_VALUE (
        MusicalNote, (chord_root_int + min_seventh_sems) % 12));
    case ChordAccent::MajorSeventh:
      return containsNote (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 11) % 12));
    case ChordAccent::FlatNinth:
      return containsNote (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 13) % 12));
    case ChordAccent::Ninth:
      return containsNote (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 14) % 12));
    case ChordAccent::SharpNinth:
      return containsNote (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 15) % 12));
    case ChordAccent::Eleventh:
      return containsNote (
        ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 17) % 12));
    case ChordAccent::FlatFifthSharpEleventh:
      return containsNote (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 6) % 12))
             && containsNote (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 18) % 12));
    case ChordAccent::SharpFifthFlatThirteenth:
      return containsNote (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 8) % 12))
             && containsNote (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 16) % 12));
    case ChordAccent::SixthThirteenth:
      return containsNote (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 9) % 12))
             && containsNote (
               ENUM_INT_TO_VALUE (MusicalNote, (chord_root_int + 21) % 12));
    default:
      return false;
    }
}

utils::Utf8String
MusicalScale::type_to_string (ScaleType type)
{
  constexpr std::string_view musical_scale_type_strings[] = {
    QT_TR_NOOP_UTF8 ("Chromatic"),
    QT_TR_NOOP_UTF8 ("Major"),
    QT_TR_NOOP_UTF8 ("Minor"),
    QT_TR_NOOP_UTF8 ("Ionian"),
    QT_TR_NOOP_UTF8 ("Dorian"),
    QT_TR_NOOP_UTF8 ("Phrygian"),
    QT_TR_NOOP_UTF8 ("Lydian"),
    QT_TR_NOOP_UTF8 ("Mixolydian"),
    QT_TR_NOOP_UTF8 ("Aeolian"),
    QT_TR_NOOP_UTF8 ("Locrian"),
    QT_TR_NOOP_UTF8 ("Melodic Minor"),
    QT_TR_NOOP_UTF8 ("Harmonic Minor"),
    QT_TR_NOOP_UTF8 ("Whole Tone"),
    QT_TR_NOOP_UTF8 ("Major Pentatonic"),
    QT_TR_NOOP_UTF8 ("Minor Pentatonic"),
    QT_TR_NOOP_UTF8 ("Octatonic Half Whole"),
    QT_TR_NOOP_UTF8 ("Octatonic Whole Half"),
    QT_TR_NOOP_UTF8 ("Acoustic"),
    QT_TR_NOOP_UTF8 ("Harmonic Major"),
    QT_TR_NOOP_UTF8 ("Phrygian Dominant"),
    QT_TR_NOOP_UTF8 ("Major Locrian"),
    QT_TR_NOOP_UTF8 ("Algerian"),
    QT_TR_NOOP_UTF8 ("Augmented"),
    QT_TR_NOOP_UTF8 ("Double Harmonic"),
    QT_TR_NOOP_UTF8 ("Chinese"),
    QT_TR_NOOP_UTF8 ("Diminished"),
    QT_TR_NOOP_UTF8 ("Dominant Diminished"),
    QT_TR_NOOP_UTF8 ("Egyptian"),
    QT_TR_NOOP_UTF8 ("Eight Tone Spanish"),
    QT_TR_NOOP_UTF8 ("Enigmatic"),
    QT_TR_NOOP_UTF8 ("Geez"),
    QT_TR_NOOP_UTF8 ("Hindu"),
    QT_TR_NOOP_UTF8 ("Hirajoshi"),
    QT_TR_NOOP_UTF8 ("Hungarian Gypsy"),
    QT_TR_NOOP_UTF8 ("Insen"),
    QT_TR_NOOP_UTF8 ("Neapolitan Major"),
    QT_TR_NOOP_UTF8 ("Neapolitan Minor"),
    QT_TR_NOOP_UTF8 ("Oriental"),
    QT_TR_NOOP_UTF8 ("Romanian Minor"),
    QT_TR_NOOP_UTF8 ("Altered"),
    QT_TR_NOOP_UTF8 ("Maqam"),
    QT_TR_NOOP_UTF8 ("Yo"),
    QT_TR_NOOP_UTF8 ("Bebop Locrian"),
    QT_TR_NOOP_UTF8 ("Bebop Dominant"),
    QT_TR_NOOP_UTF8 ("Bebop Major"),
    QT_TR_NOOP_UTF8 ("Super Locrian"),
    QT_TR_NOOP_UTF8 ("Enigmatic Minor"),
    QT_TR_NOOP_UTF8 ("Composite"),
    QT_TR_NOOP_UTF8 ("Bhairav"),
    QT_TR_NOOP_UTF8 ("Hungarian Minor"),
    QT_TR_NOOP_UTF8 ("Persian"),
    QT_TR_NOOP_UTF8 ("Iwato"),
    QT_TR_NOOP_UTF8 ("Kumoi"),
    QT_TR_NOOP_UTF8 ("Pelog"),
    QT_TR_NOOP_UTF8 ("Prometheus"),
    QT_TR_NOOP_UTF8 ("Prometheus Neapolitan"),
    QT_TR_NOOP_UTF8 ("Prometheus Liszt"),
    QT_TR_NOOP_UTF8 ("Balinese"),
    QT_TR_NOOP_UTF8 ("RagaTodi"),
    QT_TR_NOOP_UTF8 ("Japanese 1"),
    QT_TR_NOOP_UTF8 ("Japanese 2"),
  };
  return utils::Utf8String::from_qstring (
    QObject::tr (musical_scale_type_strings[(int) type].data ()));
}

utils::Utf8String
MusicalScale::to_string () const
{
  return ChordDescriptor::note_to_string (root_key_)
         + utils::Utf8String{ u8" " } + type_to_string (type_);
}

void
init_from (
  MusicalScale          &obj,
  const MusicalScale    &other,
  utils::ObjectCloneType clone_type)
{
  obj.type_ = other.type_;
  obj.root_key_ = other.root_key_;
}

void
to_json (nlohmann::json &j, const MusicalScale &s)
{
  j[MusicalScale::kTypeKey] = s.type_;
  j[MusicalScale::kRootKeyKey] = s.root_key_;
}

void
from_json (const nlohmann::json &j, MusicalScale &s)
{
  j[MusicalScale::kTypeKey].get_to (s.type_);
  j[MusicalScale::kRootKeyKey].get_to (s.root_key_);
}

} // namespace zrythm::dsp
