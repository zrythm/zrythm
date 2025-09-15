// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
class MockTrack : public Track
{
public:
  using TrackFeatures = Track::TrackFeatures;

  MockTrack (
    Type                  type,
    PortType              in_signal_type,
    PortType              out_signal_type,
    TrackFeatures         features,
    BaseTrackDependencies dependencies)
      : Track (type, in_signal_type, out_signal_type, features, std::move (dependencies))
  {
    processor_ = make_track_processor (std::nullopt, std::nullopt, std::nullopt);
  }

  MOCK_METHOD (
    void,
    collect_additional_timeline_objects,
    (std::vector<ArrangerObjectPtrVariant> & objects),
    (const, override));
};

class MockTrackFactory
{
public:
  MockTrackFactory ()
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);
    plugin_registry_ = std::make_unique<plugins::PluginRegistry> ();
    file_audio_source_registry_ =
      std::make_unique<dsp::FileAudioSourceRegistry> ();
    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();

    base_dependencies_ = std::make_unique<BaseTrackDependencies> (
      *tempo_map_wrapper_, *file_audio_source_registry_, *plugin_registry_,
      *port_registry_, *param_registry_, *obj_registry_, *transport_,
      [] { return false; });
  }

  std::unique_ptr<MockTrack> createMockTrack (
    Track::Type              type,
    dsp::PortType            in_type = dsp::PortType::Audio,
    dsp::PortType            out_type = dsp::PortType::Audio,
    MockTrack::TrackFeatures features =
      MockTrack::TrackFeatures::Automation | MockTrack::TrackFeatures::Lanes
      | MockTrack::TrackFeatures::Modulators
      | MockTrack::TrackFeatures::Recording)
  {
    return std::make_unique<MockTrack> (
      type, in_type, out_type, features, *base_dependencies_);
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<plugins::PluginRegistry>         plugin_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry>    file_audio_source_registry_;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<dsp::TempoMap>                       tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                tempo_map_wrapper_;
  std::unique_ptr<dsp::graph_test::MockTransport>      transport_;
  std::unique_ptr<BaseTrackDependencies>               base_dependencies_;

  sample_rate_t sample_rate_{ 48000 };
};

}
