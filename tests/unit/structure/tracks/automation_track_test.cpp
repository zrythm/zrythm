// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/parameter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/automation_track.h"

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
class AutomationTrackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);

    // Create processor parameter registry and add a test parameter
    param_id = processor_param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Test Parameter");

    // Create automation track
    automation_track = std::make_unique<AutomationTrack> (
      *tempo_map, file_audio_source_registry, obj_registry,
      std::move (param_id));
  }

  auto create_automation_region (
    signed_frame_t timeline_pos_samples,
    signed_frame_t length)
  {
    auto region_ref = obj_registry.create_object<arrangement::AutomationRegion> (
      *tempo_map, obj_registry, file_audio_source_registry, nullptr);
    region_ref.get_object_as<arrangement::AutomationRegion> ()
      ->position ()
      ->setSamples (static_cast<double> (timeline_pos_samples));
    region_ref.get_object_as<arrangement::AutomationRegion> ()
      ->regionMixin ()
      ->bounds ()
      ->length ()
      ->setSamples (static_cast<double> (length));
    return region_ref;
  }

  void add_automation_point (
    arrangement::AutomationRegion * region,
    double                          value,
    double                          position_ticks)
  {
    auto point_ref =
      obj_registry.create_object<arrangement::AutomationPoint> (*tempo_map);
    point_ref.get_object_as<arrangement::AutomationPoint> ()->setValue (
      static_cast<float> (value));
    point_ref.get_object_as<arrangement::AutomationPoint> ()
      ->position ()
      ->setTicks (position_ticks);
    region->add_object (point_ref);
  }

  std::unique_ptr<dsp::TempoMap>  tempo_map;
  dsp::PortRegistry               port_registry;
  dsp::ProcessorParameterRegistry processor_param_registry{ port_registry };
  dsp::ProcessorParameterUuidReference param_id{ processor_param_registry };
  arrangement::ArrangerObjectRegistry  obj_registry;
  dsp::FileAudioSourceRegistry         file_audio_source_registry;
  std::unique_ptr<AutomationTrack>     automation_track;
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
  auto region1 = create_automation_region (100, 100);
  automation_track->add_object (region1);

  auto region2 = create_automation_region (300, 100);
  automation_track->add_object (region2);

  // Test with enclosing position
  auto * found_region = automation_track->get_region_before (150, true);
  EXPECT_EQ (
    found_region, region1.get_object_as<arrangement::AutomationRegion> ());

  // Test without requiring enclosing position
  found_region = automation_track->get_region_before (250, false);
  EXPECT_EQ (
    found_region, region1.get_object_as<arrangement::AutomationRegion> ());

  // Test position after all regions
  found_region = automation_track->get_region_before (500, false);
  EXPECT_EQ (
    found_region, region2.get_object_as<arrangement::AutomationRegion> ());
}

TEST_F (AutomationTrackTest, AutomationPointBefore)
{
  auto region = create_automation_region (100, 200);
  automation_track->add_object (region);
  auto * region_ptr = region.get_object_as<arrangement::AutomationRegion> ();

  add_automation_point (region_ptr, 0.2, 0.0); // Timeline position 100 samples
  add_automation_point (
    region_ptr, 0.4,
    tempo_map->samples_to_tick (100)); // Timeline position 200 samples

  // Find point before 150 frames (should be first point)
  auto * point = automation_track->get_automation_point_before (150, true);
  EXPECT_EQ (point->position ()->samples (), 0);

  // Test position after all points
  point = automation_track->get_automation_point_before (250, true);
  EXPECT_EQ (point->position ()->samples (), 100);
}

TEST_F (AutomationTrackTest, AutomationPointAround)
{
  auto region = create_automation_region (100, 200);
  automation_track->add_object (region);
  auto * region_ptr = region.get_object_as<arrangement::AutomationRegion> ();

  add_automation_point (region_ptr, 0.3, tempo_map->samples_to_tick (50));

  // Find point near 160 ticks within 20-tick radius
  auto * point = automation_track->get_automation_point_around (
    tempo_map->samples_to_tick (160), 20, false);
  EXPECT_EQ (point->position ()->samples (), 50);

  // Test with position before any points
  point = automation_track->get_automation_point_around (50, 20, true);
  EXPECT_EQ (point, nullptr);
}

TEST_F (AutomationTrackTest, NormalizedValueCalculation)
{
  auto region = create_automation_region (100, 200);
  automation_track->add_object (region);
  auto * region_ptr = region.get_object_as<arrangement::AutomationRegion> ();

  add_automation_point (region_ptr, 0.0, 0);
  add_automation_point (region_ptr, 1.0, tempo_map->samples_to_tick (200));

  // Get value at midpoint (should be 0.5)
  auto value = automation_track->get_normalized_value (200, true);
  EXPECT_TRUE (value.has_value ());
  EXPECT_FLOAT_EQ (*value, 0.5f);

  // Test position before any regions
  value = automation_track->get_normalized_value (50, true);
  EXPECT_FALSE (value.has_value ());
}

TEST_F (AutomationTrackTest, Serialization)
{
  // Create and populate automation track
  auto region = create_automation_region (100, 200);
  automation_track->add_object (region);
  auto * region_ptr = region.get_object_as<arrangement::AutomationRegion> ();
  add_automation_point (region_ptr, 0.5, tempo_map->samples_to_tick (150));

  automation_track->setAutomationMode (AutomationTrack::AutomationMode::Record);
  automation_track->setRecordMode (AutomationTrack::AutomationRecordMode::Latch);

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, *automation_track);

  // Create dummy track with same parameters
  auto dummy_track = std::make_unique<AutomationTrack> (
    *tempo_map, file_audio_source_registry, obj_registry,
    dsp::ProcessorParameterUuidReference (
      automation_track->parameter ()->get_uuid (), processor_param_registry));

  // Deserialize into dummy track
  from_json (j, *dummy_track);

  // Verify serialization/deserialization
  EXPECT_EQ (
    dummy_track->getAutomationMode (), AutomationTrack::AutomationMode::Record);
  EXPECT_EQ (
    dummy_track->getRecordMode (), AutomationTrack::AutomationRecordMode::Latch);

  // Verify regions and points
  EXPECT_TRUE (dummy_track->contains_automation ());
  auto dummy_regions = dummy_track->get_children_vector ();
  EXPECT_EQ (dummy_regions.size (), 1);

  auto * dummy_region =
    std::get<arrangement::AutomationRegion *> (dummy_regions[0].get_object ());
  EXPECT_EQ (dummy_region->position ()->samples (), 100);
  EXPECT_EQ (
    dummy_region->regionMixin ()->bounds ()->length ()->samples (), 200);

  auto dummy_points = dummy_region->get_children_vector ();
  EXPECT_EQ (dummy_points.size (), 1);
  auto * dummy_point =
    std::get<arrangement::AutomationPoint *> (dummy_points[0].get_object ());
  EXPECT_FLOAT_EQ (dummy_point->value (), 0.5f);
  EXPECT_EQ (
    dummy_point->position ()->ticks (), tempo_map->samples_to_tick (150));
}

} // namespace zrythm::structure::tracks
