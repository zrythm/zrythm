// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>

#include "structure/tracks/midi_track.h"

namespace zrythm::structure::tracks
{
MidiTrack::MidiTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Midi,
        PortType::Midi,
        PortType::Midi,
        TrackFeatures::Automation | TrackFeatures::Lanes
          | TrackFeatures::PianoRoll | TrackFeatures::Recording,
        dependencies.to_base_dependencies ())
{
  color_ = Color (QColor ("#F79616"));
  icon_name_ = u8"signal-midi";

  processor_ = make_track_processor (
    [this] (
      const dsp::ITransport &transport, const EngineProcessTimeInfo &time_nfo,
      dsp::MidiEventVector *                        midi_events,
      std::optional<TrackProcessor::StereoPortPair> stereo_ports) {
      lanes ()->fill_events_callback (
        transport, time_nfo, midi_events, stereo_ports);
    },
    [this] (dsp::MidiEventVector &events) {
      pianoRollTrackMixin ()->transform_midi_inputs_func (events);
    });
}

void
init_from (
  MidiTrack             &obj,
  const MidiTrack       &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
