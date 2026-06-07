// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "structure/tracks/piano_roll_track.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::tracks
{
void
PianoRollTrackMixin::transform_midi_inputs_func (
  dsp::MidiEventBuffer &events) const
{
  // Change the MIDI channel on the midi input to the channel set on the track
  if (!passthrough_midi_input_)
    {
      const auto ch = static_cast<midi_byte_t> (midi_ch_ - 1);
      transform_scratch_.clear ();
      for (const auto &ev : events)
        {
          const auto d = ev.data ();
          if (d.size () >= 1 && (d[0] & 0xF0) != 0xF0)
            {
              const std::array<midi_byte_t, 3> raw = {
                static_cast<midi_byte_t> ((d[0] & 0xF0) | ch),
                d.size () > 1 ? static_cast<midi_byte_t> (d[1]) : midi_byte_t{ 0 },
                d.size () > 2 ? static_cast<midi_byte_t> (d[2]) : midi_byte_t{ 0 },
              };
              transform_scratch_.push_back (ev.time (), raw);
            }
          else
            {
              transform_scratch_.push_back (ev.time (), d);
            }
        }
      events.swap (transform_scratch_);
    }
}

PianoRollTrackMixin::PianoRollTrackMixin (QObject * parent) : QObject (parent)
{
  transform_scratch_.reserve (4096);
}

void
init_from (
  PianoRollTrackMixin       &obj,
  const PianoRollTrackMixin &other,
  utils::ObjectCloneType     clone_type)
{
  obj.drum_mode_ = other.drum_mode_;
  obj.midi_ch_ = other.midi_ch_;
  obj.passthrough_midi_input_.store (other.passthrough_midi_input_.load ());
  obj.midi_channels_ = other.midi_channels_;
}

void
to_json (nlohmann::json &j, const PianoRollTrackMixin &track)
{
  j[PianoRollTrackMixin::kDrumModeKey] = track.drum_mode_;
  j[PianoRollTrackMixin::kMidiChKey] = track.midi_ch_;
  j[PianoRollTrackMixin::kPassthroughMidiInputKey] =
    track.passthrough_midi_input_.load ();
  if (track.midi_channels_)
    {
      j[PianoRollTrackMixin::kMidiChannelsKey] = track.midi_channels_;
    }
}

void
from_json (const nlohmann::json &j, PianoRollTrackMixin &track)
{
  j.at (PianoRollTrackMixin::kDrumModeKey).get_to (track.drum_mode_);
  j.at (PianoRollTrackMixin::kMidiChKey).get_to (track.midi_ch_);
  bool passthrough_midi_input{};
  j.at (PianoRollTrackMixin::kPassthroughMidiInputKey)
    .get_to (passthrough_midi_input);
  track.passthrough_midi_input_.store (passthrough_midi_input);
  if (j.contains (PianoRollTrackMixin::kMidiChannelsKey))
    {
      j.at (PianoRollTrackMixin::kMidiChannelsKey).get_to (track.midi_channels_);
    }
}
}
