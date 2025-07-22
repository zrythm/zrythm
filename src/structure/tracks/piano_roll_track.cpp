// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/midi_region.h"
#include "structure/tracks/piano_roll_track.h"

#include <midilib/src/midifile.h>
#include <midilib/src/midiinfo.h>

namespace zrythm::structure::tracks
{
PianoRollTrack::PianoRollTrack ()
{
  processor_->set_fill_events_callback (
    [this] (
      const dsp::ITransport &transport, const EngineProcessTimeInfo &time_nfo,
      dsp::MidiEventVector *                        midi_events,
      std::optional<TrackProcessor::StereoPortPair> stereo_ports) {
      for (auto &lane_var : lanes_)
        {
          using TrackLaneT = LanedTrackImpl::TrackLaneType;
          auto * lane = std::get<TrackLaneT *> (lane_var);
          for (const auto * r : lane->get_children_view ())
            {
              TrackProcessor::fill_events_from_region_rt (
                transport, time_nfo, midi_events, stereo_ports, *r);
            }
        }
    });
}

void
PianoRollTrack::write_to_midi_file (
  MIDI_FILE *            mf,
  dsp::MidiEventVector * events,
  const Position *       start,
  const Position *       end,
  bool                   lanes_as_tracks,
  bool                   use_track_pos)
{
  [[maybe_unused]] int                  midi_track_pos = pos_;
  std::unique_ptr<dsp::MidiEventVector> own_events;
  if (!lanes_as_tracks && use_track_pos)
    {
      z_return_if_fail (!events);
      midiTrackAddText (mf, pos_, textTrackName, name_.c_str ());
      own_events = std::make_unique<dsp::MidiEventVector> ();
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
      // TODO
      // own_events->write_to_midi_file (mf, midi_track_pos);
    }
}

void
PianoRollTrack::clear_objects ()
{
  LanedTrackImpl::clear_objects ();
  automationTracklist ()->clear_arranger_objects ();
}

uint8_t
PianoRollTrack::get_midi_ch (const arrangement::MidiRegion &midi_region) const
{
  return 1;
// TODO
#if 0
  uint8_t ret;
  auto   &lane = get_lane_for_mr (midi_region);
  if (lane.midi_ch_ > 0)
    ret = lane.midi_ch_;
  else
    {
      auto * track = lane.get_track ();
      z_return_val_if_fail (track, 1);
      auto piano_roll_track = dynamic_cast<tracks::PianoRollTrack *> (track);
      ret = piano_roll_track->midi_ch_;
    }

  z_return_val_if_fail (ret > 0, 1);

  return ret;
#endif
}

void
PianoRollTrack::get_regions_in_range (
  std::vector<arrangement::ArrangerObjectUuidReference> &regions,
  std::optional<signed_frame_t>                          p1,
  std::optional<signed_frame_t>                          p2)
{
  LanedTrackImpl::get_regions_in_range (regions, p1, p2);
  // TODO
  // AutomatableTrack::get_regions_in_range (regions, p1, p2);
}

void
init_from (
  PianoRollTrack        &obj,
  const PianoRollTrack  &other,
  utils::ObjectCloneType clone_type)
{
  obj.drum_mode_ = other.drum_mode_;
  obj.midi_ch_ = other.midi_ch_;
  obj.passthrough_midi_input_ = other.passthrough_midi_input_;
}

void
PianoRollTrack::set_playback_caches ()
{
  LanedTrackImpl::set_playback_caches ();
  // AutomatableTrack::set_playback_caches ();
}

void
PianoRollTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  RecordableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  LanedTrackImpl::init_loaded (plugin_registry, port_registry, param_registry);
}
}
