// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/midi_note.h"

#include <fmt/format.h>

namespace zrythm::structure::arrangement
{
MidiNote::MidiNote (const dsp::TempoMap &tempo_map, QObject * parent)
    : QObject (parent), ArrangerObject (Type::MidiNote, tempo_map, *this),
      bounds_ (utils::make_qobject_unique<ArrangerObjectBounds> (*position ())),
      mute_ (utils::make_qobject_unique<ArrangerObjectMuteFunctionality> (this))
{
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
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, tracks::PianoRollTrack>)
        {
          auto &events =
            track->processor_->get_midi_in_port ().midi_events_.queued_events_;

          if (listen)
            {
              /* if note is on but pitch changed */
              if (currently_listened_ && pitch_ != last_listened_pitch_)
                {
                  /* create midi note off */
                  events.add_note_off (1, last_listened_pitch_, 0);

                  /* create note on at the new value */
                  events.add_note_on (1, pitch_, vel_->vel_, 0);
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
                  events.add_note_on (1, pitch_, vel_->vel_, 0);
                  last_listened_pitch_ = pitch_;
                  currently_listened_ = 1;
                }
            }
          /* if turning listening off */
          else if (currently_listened_)
            {
              /* create midi note off */
              events.add_note_off (1, last_listened_pitch_, 0);
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
      auto region = dynamic_cast<MidiRegion *> (get_region ());
      z_return_if_fail (region);
      auto track_var = region->get_track ();
      std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, tracks::PianoRollTrack>)
            {
              auto &midi_events =
                track->processor_->get_piano_roll_port ()
                  .midi_events_.queued_events_;

              uint8_t midi_ch = region->get_midi_ch ();
              midi_events.add_note_off (midi_ch, pitch_, 0);
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
  init_from (*obj.bounds_, *other.bounds_, clone_type);
  init_from (*obj.mute_, *other.mute_, clone_type);
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}
}
