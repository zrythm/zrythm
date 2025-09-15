// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include <QSignalSpy>

#include "helpers/mock_qobject.h"

#include "./arranger_object_test.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    parent = std::make_unique<MockQObject> ();

    // Create objects with proper parenting
    obj = std::make_unique<MockArrangerObject> (
      ArrangerObject::Type::Marker, *tempo_map,
      MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());
    obj2 = std::make_unique<MockArrangerObject> (
      ArrangerObject::Type::MidiNote, *tempo_map,
      MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap>      tempo_map;
  std::unique_ptr<MockQObject>        parent;
  std::unique_ptr<MockArrangerObject> obj;
  std::unique_ptr<MockArrangerObject> obj2;
};

// Test initial state
TEST_F (ArrangerObjectTest, InitialState)
{
  EXPECT_EQ (obj->type (), ArrangerObject::Type::Marker);
  EXPECT_EQ (obj->position ()->samples (), 0);
  EXPECT_NE (obj->position (), nullptr);
}

// Test type property
TEST_F (ArrangerObjectTest, TypeProperty)
{
  EXPECT_EQ (obj->type (), ArrangerObject::Type::Marker);
  EXPECT_EQ (obj2->type (), ArrangerObject::Type::MidiNote);
}

// Test position operations
TEST_F (ArrangerObjectTest, PositionOperations)
{
  obj->position ()->setTicks (960.0);
  EXPECT_DOUBLE_EQ (obj->position ()->ticks (), 960.0);

  obj->position ()->setSeconds (1.5);
  EXPECT_DOUBLE_EQ (obj->position ()->seconds (), 1.5);
}

TEST_F (ArrangerObjectTest, IsStartHitByRange)
{
  // Set position to 1000 samples
  obj->position ()->setSamples (1000);

  // Test inclusive start, exclusive end (default)
  EXPECT_TRUE (obj->is_start_hit_by_range (1000, 2000));  // exact start
  EXPECT_TRUE (obj->is_start_hit_by_range (500, 1500));   // within range
  EXPECT_FALSE (obj->is_start_hit_by_range (1001, 2000)); // just after start
  EXPECT_FALSE (obj->is_start_hit_by_range (2000, 3000)); // after range
  EXPECT_FALSE (obj->is_start_hit_by_range (0, 999));     // before range

  // Test exclusive start
  EXPECT_FALSE (
    obj->is_start_hit_by_range (1000, 2000, false)); // exact start (excluded)
  EXPECT_TRUE (
    obj->is_start_hit_by_range (999, 2000, false)); // after exclusive start

  // Test inclusive end
  EXPECT_TRUE (
    obj->is_start_hit_by_range (0, 1000, true, true)); // exact end (included)
  EXPECT_FALSE (
    obj->is_start_hit_by_range (0, 1000, true, false)); // exact end (excluded)

  // Test exact position at both boundaries
  EXPECT_TRUE (obj->is_start_hit_by_range (1000, 1000, true, true)); // included
  EXPECT_FALSE (
    obj->is_start_hit_by_range (1000, 1000, false, false)); // excluded

  // Test negative values (clamped to zero currently)
  obj->position ()->setSamples (-500);
  EXPECT_FALSE (obj->is_start_hit_by_range (-1000, 0)); // within negative range
  EXPECT_TRUE (obj->is_start_hit_by_range (0, 1));      // including 0

  // Test large values
  obj->position ()->setSamples (1e9);
  EXPECT_TRUE (obj->is_start_hit_by_range (
    static_cast<signed_frame_t> (1e9 - 100),
    static_cast<signed_frame_t> (1e9 + 100)));
}

// Test UUID functionality
TEST_F (ArrangerObjectTest, UuidFunctionality)
{
  // Test uniqueness
  EXPECT_NE (obj->get_uuid (), obj2->get_uuid ());

  // Test persistence through cloning
  const auto         original_uuid = obj->get_uuid ();
  MockArrangerObject temp (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());
  init_from (temp, *obj, utils::ObjectCloneType::Snapshot);
  EXPECT_EQ (temp.get_uuid (), original_uuid);
}

// Test serialization/deserialization
TEST_F (ArrangerObjectTest, Serialization)
{
  // Set initial state
  obj->position ()->setTicks (1920.0);

  // Serialize
  nlohmann::json j;
  to_json (j, *obj);

  // Create new object from serialized data
  MockArrangerObject new_obj (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());
  from_json (j, new_obj);

  // Verify state
  EXPECT_EQ (new_obj.get_uuid (), obj->get_uuid ());
  EXPECT_EQ (new_obj.type (), obj->type ());
  EXPECT_DOUBLE_EQ (new_obj.position ()->ticks (), 1920.0);
}

// Test edge cases
TEST_F (ArrangerObjectTest, EdgeCases)
{
  // Negative position
  obj->position ()->setTicks (-100.0);
  EXPECT_GE (obj->position ()->ticks (), 0.0);

  // Large position
  obj->position ()->setTicks (1e9);
  EXPECT_GT (obj->position ()->seconds (), 0.0);
}

// Test subcomponent creation based on feature flags
TEST_F (ArrangerObjectTest, SubcomponentCreation)
{
  // Test with only bounds feature
  auto bounds_only_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());

  EXPECT_NE (bounds_only_obj->bounds (), nullptr);
  EXPECT_EQ (bounds_only_obj->loopRange (), nullptr);
  EXPECT_EQ (bounds_only_obj->name (), nullptr);
  EXPECT_EQ (bounds_only_obj->color (), nullptr);
  EXPECT_EQ (bounds_only_obj->mute (), nullptr);
  EXPECT_EQ (bounds_only_obj->fadeRange (), nullptr);

  // Test with region features (includes bounds, looping, name, color, mute)
  auto region_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Region, parent.get ());

  EXPECT_NE (region_obj->bounds (), nullptr);
  EXPECT_NE (region_obj->loopRange (), nullptr);
  EXPECT_NE (region_obj->name (), nullptr);
  EXPECT_NE (region_obj->color (), nullptr);
  EXPECT_NE (region_obj->mute (), nullptr);
  EXPECT_EQ (region_obj->fadeRange (), nullptr);

  // Test with all features
  auto all_features_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Region
      | MockArrangerObject::ArrangerObjectFeatures::Fading,
    parent.get ());

  EXPECT_NE (all_features_obj->bounds (), nullptr);
  EXPECT_NE (all_features_obj->loopRange (), nullptr);
  EXPECT_NE (all_features_obj->name (), nullptr);
  EXPECT_NE (all_features_obj->color (), nullptr);
  EXPECT_NE (all_features_obj->mute (), nullptr);
  EXPECT_NE (all_features_obj->fadeRange (), nullptr);
}

// Test signal connections
TEST_F (ArrangerObjectTest, SignalConnections)
{
  // Create object with all features to test signal connections
  auto signal_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Region
      | MockArrangerObject::ArrangerObjectFeatures::Fading,
    parent.get ());

  // Setup signal spy
  QSignalSpy propertiesChangedSpy (
    signal_obj.get (), &MockArrangerObject::propertiesChanged);

  // Test position change signal
  signal_obj->position ()->setTicks (100.0);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
  propertiesChangedSpy.clear ();

  // Test bounds length change signal (may produce > 1 signals due to trackLength)
  signal_obj->bounds ()->length ()->setTicks (200.0);
  EXPECT_GE (propertiesChangedSpy.count (), 1);
  propertiesChangedSpy.clear ();

  // Test name change signal
  signal_obj->name ()->setName ("Test Name");
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
  propertiesChangedSpy.clear ();

  // Test color change signal
  signal_obj->color ()->setColor (QColor (255, 0, 0));
  EXPECT_GE (propertiesChangedSpy.count (), 1);
  propertiesChangedSpy.clear ();

  // Test mute change signal
  signal_obj->mute ()->setMuted (true);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
  propertiesChangedSpy.clear ();

  // Test fade properties change signal
  signal_obj->fadeRange ()->startOffset ()->setTicks (50.0);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
}

// Test serialization/deserialization with subcomponents
TEST_F (ArrangerObjectTest, SerializationWithSubcomponents)
{
  // Create object with all features
  auto full_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Region
      | MockArrangerObject::ArrangerObjectFeatures::Fading,
    parent.get ());

  // Set values on all subcomponents
  full_obj->position ()->setTicks (1920.0);
  full_obj->bounds ()->length ()->setTicks (960.0);
  full_obj->loopRange ()->loopStartPosition ()->setTicks (480.0);
  full_obj->name ()->setName ("Test Object");
  full_obj->color ()->setColor (QColor (128, 64, 32));
  full_obj->mute ()->setMuted (true);
  full_obj->fadeRange ()->startOffset ()->setTicks (240.0);

  // Serialize
  nlohmann::json j;
  to_json (j, *full_obj);

  // Create new object from serialized data
  MockArrangerObject new_obj (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Region
      | MockArrangerObject::ArrangerObjectFeatures::Fading,
    parent.get ());
  from_json (j, new_obj);

  // Verify state
  EXPECT_EQ (new_obj.get_uuid (), full_obj->get_uuid ());
  EXPECT_EQ (new_obj.type (), full_obj->type ());
  EXPECT_DOUBLE_EQ (new_obj.position ()->ticks (), 1920.0);
  EXPECT_DOUBLE_EQ (new_obj.bounds ()->length ()->ticks (), 960.0);
  EXPECT_DOUBLE_EQ (new_obj.loopRange ()->loopStartPosition ()->ticks (), 480.0);
  EXPECT_EQ (new_obj.name ()->name (), "Test Object");
  EXPECT_EQ (new_obj.color ()->color (), QColor (128, 64, 32));
  EXPECT_TRUE (new_obj.mute ()->muted ());
  EXPECT_DOUBLE_EQ (new_obj.fadeRange ()->startOffset ()->ticks (), 240.0);
}

// Test copying with subcomponents
TEST_F (ArrangerObjectTest, CopyingWithSubcomponents)
{
  // Create object with all features
  auto source_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Region
      | MockArrangerObject::ArrangerObjectFeatures::Fading,
    parent.get ());

  // Set values on all subcomponents
  source_obj->position ()->setTicks (3840.0);
  source_obj->bounds ()->length ()->setTicks (1920.0);
  source_obj->loopRange ()->loopStartPosition ()->setTicks (960.0);
  source_obj->name ()->setName ("Source Object");
  source_obj->color ()->setColor (QColor (64, 128, 192));
  source_obj->mute ()->setMuted (false);
  source_obj->fadeRange ()->startOffset ()->setTicks (480.0);

  // Create target object
  auto target_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Region
      | MockArrangerObject::ArrangerObjectFeatures::Fading,
    parent.get ());

  // Copy
  init_from (*target_obj, *source_obj, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target_obj->get_uuid (), source_obj->get_uuid ());
  EXPECT_DOUBLE_EQ (target_obj->position ()->ticks (), 3840.0);
  EXPECT_DOUBLE_EQ (target_obj->bounds ()->length ()->ticks (), 1920.0);
  EXPECT_DOUBLE_EQ (
    target_obj->loopRange ()->loopStartPosition ()->ticks (), 960.0);
  EXPECT_EQ (target_obj->name ()->name (), "Source Object");
  EXPECT_EQ (target_obj->color ()->color (), QColor (64, 128, 192));
  EXPECT_FALSE (target_obj->mute ()->muted ());
  EXPECT_DOUBLE_EQ (target_obj->fadeRange ()->startOffset ()->ticks (), 480.0);
}

} // namespace zrythm::structure::arrangement
