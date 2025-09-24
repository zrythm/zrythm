// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class AutomationRegionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0 * mp_units::si::hertz);
    region = std::make_unique<AutomationRegion> (
      *tempo_map, registry, file_audio_source_registry, nullptr);

    // Set up region properties
    region->position ()->setTicks (100);
    region->bounds ()->length ()->setTicks (200);
  }

  auto create_automation_point (double value)
  {
    // Create AutomationPoint using registry
    auto point_ref =
      registry.create_object<AutomationPoint> (*tempo_map, region.get ());
    point_ref.get_object_as<AutomationPoint> ()->setValue (
      static_cast<float> (value));
    return point_ref;
  }

  void add_automation_point (double value, double position_ticks)
  {
    auto point_ref = create_automation_point (value);
    point_ref.get_object_as<AutomationPoint> ()->position ()->setTicks (
      position_ticks);
    region->add_object (point_ref);
  }

  std::unique_ptr<dsp::TempoMap>    tempo_map;
  ArrangerObjectRegistry            registry;
  dsp::FileAudioSourceRegistry      file_audio_source_registry;
  std::unique_ptr<AutomationRegion> region;
};

TEST_F (AutomationRegionTest, InitialState)
{
  EXPECT_EQ (region->type (), ArrangerObject::Type::AutomationRegion);
  EXPECT_EQ (region->position ()->ticks (), 100);
  EXPECT_NE (region->bounds (), nullptr);
  EXPECT_NE (region->loopRange (), nullptr);
  EXPECT_NE (region->name (), nullptr);
  EXPECT_NE (region->color (), nullptr);
  EXPECT_NE (region->mute (), nullptr);
  EXPECT_EQ (region->get_children_vector ().size (), 0);
}

TEST_F (AutomationRegionTest, AddAutomationPoint)
{
  // Add an automation point
  add_automation_point (0.5, 100);

  // Verify point was added
  EXPECT_EQ (region->get_children_vector ().size (), 1);
  auto point = region->get_children_view ()[0];
  EXPECT_FLOAT_EQ (point->value (), 0.5f);
  EXPECT_EQ (point->position ()->ticks (), 100);
}

TEST_F (AutomationRegionTest, RemoveAutomationPoint)
{
  // Add an automation point
  auto point_ref = create_automation_point (0.8);
  region->add_object (point_ref);

  // Remove point
  auto removed_ref = region->remove_object (point_ref.id ());

  // Verify point was removed
  EXPECT_EQ (region->get_children_vector ().size (), 0);
  EXPECT_EQ (removed_ref.id (), point_ref.id ());
}

TEST_F (AutomationRegionTest, ValueChange)
{
  // Add an automation point
  add_automation_point (0.3, 150);

  // Change value
  region->get_children_view ()[0]->setValue (0.7f);

  // Verify change
  EXPECT_FLOAT_EQ (region->get_children_view ()[0]->value (), 0.7f);
}

TEST_F (AutomationRegionTest, GetPrevNextPoint)
{
  // Add points
  add_automation_point (0.2, 100);
  add_automation_point (0.4, 200);
  add_automation_point (0.6, 300);

  // Get next point
  auto point1 = region->get_children_view ()[0];
  auto next_point = region->get_next_ap (*point1, true);
  EXPECT_EQ (next_point->position ()->ticks (), 200);

  // Get previous point (should be null for first point)
  auto prev_point = region->get_prev_ap (*point1);
  EXPECT_EQ (prev_point, nullptr);

  // Get previous point for middle point
  auto point2 = region->get_children_view ()[1];
  prev_point = region->get_prev_ap (*point2);
  EXPECT_EQ (prev_point->position ()->ticks (), 100);
}

TEST_F (AutomationRegionTest, CurveCalculation)
{
  // Add points
  add_automation_point (0.0, 100);
  add_automation_point (1.0, 200);

  // Get curve value at midpoint
  auto   point1 = region->get_children_view ()[0];
  double value = region->get_normalized_value_in_curve (*point1, 0.5);

  // Default curve is linear, so midpoint should be 0.5
  EXPECT_DOUBLE_EQ (value, 0.5);
}

TEST_F (AutomationRegionTest, CurvesUp)
{
  // Add points
  add_automation_point (0.0, 100);
  add_automation_point (1.0, 200);

  // Verify curve direction
  auto point1 = region->get_children_view ()[0];
  EXPECT_TRUE (region->curves_up (*point1));

  // Add descending points
  add_automation_point (0.0, 300);
  auto point3 = region->get_children_view ()[2];
  EXPECT_FALSE (region->curves_up (*point3));
}

TEST_F (AutomationRegionTest, Serialization)
{
  // Add an automation point
  add_automation_point (0.75, 150);
  const auto point_id = region->get_children_view ()[0]->get_uuid ();

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region = std::make_unique<AutomationRegion> (
    *tempo_map, registry, file_audio_source_registry, nullptr);
  from_json (j, *new_region);

  // Verify deserialization
  EXPECT_EQ (new_region->get_children_vector ().size (), 1);
  auto point = new_region->get_children_view ()[0];
  EXPECT_FLOAT_EQ (point->value (), 0.75f);
  EXPECT_EQ (point->position ()->ticks (), 150);
  EXPECT_EQ (point->get_uuid (), point_id);
}

TEST_F (AutomationRegionTest, ObjectCloning)
{
  // Add an automation point
  add_automation_point (0.25, 250);
  const auto original_id = region->get_children_view ()[0]->get_uuid ();

  // Clone region with new identities
  auto cloned_region = std::make_unique<AutomationRegion> (
    *tempo_map, registry, file_audio_source_registry, nullptr);
  init_from (*cloned_region, *region, utils::ObjectCloneType::NewIdentity);

  // Verify cloning
  EXPECT_EQ (cloned_region->get_children_vector ().size (), 1);
  auto cloned_point = cloned_region->get_children_view ()[0];
  EXPECT_NE (cloned_point->get_uuid (), original_id);
  EXPECT_FLOAT_EQ (cloned_point->value (), 0.25f);
  EXPECT_EQ (cloned_point->position ()->ticks (), 250);
}

} // namespace zrythm::structure::arrangement
