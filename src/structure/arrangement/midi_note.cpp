// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/midi_event.h"
#include "structure/arrangement/midi_note.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
MidiNote::MidiNote (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  QObject *                   parent)
    : ArrangerObject (
        Type::MidiNote,
        tempo_map_wrapper,
        ArrangerObjectFeatures::Bounds | ArrangerObjectFeatures::Mute
          | ArrangerObjectFeatures::ClipOwned,
        parent)
{
  QObject::connect (
    this, &MidiNote::pitchChanged, this, &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &MidiNote::velocityChanged, this, &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &MidiNote::midiChannelChanged, this,
    &ArrangerObject::propertiesChanged);
}

#if 0
  /**
   * Listen to the given MidiNote.
   *
   * @param listen Turn note on if 1, or turn it
   *   off if 0.
   */
void
MidiNote::listen (bool listen)
{
  auto track_var = get_track ();
  std::visit (
    [&] (auto &&track) {
      using TrackT = utils::base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, tracks::PianoRollTrack>)
        {
          auto &events = track->processor_->get_midi_in_port ().buffer_;

          if (listen)
            {
              /* if note is on but pitch changed */
              if (currently_listened_ && pitch_ != last_listened_pitch_)
                {
                  /* create midi note off */
                  events.push_back (dsp::midi_event::make_note_off (0, last_listened_pitch_, 0));

                  /* create note on at the new value */
                  events.push_back (dsp::midi_event::make_note_on (0, pitch_, vel_->vel_, 0));
                  last_listened_pitch_ = pitch_;
                }
              /* if note is on and pitch is the same */
              else if (currently_listened_ && pitch_ == last_listened_pitch_)
                {
                  /* do nothing */
                }
              /* if note is not on */
              else if (!currently_listened_)
                {
                  /* turn it on */
                  events.push_back (dsp::midi_event::make_note_on (0, pitch_, vel_->vel_, 0));
                  last_listened_pitch_ = pitch_;
                  currently_listened_ = 1;
                }
            }
          /* if turning listening off */
          else if (currently_listened_)
            {
              /* create midi note off */
              events.push_back (dsp::midi_event::make_note_off (0, last_listened_pitch_, 0));
              currently_listened_ = false;
              last_listened_pitch_ = 255;
            }
        }
    },
    track_var);
}
#endif

utils::Utf8String
MidiNote::pitch_to_string (uint8_t pitch, Notation notation, bool rich_text)
{
  if (notation == Notation::Pitch && rich_text)
    {
      throw std::invalid_argument ("Pitch notation not supported for rich text");
    }

  if (notation == Notation::Pitch)
    {
      return utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("{}", pitch));
    }

  // below assumes musical notation
  utils::Utf8String note_str;
  note_str = dsp::ChordDescriptor::note_to_string (
    ENUM_INT_TO_VALUE (dsp::MusicalNote, pitch % 12));

  const int note_val = pitch / 12 - 1;
  if (rich_text)
    {
      auto buf = fmt::format ("{}<sup>{}</sup>", note_str, note_val);
      return utils::Utf8String::from_utf8_encoded_string (buf);
    }

  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}{}", note_str, note_val));
}

#if 0
void
MidiNote::set_pitch (const uint8_t val)
{
  z_return_if_fail (val < 128);

  /* if currently playing set a note off event. */
  if (
    is_hit (TRANSPORT->get_playhead_position_in_gui_thread ().frames_)
    && TRANSPORT->isRolling ())
    {
      auto clip = dynamic_cast<MidiClip *> (get_clip ());
      z_return_if_fail (clip);
      auto track_var = clip->get_track ();
      std::visit (
        [&] (auto &&track) {
          using TrackT = utils::base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, tracks::PianoRollTrack>)
            {
              auto &midi_events =
                track->processor_->get_piano_roll_port ().buffer_;

              uint8_t midi_ch = clip->get_midi_ch ();
              midi_events.push_back (dsp::midi_event::make_note_off (midi_ch, pitch_, 0));
            }
        },
        track_var);
    }

  pitch_ = val;
}
#endif

void
init_from (MidiNote &obj, const MidiNote &other, utils::ObjectCloneType clone_type)
{
  obj.pitch_ = other.pitch_;
  obj.setVelocity (other.velocity ());
  obj.setMidiChannel (other.midiChannel ());
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

MidiNote::~MidiNote () = default;

void
to_json (nlohmann::json &j, const MidiNote &note)
{
  to_json (j, static_cast<const ArrangerObject &> (note));
  j[MidiNote::kVelocityKey] = note.velocity_;
  j[MidiNote::kPitchKey] = note.pitch_;
  j[MidiNote::kChannelKey] = note.midi_channel_;
}

void
from_json (const nlohmann::json &j, MidiNote &note)
{
  from_json (j, static_cast<ArrangerObject &> (note));
  j.at (MidiNote::kVelocityKey).get_to (note.velocity_);
  j.at (MidiNote::kPitchKey).get_to (note.pitch_);
  if (j.contains (MidiNote::kChannelKey))
    {
      j.at (MidiNote::kChannelKey).get_to (note.midi_channel_);
    }
}
}
