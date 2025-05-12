// SPDX-FileCopyrightText: © 2018-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <cstring>

#include "dsp/chord_descriptor.h"
#include "utils/midi.h"

namespace zrythm::dsp
{

/**
 * This edits the last 36 notes, skipping the first 12.
 */
static void
invert_chord (auto &notes, int inversion)
{
  if (inversion > 0)
    {
      for (int i = 0; i < inversion; i++)
        {
          for (size_t j = 12; j < ChordDescriptor::MAX_NOTES; j++)
            {
              if (notes[j])
                {
                  notes[j] = false;
                  notes[j + 12] = true;
                  break;
                }
            }
        }
    }
  else if (inversion < 0)
    {
      for (const auto _ : std::views::iota (0, -inversion))
        {
          for (
            const auto j :
            std::views::iota (12zu, ChordDescriptor::MAX_NOTES)
              | std::views::reverse)
            {
              if (notes[j])
                {
                  notes[j] = false;
                  notes[j - 12] = true;
                  break;
                }
            }
        }
    }
}

void
ChordDescriptor::update_notes ()
{
  if (type_ == ChordType::Custom)
    return;

  std::ranges::fill (notes_, false);

  if (type_ == ChordType::None)
    return;

  const auto root = static_cast<size_t> (ENUM_VALUE_TO_INT (root_note_));
  const auto bass = static_cast<size_t> (ENUM_VALUE_TO_INT (bass_note_));

  /* add bass note */
  if (has_bass_)
    {
      notes_[bass] = true;
    }

  /* add root note */
  notes_[12 + root] = true;

  /* add 2 more notes for triad */
  switch (type_)
    {
    case ChordType::Major:
      notes_[12 + root + 4] = true;
      notes_[12 + root + 4 + 3] = true;
      break;
    case ChordType::Minor:
      notes_[12 + root + 3] = true;
      notes_[12 + root + 3 + 4] = true;
      break;
    case ChordType::Diminished:
      notes_[12 + root + 3] = true;
      notes_[12 + root + 3 + 3] = true;
      break;
    case ChordType::Augmented:
      notes_[12 + root + 4] = true;
      notes_[12 + root + 4 + 4] = true;
      break;
    case ChordType::SuspendedSecond:
      notes_[12 + root + 2] = true;
      notes_[12 + root + 2 + 5] = true;
      break;
    case ChordType::SuspendedFourth:
      notes_[12 + root + 5] = true;
      notes_[12 + root + 5 + 2] = true;
      break;
    default:
      z_warning ("chord unimplemented");
      break;
    }

  unsigned int min_seventh_sems = type_ == ChordType::Diminished ? 9 : 10;

  /* add accents */
  const auto root_note_int =
    static_cast<size_t> (ENUM_VALUE_TO_INT (root_note_));
  switch (accent_)
    {
    case ChordAccent::None:
      break;
    case ChordAccent::Seventh:
      notes_[12 + root_note_int + min_seventh_sems] = true;
      break;
    case ChordAccent::MajorSeventh:
      notes_[12 + root_note_int + 11] = true;
      break;
    case ChordAccent::FlatNinth:
      notes_[12 + root_note_int + 13] = true;
      break;
    case ChordAccent::Ninth:
      notes_[12 + root_note_int + 14] = true;
      break;
    case ChordAccent::SharpNinth:
      notes_[12 + root_note_int + 15] = true;
      break;
    case ChordAccent::Eleventh:
      notes_[12 + root_note_int + 17] = true;
      break;
    case ChordAccent::FlatFifthSharpEleventh:
      notes_[12 + root_note_int + 6] = true;
      notes_[12 + root_note_int + 18] = true;
      break;
    case ChordAccent::SharpFifthFlatThirteenth:
      notes_[12 + root_note_int + 8] = true;
      notes_[12 + root_note_int + 16] = true;
      break;
    case ChordAccent::SixthThirteenth:
      notes_[12 + root_note_int + 9] = true;
      notes_[12 + root_note_int + 21] = true;
      break;
    default:
      z_warning ("chord unimplemented");
      break;
    }

  /* add the 7th to accents > 7 */
  if (
    accent_ >= ChordAccent::FlatNinth && accent_ <= ChordAccent::SixthThirteenth)
    {
      notes_[12 + root_note_int + min_seventh_sems] = true;
    }

  invert_chord (notes_, inversion_);
}

static constexpr std::array<std::string_view, 8> chord_type_strings = {
  "Invalid", "Maj", "min", "dim", "sus4", "sus2", "aug", "custom",
};

static constexpr std::array<std::string_view, 10> chord_accent_strings = {
  "None",
  "7",
  "j7",
  "\u266D9",
  "9",
  "\u266F9",
  "11",
  "\u266D5/\u266F11",
  "\u266F5/\u266D13",
  "6/13",
};

utils::Utf8String
ChordDescriptor::note_to_string (MusicalNote note)
{
  return utils::Utf8String::from_utf8_encoded_string (
    utils::midi::midi_get_note_name ((midi_byte_t) note));
}

utils::Utf8String
ChordDescriptor::chord_type_to_string (ChordType type)
{
  return utils::Utf8String::from_utf8_encoded_string (
    chord_type_strings.at (static_cast<size_t> (ENUM_VALUE_TO_INT (type))));
}

utils::Utf8String
ChordDescriptor::chord_accent_to_string (ChordAccent accent)
{
  return utils::Utf8String::from_utf8_encoded_string (
    chord_accent_strings.at (static_cast<size_t> (ENUM_VALUE_TO_INT (accent))));
}

bool
ChordDescriptor::is_key_bass (MusicalNote key) const
{
  if (has_bass_)
    {
      return bass_note_ == key;
    }

  return root_note_ == key;
}

bool
ChordDescriptor::is_key_in_chord (MusicalNote key) const
{
  if (is_key_bass (key))
    {
      return true;
    }

  for (size_t i = 0; i < MAX_NOTES; i++)
    {
      if (notes_[i] && i % 12 == static_cast<decltype (i)> (key))
        return true;
    }
  return false;
}

utils::Utf8String
ChordDescriptor::to_string () const
{
  auto str = note_to_string (root_note_);
  str += chord_type_to_string (type_);

  if (accent_ > ChordAccent::None)
    {
      str += u8" ";
      str += chord_accent_to_string (accent_);
    }
  if (has_bass_ && (bass_note_ != root_note_))
    {
      str += u8"/";
      str += note_to_string (bass_note_);
    }

  if (inversion_ != 0)
    {
      str += u8" i";
      str += utils::Utf8String::from_utf8_encoded_string (
        std::to_string (inversion_));
    }

  return str;
}

} // namespace zrythm::dsp
