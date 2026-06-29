// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/parameter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/automation_track.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
class AutomationTrackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    registry = std::make_unique<utils::ObjectRegistry> ();

    auto param_id = utils::create_object<dsp::ProcessorParameter> (
      *registry, *registry, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Test Parameter");

    automation_track = std::make_unique<AutomationTrack> (
      *tempo_map_wrapper, *registry, std::move (param_id));
  }

  auto create_automation_clip (int64_t timeline_pos_samples, int64_t length)
  {
    auto clip_ref = utils::create_object<arrangement::AutomationClip> (
      *registry, *tempo_map_wrapper, *registry, nullptr);
    clip_ref.get_object_as<arrangement::AutomationClip> ()->position ()->setTicks (
      tempo_map->samples_to_tick (units::samples (timeline_pos_samples))
        .asDouble ());
    clip_ref.get_object_as<arrangement::AutomationClip> ()->length ()->setTicks (
      tempo_map->samples_to_tick (units::samples (length)).asDouble ());
    return clip_ref;
  }

  void add_automation_point (
    arrangement::AutomationClip * clip,
    double                        value,
    double                        position_ticks)
  {
    auto point_ref = utils::create_object<arrangement::AutomationPoint> (
      *registry, *tempo_map_wrapper);
    point_ref.get_object_as<arrangement::AutomationPoint> ()->setValue (
      static_cast<float> (value));
    point_ref.get_object_as<arrangement::AutomationPoint> ()
      ->position ()
      ->setTicks (position_ticks);
    clip->add_object (point_ref);
  }

  std::unique_ptr<dsp::TempoMap>         tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper>  tempo_map_wrapper;
  std::unique_ptr<utils::ObjectRegistry> registry;
  std::unique_ptr<AutomationTrack>       automation_track;
};

TEST_F (AutomationTrackTest, InitialState)
{
  EXPECT_EQ (automation_track->parameter ()->label (), "Test Parameter");
  EXPECT_EQ (
    automation_track->getAutomationMode (),
    AutomationTrack::AutomationMode::Read);
  EXPECT_EQ (
    automation_track->getRecordMode (),
    AutomationTrack::AutomationRecordMode::Touch);
  EXPECT_FALSE (automation_track->contains_automation ());
}

TEST_F (AutomationTrackTest, AutomationModeChanges)
{
  automation_track->setAutomationMode (AutomationTrack::AutomationMode::Record);
  EXPECT_EQ (
    automation_track->getAutomationMode (),
    AutomationTrack::AutomationMode::Record);

  automation_track->setRecordMode (AutomationTrack::AutomationRecordMode::Latch);
  EXPECT_EQ (
    automation_track->getRecordMode (),
    AutomationTrack::AutomationRecordMode::Latch);

  automation_track->swapRecordMode ();
  EXPECT_EQ (
    automation_track->getRecordMode (),
    AutomationTrack::AutomationRecordMode::Touch);
}

TEST_F (AutomationTrackTest, RegeneratePlaybackCaches)
{
  // Create a clip with automation points
  auto clip = create_automation_clip (100, 200);
  automation_track->add_object (clip);
  auto * clip_ptr = clip.get_object_as<arrangement::AutomationClip> ();

  add_automation_point (clip_ptr, 0.0, 0);
  add_automation_point (
    clip_ptr, 1.0,
    tempo_map->samples_to_tick (units::samples (200)).asDouble ());

  // Test cache regeneration with specific range
  utils::ExpandableTickRange range (std::make_pair (0.0, 500.0));
  automation_track->regeneratePlaybackCaches (range);

  // Verify the automation track still contains the automation
  EXPECT_TRUE (automation_track->contains_automation ());

  // Test with full content range
  automation_track->regeneratePlaybackCaches ({});
  EXPECT_TRUE (automation_track->contains_automation ());
}

TEST_F (AutomationTrackTest, ShouldReadAutomation)
{
  automation_track->setAutomationMode (AutomationTrack::AutomationMode::Off);
  EXPECT_FALSE (automation_track->should_read_automation ());

  automation_track->setAutomationMode (AutomationTrack::AutomationMode::Read);
  EXPECT_TRUE (automation_track->should_read_automation ());

  // TODO
  // automation_track->setAutomationMode
  // (AutomationTrack::AutomationMode::Record); EXPECT_FALSE
  // (automation_track->should_read_automation ());
}

TEST_F (AutomationTrackTest, ShouldBeRecording)
{
  automation_track->setAutomationMode (AutomationTrack::AutomationMode::Record);

  // Latch mode should always return true
  automation_track->setRecordMode (AutomationTrack::AutomationRecordMode::Latch);
  EXPECT_TRUE (automation_track->should_be_recording (true));
  EXPECT_TRUE (automation_track->should_be_recording (false));

  // TODO: Implement touch mode tests once functionality is complete
}

TEST_F (AutomationTrackTest, RegionBeforePosition)
{
  auto clip1 = create_automation_clip (100, 100);
  automation_track->add_object (clip1);

  auto clip2 = create_automation_clip (300, 100);
  automation_track->add_object (clip2);

  // Test with enclosing position
  auto * found_region =
    automation_track->get_clip_before (units::samples (150), true);
  EXPECT_EQ (found_region, clip1.get_object_as<arrangement::AutomationClip> ());

  // Test without requiring enclosing position
  found_region = automation_track->get_clip_before (units::samples (250), false);
  EXPECT_EQ (found_region, clip1.get_object_as<arrangement::AutomationClip> ());

  // Test position after all clips
  found_region = automation_track->get_clip_before (units::samples (500), false);
  EXPECT_EQ (found_region, clip2.get_object_as<arrangement::AutomationClip> ());
}

TEST_F (AutomationTrackTest, AutomationPointBefore)
{
  auto clip = create_automation_clip (100, 200);
  automation_track->add_object (clip);
  auto * clip_ptr = clip.get_object_as<arrangement::AutomationClip> ();

  add_automation_point (clip_ptr, 0.2, 0.0); // Timeline position 100 samples
  add_automation_point (
    clip_ptr, 0.4,
    tempo_map->samples_to_tick (units::samples (100)).asDouble ()); // Timeline
                                                                    // position
                                                                    // 200
                                                                    // samples

  // Find point before 150 frames (should be first point)
  auto * point =
    automation_track->get_automation_point_before (units::samples (150), true);
  EXPECT_EQ (point->position ()->ticks (), 0.0);

  // Test position after all points
  point =
    automation_track->get_automation_point_before (units::samples (250), true);
  EXPECT_EQ (
    point->position ()->ticks (),
    tempo_map->samples_to_tick (units::samples (100)).asDouble ());
}

TEST_F (AutomationTrackTest, AutomationPointAround)
{
  auto clip = create_automation_clip (100, 200);
  automation_track->add_object (clip);
  auto * clip_ptr = clip.get_object_as<arrangement::AutomationClip> ();

  add_automation_point (
    clip_ptr, 0.3, tempo_map->samples_to_tick (units::samples (50)).asDouble ());

  // Find point near 160 ticks within 20-tick radius
  auto * point = automation_track->get_automation_point_around (
    tempo_map->samples_to_tick (units::samples (160)).asQuantity (),
    units::ticks (20), false);
  EXPECT_EQ (
    point->position ()->ticks (),
    tempo_map->samples_to_tick (units::samples (50)).asDouble ());

  // Test with position before any points
  point = automation_track->get_automation_point_around (
    units::ticks (50), units::ticks (20), true);
  EXPECT_EQ (point, nullptr);
}

TEST_F (AutomationTrackTest, NormalizedValueCalculation)
{
  auto clip = create_automation_clip (100, 200);
  automation_track->add_object (clip);
  auto * clip_ptr = clip.get_object_as<arrangement::AutomationClip> ();

  add_automation_point (clip_ptr, 0.0, 0);
  add_automation_point (
    clip_ptr, 1.0,
    tempo_map->samples_to_tick (units::samples (200)).asDouble ());

  // Get value at midpoint (should be 0.5)
  auto value =
    automation_track->get_normalized_value (units::samples (200), true);
  EXPECT_TRUE (value.has_value ());
  EXPECT_FLOAT_EQ (*value, 0.5f);

  // Test position before any clips
  value = automation_track->get_normalized_value (units::samples (50), true);
  EXPECT_FALSE (value.has_value ());
}

TEST_F (AutomationTrackTest, GenerateAutomationTracks)
{
  // Mock ProcessorBase for testing
  class MockProcessor : public dsp::ProcessorBase
  {
  public:
    MockProcessor (utils::IObjectRegistry &registry)
        : ProcessorBase (registry, u8"MockProcessor"), registry_ (registry)
    {
      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"param1"),
          dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
          u8"Param 1"));
      get_parameters ()
        .at (0)
        .get_object_as<dsp::ProcessorParameter> ()
        ->set_automatable (true);

      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"param2"),
          dsp::ParameterRange (dsp::ParameterRange::Type::Linear, -1.0f, 1.0f),
          u8"Param 2"));
      get_parameters ()
        .at (1)
        .get_object_as<dsp::ProcessorParameter> ()
        ->set_automatable (true);

      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"param3"),
          dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 100.0f),
          u8"Param 3"));
      get_parameters ()
        .at (2)
        .get_object_as<dsp::ProcessorParameter> ()
        ->set_automatable (false);
    }

  private:
    utils::IObjectRegistry &registry_;
  };

  MockProcessor                                         processor{ *registry };
  std::vector<utils::QObjectUniquePtr<AutomationTrack>> tracks;
  generate_automation_tracks_for_processor (
    tracks, processor, *tempo_map_wrapper, *registry, nullptr);

  // Should create tracks for automatable parameters only (2 of 3)
  EXPECT_EQ (tracks.size (), 2);

  // Verify parameter names
  EXPECT_EQ (tracks[0]->parameter ()->label (), "Param 1");
  EXPECT_EQ (tracks[1]->parameter ()->label (), "Param 2");
}

TEST_F (AutomationTrackTest, Serialization)
{
  // Create and populate automation track
  auto clip = create_automation_clip (100, 200);
  automation_track->add_object (clip);
  auto * clip_ptr = clip.get_object_as<arrangement::AutomationClip> ();
  add_automation_point (
    clip_ptr, 0.5,
    tempo_map->samples_to_tick (units::samples (150)).asDouble ());

  automation_track->setAutomationMode (AutomationTrack::AutomationMode::Record);
  automation_track->setRecordMode (AutomationTrack::AutomationRecordMode::Latch);

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, *automation_track);

  // Create dummy track with same parameters
  auto dummy_track = std::make_unique<AutomationTrack> (
    *tempo_map_wrapper, *registry,
    dsp::ProcessorParameterUuidReference (
      automation_track->parameter ()->get_uuid (), *registry));

  // Deserialize into dummy track
  from_json (j, *dummy_track);

  // Verify serialization/deserialization
  EXPECT_EQ (
    dummy_track->getAutomationMode (), AutomationTrack::AutomationMode::Record);
  EXPECT_EQ (
    dummy_track->getRecordMode (), AutomationTrack::AutomationRecordMode::Latch);

  // Verify clips and points
  EXPECT_TRUE (dummy_track->contains_automation ());
  const auto &dummy_regions = dummy_track->get_children_vector ();
  EXPECT_EQ (dummy_regions.size (), 1);

  auto * dummy_region =
    dummy_regions[0].get_object_as<arrangement::AutomationClip> ();
  EXPECT_EQ (
    dummy_region->position ()->ticks (),
    tempo_map->samples_to_tick (units::samples (100)).asDouble ());
  EXPECT_EQ (
    dummy_region->length ()->ticks (),
    tempo_map->samples_to_tick (units::samples (200)).asDouble ());

  const auto &dummy_points = dummy_region->get_children_vector ();
  EXPECT_EQ (dummy_points.size (), 1);
  auto * dummy_point =
    dummy_points[0].get_object_as<arrangement::AutomationPoint> ();
  EXPECT_FLOAT_EQ (dummy_point->value (), 0.5f);
  EXPECT_EQ (
    dummy_point->position ()->ticks (),
    tempo_map->samples_to_tick (units::samples (150)).asDouble ());
}

} // namespace zrythm::structure::tracks
