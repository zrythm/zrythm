// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/processor_base.h"
#include "structure/tracks/automatable_track.h"

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
// Mock ProcessorBase for testing
class MockProcessor : public dsp::ProcessorBase
{
public:
  MockProcessor (dsp::ProcessorBase::ProcessorBaseDependencies dependencies)
      : ProcessorBase (dependencies, u8"MockProcessor"),
        param_registry_ (dependencies.param_registry_)
  {
    // Create test parameters
    add_parameter (param_registry_.create_object<dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (u8"param1"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Param 1"));
    get_parameters ()
      .at (0)
      .get_object_as<dsp::ProcessorParameter> ()
      ->set_automatable (true);

    add_parameter (param_registry_.create_object<dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (u8"param2"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, -1.0f, 1.0f),
      u8"Param 2"));
    get_parameters ()
      .at (1)
      .get_object_as<dsp::ProcessorParameter> ()
      ->set_automatable (true);

    // Non-automatable parameter
    add_parameter (param_registry_.create_object<dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (u8"param3"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 100.0f),
      u8"Param 3"));
    get_parameters ()
      .at (2)
      .get_object_as<dsp::ProcessorParameter> ()
      ->set_automatable (false);
  }

private:
  dsp::ProcessorParameterRegistry &param_registry_;
};

class AutomatableTrackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    automatable_track = std::make_unique<AutomatableTrackMixin> (dependencies);
  }

  dsp::TempoMap                       tempo_map{ 44100 };
  dsp::FileAudioSourceRegistry        file_audio_source_registry;
  dsp::PortRegistry                   port_registry;
  dsp::ProcessorParameterRegistry     param_registry{ port_registry };
  arrangement::ArrangerObjectRegistry obj_registry;
  AutomationTrackHolder::Dependencies dependencies{
    .tempo_map_ = tempo_map,
    .file_audio_source_registry_ = file_audio_source_registry,
    .port_registry_ = port_registry,
    .param_registry_ = param_registry,
    .object_registry_ = obj_registry
  };
  std::unique_ptr<AutomatableTrackMixin> automatable_track;
};

TEST_F (AutomatableTrackTest, InitialState)
{
  EXPECT_FALSE (automatable_track->automationVisible ());
  EXPECT_NE (automatable_track->automationTracklist (), nullptr);
  EXPECT_EQ (automatable_track->automationTracklist ()->rowCount (), 0);
}

TEST_F (AutomatableTrackTest, AutomationVisibility)
{
  automatable_track->setAutomationVisible (true);
  EXPECT_TRUE (automatable_track->automationVisible ());

  automatable_track->setAutomationVisible (false);
  EXPECT_FALSE (automatable_track->automationVisible ());
}

TEST_F (AutomatableTrackTest, GenerateAutomationTracks)
{
  MockProcessor processor{
    MockProcessor::ProcessorBaseDependencies{
                                             .port_registry_ = dependencies.port_registry_,
                                             .param_registry_ = dependencies.param_registry_ }
  };
  std::vector<utils::QObjectUniquePtr<AutomationTrack>> tracks;
  automatable_track->generate_automation_tracks_for_processor (
    tracks, processor);

  // Should create tracks for automatable parameters only (2 of 3)
  EXPECT_EQ (tracks.size (), 2);

  // Verify parameter names
  EXPECT_EQ (tracks[0]->parameter ()->label (), "Param 1");
  EXPECT_EQ (tracks[1]->parameter ()->label (), "Param 2");
}

TEST_F (AutomatableTrackTest, Serialization)
{
  // Set initial state
  automatable_track->setAutomationVisible (true);

  // Serialize
  nlohmann::json j;
  to_json (j, *automatable_track);

  // Create new instance
  auto new_track = std::make_unique<AutomatableTrackMixin> (dependencies);
  from_json (j, *new_track);

  // Verify deserialization
  EXPECT_TRUE (new_track->automationVisible ());
  // Can't directly compare tracklists, but should be initialized
  EXPECT_NE (new_track->automationTracklist (), nullptr);
}

TEST_F (AutomatableTrackTest, AutomationVisibilityShowsFirstTrack)
{
  // When setting automation visible and no tracks are visible, should show
  // first track
  automatable_track->setAutomationVisible (true);

  auto * tracklist = automatable_track->automationTracklist ();
  if (tracklist->rowCount () > 0)
    {
      auto * holder =
        tracklist
          ->data (
            tracklist->index (0), AutomationTracklist::AutomationTrackHolderRole)
          .value<AutomationTrackHolder *> ();
      EXPECT_TRUE (holder->visible ());
      EXPECT_TRUE (holder->created_by_user_);
    }
}

} // namespace zrythm::structure::tracks
