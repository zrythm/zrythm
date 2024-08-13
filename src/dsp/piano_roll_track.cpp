// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_region.h"
#include "dsp/piano_roll_track.h"
#include "utils/types.h"

void
PianoRollTrack::write_to_midi_file (
  MIDI_FILE *       mf,
  MidiEventVector * events,
  const Position *  start,
  const Position *  end,
  bool              lanes_as_tracks,
  bool              use_track_pos)
{
  int                              midi_track_pos = pos_;
  std::unique_ptr<MidiEventVector> own_events;
  if (!lanes_as_tracks && use_track_pos)
    {
      z_return_if_fail (!events);
      midiTrackAddText (mf, pos_, textTrackName, name_.c_str ());
      own_events = std::make_unique<MidiEventVector> ();
    }

  for (auto &lane : lanes_)
    {
      lane->write_to_midi_file (
        mf, own_events ? own_events.get () : events, start, end,
        lanes_as_tracks, use_track_pos);
    }

  if (own_events)
    {
      own_events->write_to_midi_file (mf, midi_track_pos);
    }
}

void
PianoRollTrack::get_velocities_in_range (
  const Position *         start_pos,
  const Position *         end_pos,
  std::vector<Velocity *> &velocities,
  bool                     inside) const
{
  for (auto &lane : lanes_)
    {
      for (const auto &region : lane->regions_)
        {
          region->get_velocities_in_range (
            start_pos, end_pos, velocities, inside);
        }
    }
}

void
PianoRollTrack::fill_events (
  const EngineProcessTimeInfo &time_nfo,
  MidiEventVector             &midi_events)
{
  fill_events_common (time_nfo, &midi_events, nullptr);
}