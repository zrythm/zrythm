// SPDX-FileCopyrightText: © 2018-2019, 2021-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"
#include "utils/enum_utils.h"
#include "utils/utf8_string.h"

#include <nlohmann/json.hpp>

DEFINE_ENUM_FORMATTER (
  zrythm::dsp::MusicalScale::ScaleType,
  scale_type,
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
  QT_TR_NOOP_UTF8 ("Blues"),
  QT_TR_NOOP_UTF8 ("Flamenco"),
  QT_TR_NOOP_UTF8 ("Gypsy"),
  QT_TR_NOOP_UTF8 ("Half Diminished"),
  QT_TR_NOOP_UTF8 ("In"),
  QT_TR_NOOP_UTF8 ("Istrian"),
  QT_TR_NOOP_UTF8 ("Lydian Augmented"),
  QT_TR_NOOP_UTF8 ("Tritone"),
  QT_TR_NOOP_UTF8 ("Ukranian Dorian"))

static_assert (
  scale_type_count ()
    == magic_enum::enum_count<zrythm::dsp::MusicalScale::ScaleType> (),
  "ScaleType enum and string list out of sync");

// Validate boundary values used by availableScaleTypes() and
// availableScaleTypesExotic().  OctatonicWholeHalf must be the last common
// scale, Acoustic must immediately follow it, and Japanese2 must be the final
// exotic entry.
static_assert (
  static_cast<int> (zrythm::dsp::MusicalScale::ScaleType::OctatonicWholeHalf)
  == 16);
static_assert (
  static_cast<int> (zrythm::dsp::MusicalScale::ScaleType::Acoustic) == 17);
static_assert (
  static_cast<int> (zrythm::dsp::MusicalScale::ScaleType::Japanese2) == 60);

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

boost::container::static_vector<MusicalScale::DiatonicTriad, 12>
MusicalScale::get_diatonic_triads (ScaleType type, MusicalNote root_note)
{
  const auto   triads = get_triad_types_for_type (type, true);
  const auto * notes = get_notes_for_type (type, true);
  if (notes == nullptr)
    return {};

  const auto root_val = std::to_underlying (root_note);

  boost::container::static_vector<DiatonicTriad, 12> result;
  int                                                cur_chord = 0;
  for (int i = 0; i < 12; ++i)
    {
      if (!notes[i])
        continue;

      result.push_back (
        {
          static_cast<MusicalNote> ((root_val + i) % 12),
          triads[cur_chord],
        });
      ++cur_chord;
    }
  return result;
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
MusicalScale::
  scaleContainsNote (ScaleType type, MusicalNote rootKey, MusicalNote note)
{
  const auto note_int = std::to_underlying (note);
  if (note_int >= 12)
    return false;

  if (rootKey == note)
    return true;

  const auto root_int = std::to_underlying (rootKey);
  if (root_int >= 12)
    return false;

  const bool * notes = get_notes_for_type (type, false);
  if (notes == nullptr)
    return false;

  return notes[(note_int - root_int + 12) % 12];
}

QString
MusicalScale::noteToString (MusicalNote note)
{
  return ChordDescriptor::note_to_string (note).to_qstring ();
}

QStringList
MusicalScale::allNoteNames ()
{
  QStringList names;
  names.reserve (12);
  for (int i = 0; i < 12; ++i)
    names.append (noteToString (static_cast<MusicalNote> (i)));
  return names;
}

QVariantList
MusicalScale::availableScaleTypes ()
{
  auto         all = scale_type_to_qml_list ();
  QVariantList list;
  const auto   end = static_cast<int> (ScaleType::OctatonicWholeHalf);
  for (int i = 0; i <= end && i < all.size (); ++i)
    list.append (all.at (i));
  return list;
}

QVariantList
MusicalScale::availableScaleTypesExotic ()
{
  auto         all = scale_type_to_qml_list ();
  QVariantList list;
  const auto   start = static_cast<int> (ScaleType::Acoustic);
  const auto   end = static_cast<int> (ScaleType::Japanese2);
  for (int i = start; i <= end && i < all.size (); ++i)
    list.append (all.at (i));
  return list;
}

QVariantList
MusicalScale::availableScaleTypesWithTriads ()
{
  const auto   all = scale_type_to_qml_list ();
  QVariantList list;
  for (int i = 0; i < static_cast<int> (all.size ()); ++i)
    {
      const auto type = static_cast<ScaleType> (i);
      const auto triads = get_triad_types_for_type (type, true);
      const bool has_real_triads = std::ranges::any_of (
        triads, [] (ChordType t) { return t != ChordType::None; });
      if (has_real_triads)
        list.append (all.at (i));
    }
  return list;
}

QVariantList
MusicalScale::getDiatonicChords (ScaleType type, MusicalNote rootKey)
{
  static const QStringList roman_numerals = {
    "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII",
  };

  const auto   triads = get_triad_types_for_type (type, true);
  const auto * notes = get_notes_for_type (type, true);
  if (notes == nullptr)
    return {};

  const auto root_val = std::to_underlying (rootKey);

  QVariantList result;
  for (int i = 0, deg = 0; i < 12; ++i)
    {
      if (!notes[i])
        continue;

      if (
        deg < static_cast<int> (triads.size ())
        && triads[deg] != ChordType::None)
        {
          const auto note_val = (root_val + i) % 12;
          const auto note = static_cast<MusicalNote> (note_val);
          const auto chord_type = triads[deg];

          const auto note_str = ChordDescriptor::note_to_string (note);
          auto       type_str = utils::Utf8String{};
          if (chord_type != ChordType::Major)
            type_str = ChordDescriptor::chord_type_to_string (chord_type);

          auto degree_label =
            deg < roman_numerals.size ()
              ? roman_numerals[deg]
              : QString::number (deg + 1);
          if (chord_type == ChordType::Minor)
            degree_label = degree_label.toLower ();
          else if (chord_type == ChordType::Diminished)
            degree_label = degree_label.toLower () + u8"\u00B0";

          QVariantMap entry;
          entry["display"] = (note_str + type_str).to_qstring ();
          entry["rootNote"] = note_val;
          entry["chordType"] = std::to_underlying (chord_type);
          entry["degree"] = deg + 1;
          entry["degreeLabel"] = degree_label;
          result.append (entry);
        }
      ++deg;
    }
  return result;
}

bool
MusicalScale::
  containsChord (MusicalNote root, ChordType type, ChordAccent accent) const
{
  if (!containsNote (root))
    return false;

  const auto root_val = std::to_underlying (root);
  const auto intervals =
    ChordDescriptor::get_intervals_for_type_and_accent (type, accent);
  return std::ranges::all_of (intervals, [&] (int interval) {
    return containsNote (
      ENUM_INT_TO_VALUE (MusicalNote, (root_val + interval) % 12));
  });
}

/**
 * Returns if the accent is in the scale.
 */
bool
MusicalScale::isAccentInScale (
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
  return scale_type_to_string (type, true);
}

utils::Utf8String
MusicalScale::to_string () const
{
  return ChordDescriptor::note_to_string (root_key_)
         + utils::Utf8String{ u8" " } + type_to_string (type_);
}

QString
MusicalScale::toString () const
{
  return to_string ().to_qstring ();
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
