// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/dsp/arranger_object_factory.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/instrument_track.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/midi_track.h"
#include "gui/dsp/piano_roll_track.h"
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

  for (auto &lane_var : lanes_)
    {
      auto lane = std::get<MidiLane *> (lane_var);
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
  for (const auto &lane_var : lanes_)
    {
      auto * lane = std::get<MidiLane *> (lane_var);
      for (auto * region : lane->get_children_view ())
        {
          region->get_velocities_in_range (
            start_pos, end_pos, velocities, inside);
        }
    }
}

void
PianoRollTrack::clear_objects ()
{
  LanedTrackImpl::clear_objects ();
  AutomatableTrack::clear_objects ();
}

void
PianoRollTrack::get_regions_in_range (
  std::vector<Region *> &regions,
  const Position *       p1,
  const Position *       p2)
{
  LanedTrackImpl::get_regions_in_range (regions, p1, p2);
  AutomatableTrack::get_regions_in_range (regions, p1, p2);
}

void
PianoRollTrack::copy_members_from (
  const PianoRollTrack &other,
  ObjectCloneType       clone_type)
{
  drum_mode_ = other.drum_mode_;
  midi_ch_ = other.midi_ch_;
  passthrough_midi_input_ = other.passthrough_midi_input_;
}

void
PianoRollTrack::set_playback_caches ()
{
  LanedTrackImpl::set_playback_caches ();
  AutomatableTrack::set_playback_caches ();
}

void
PianoRollTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  RecordableTrack::init_loaded (plugin_registry, port_registry);
  LanedTrackImpl::init_loaded (plugin_registry, port_registry);
}
