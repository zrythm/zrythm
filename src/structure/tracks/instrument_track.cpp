// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/instrument_track.h"

namespace zrythm::structure::tracks
{
InstrumentTrack::InstrumentTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Instrument,
        PortType::Midi,
        PortType::Audio,
        TrackFeatures::Automation | TrackFeatures::Lanes
          | TrackFeatures::PianoRoll | TrackFeatures::Recording,
        dependencies.to_base_dependencies ())
{
  color_ = Color (QColor ("#FF9616"));
  icon_name_ = u8"instrument";

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
  InstrumentTrack       &obj,
  const InstrumentTrack &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
