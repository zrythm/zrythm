// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/piano_roll_track.h"

namespace zrythm::structure::tracks
{
PianoRollTrackMixin::PianoRollTrackMixin (QObject * parent) : QObject (parent)
{
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
