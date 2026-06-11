// SPDX-FileCopyrightText: © 2018-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "utils/enum_utils.h"
#include "utils/logger.h"
#include "utils/midi.h"
#include "utils/utf8_string.h"

#include <boost/container/static_vector.hpp>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

boost::container::static_vector<int, ChordDescriptor::kMaxIntervals>
ChordDescriptor::getIntervals () const
{
  if (custom_intervals_)
    return *custom_intervals_;

  if (type_ == ChordType::None || type_ == ChordType::Custom)
    return {};

  boost::container::static_vector<int, kMaxIntervals> intervals;

  switch (type_)
    {
    case ChordType::Major:
      intervals = { 0, 4, 7 };
      break;
    case ChordType::Minor:
      intervals = { 0, 3, 7 };
      break;
    case ChordType::Diminished:
      intervals = { 0, 3, 6 };
      break;
    case ChordType::Augmented:
      intervals = { 0, 4, 8 };
      break;
    case ChordType::SuspendedSecond:
      intervals = { 0, 2, 7 };
      break;
    case ChordType::SuspendedFourth:
      intervals = { 0, 5, 7 };
      break;
    default:
      return {};
    }

  const unsigned int min_seventh = type_ == ChordType::Diminished ? 9 : 10;

  switch (accent_)
    {
    case ChordAccent::None:
      break;
    case ChordAccent::Seventh:
      intervals.push_back (min_seventh);
      break;
    case ChordAccent::MajorSeventh:
      intervals.push_back (11);
      break;
    case ChordAccent::FlatNinth:
      intervals.push_back (min_seventh);
      intervals.push_back (13);
      break;
    case ChordAccent::Ninth:
      intervals.push_back (min_seventh);
      intervals.push_back (14);
      break;
    case ChordAccent::SharpNinth:
      intervals.push_back (min_seventh);
      intervals.push_back (15);
      break;
    case ChordAccent::Eleventh:
      intervals.push_back (min_seventh);
      intervals.push_back (17);
      break;
    case ChordAccent::FlatFifthSharpEleventh:
      intervals.push_back (min_seventh);
      intervals.push_back (6);
      intervals.push_back (18);
      break;
    case ChordAccent::SharpFifthFlatThirteenth:
      intervals.push_back (min_seventh);
      intervals.push_back (8);
      intervals.push_back (16);
      break;
    case ChordAccent::SixthThirteenth:
      intervals.push_back (min_seventh);
      intervals.push_back (9);
      intervals.push_back (21);
      break;
    default:
      z_warning ("chord accent unimplemented");
      break;
    }

  return intervals;
}

auto
ChordDescriptor::getMidiPitches (std::uint8_t base_pitch) const -> ChordPitches
{
  ChordPitches result;
  const auto   root_val = static_cast<int> (root_note_);

  if (bass_note_)
    {
      const auto pitch = base_pitch + static_cast<int> (*bass_note_);
      if (pitch >= 0 && pitch <= 127)
        result.data[result.count++] = static_cast<std::uint8_t> (pitch);
    }

  const auto intervals = getIntervals ();
  boost::container::static_vector<std::uint8_t, ChordPitches::kMaxPitches>
    chord_tones;
  for (int interval : intervals)
    {
      const auto pitch = base_pitch + 12 + root_val + interval;
      if (pitch >= 0 && pitch <= 127)
        chord_tones.push_back (static_cast<std::uint8_t> (pitch));
    }

  std::ranges::sort (chord_tones);

  for (int i = 0; i < std::abs (inversion_); i++)
    {
      if (inversion_ > 0)
        {
          std::ranges::sort (chord_tones);
          for (auto &pitch : chord_tones)
            {
              if (pitch + 12 <= 127)
                {
                  pitch += 12;
                  break;
                }
            }
        }
      else
        {
          std::ranges::sort (chord_tones);
          for (auto it = chord_tones.rbegin (); it != chord_tones.rend (); ++it)
            {
              if (*it >= 12)
                {
                  *it -= 12;
                  break;
                }
            }
        }
    }

  for (auto pitch : chord_tones)
    {
      if (result.count < ChordPitches::kMaxPitches)
        result.data[result.count++] = pitch;
    }

  return result;
}

int
ChordDescriptor::maxInversion () const
{
  int max_inv = 2;
  switch (accent_)
    {
    case ChordAccent::None:
      break;
    case ChordAccent::Seventh:
    case ChordAccent::MajorSeventh:
    case ChordAccent::FlatNinth:
    case ChordAccent::Ninth:
    case ChordAccent::SharpNinth:
    case ChordAccent::Eleventh:
      max_inv = 3;
      break;
    case ChordAccent::FlatFifthSharpEleventh:
    case ChordAccent::SharpFifthFlatThirteenth:
    case ChordAccent::SixthThirteenth:
      max_inv = 4;
      break;
    default:
      break;
    }
  return max_inv;
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
    utils::midi::midi_get_note_name (static_cast<midi_byte_t> (note)));
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
  if (bass_note_)
    return *bass_note_ == key;
  return root_note_ == key;
}

bool
ChordDescriptor::is_key_in_chord (MusicalNote key) const
{
  if (is_key_bass (key))
    return true;

  const auto key_val = static_cast<int> (key);
  const auto root_val = static_cast<int> (root_note_);

  for (int interval : getIntervals ())
    {
      if ((root_val + interval) % 12 == key_val)
        return true;
    }
  return false;
}

bool
ChordDescriptor::isEquivalent (const ChordDescriptor &other) const
{
  return root_note_ == other.root_note_ && type_ == other.type_
         && accent_ == other.accent_ && inversion_ == other.inversion_
         && bass_note_ == other.bass_note_
         && custom_intervals_ == other.custom_intervals_;
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
  if (bass_note_ && *bass_note_ != root_note_)
    {
      str += u8"/";
      str += note_to_string (*bass_note_);
    }

  if (inversion_ != 0)
    {
      str += u8" i";
      str += utils::Utf8String::from_utf8_encoded_string (
        std::to_string (inversion_));
    }

  return str;
}

QString
ChordDescriptor::displayName () const
{
  return to_string ().to_qstring ();
}

void
to_json (nlohmann::json &j, const ChordDescriptor &c)
{
  j = {
    { "rootNote",  c.root_note_ },
    { "type",      c.type_      },
    { "accent",    c.accent_    },
    { "inversion", c.inversion_ },
  };
  if (c.bass_note_)
    j["bassNote"] = *c.bass_note_;
}

void
from_json (const nlohmann::json &j, ChordDescriptor &c)
{
  j.at ("rootNote").get_to (c.root_note_);
  j.at ("type").get_to (c.type_);
  j.at ("accent").get_to (c.accent_);
  j.at ("inversion").get_to (c.inversion_);

  if (j.contains ("bassNote"))
    c.bass_note_ = j.at ("bassNote").get<MusicalNote> ();
  else
    c.bass_note_ = std::nullopt;
}

} // namespace zrythm::dsp
