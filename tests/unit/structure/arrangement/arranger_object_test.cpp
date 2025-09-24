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
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0 * mp_units::si::hertz);
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
  EXPECT_EQ (obj->parentObject (), nullptr);
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

// Test parent object operations
TEST_F (ArrangerObjectTest, ParentObjectOperations)
{
  // Initially no parent
  EXPECT_EQ (obj->parentObject (), nullptr);
  EXPECT_EQ (obj2->parentObject (), nullptr);

  // Set parent
  obj->setParentObject (obj2.get ());
  EXPECT_EQ (obj->parentObject (), obj2.get ());

  // Set to null parent
  obj->setParentObject (nullptr);
  EXPECT_EQ (obj->parentObject (), nullptr);

  // Set parent again
  obj->setParentObject (obj2.get ());
  EXPECT_EQ (obj->parentObject (), obj2.get ());

  // Setting same parent should be no-op
  QSignalSpy parentChangedSpy (
    obj.get (), &MockArrangerObject::parentObjectChanged);
  obj->setParentObject (obj2.get ());
  EXPECT_EQ (parentChangedSpy.count (), 0); // No signal for same parent
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

// Test time conversion with parent object
TEST_F (ArrangerObjectTest, TimeConversionWithParent)
{
  // Set up parent object at position 1920 ticks
  obj2->position ()->setTicks (1920.0);
  const double parent_ticks = obj2->position ()->ticks ();
  const double parent_seconds = obj2->position ()->seconds ();
  const double parent_samples =
    tempo_map->tick_to_samples (parent_ticks * units::tick)
      .numerical_value_in (units::sample);

  // Set child object with parent
  obj->setParentObject (obj2.get ());

  // Test time conversions relative to parent
  const double relative_ticks = 480.0;
  const double expected_seconds =
    tempo_map->tick_to_seconds ((parent_ticks + relative_ticks) * units::tick)
      .numerical_value_in (mp_units::si::second)
    - parent_seconds;
  const double expected_samples =
    tempo_map->tick_to_samples ((parent_ticks + relative_ticks) * units::tick)
      .numerical_value_in (units::sample)
    - parent_samples;

  // Test tick to seconds conversion
  EXPECT_DOUBLE_EQ (
    obj->position ()->position ().time_conversion_functions ().tick_to_seconds (
      relative_ticks),
    expected_seconds);

  // Test seconds to tick conversion
  EXPECT_DOUBLE_EQ (
    obj->position ()->position ().time_conversion_functions ().seconds_to_tick (
      expected_seconds),
    relative_ticks);

  // Test tick to samples conversion
  EXPECT_DOUBLE_EQ (
    obj->position ()->position ().time_conversion_functions ().tick_to_samples (
      relative_ticks),
    expected_samples);

  // Test samples to tick conversion
  EXPECT_DOUBLE_EQ (
    obj->position ()->position ().time_conversion_functions ().samples_to_tick (
      expected_samples),
    relative_ticks);

  // Remove parent and test global time conversion
  obj->setParentObject (nullptr);

  EXPECT_DOUBLE_EQ (
    obj->position ()->position ().time_conversion_functions ().tick_to_seconds (
      relative_ticks),
    tempo_map->tick_to_seconds (relative_ticks * units::tick)
      .numerical_value_in (mp_units::si::second));

  EXPECT_DOUBLE_EQ (
    obj->position ()->position ().time_conversion_functions ().seconds_to_tick (
      expected_seconds),
    tempo_map->seconds_to_tick (expected_seconds * mp_units::si::second)
      .numerical_value_in (units::tick));
}

// Test edge cases
TEST_F (ArrangerObjectTest, EdgeCases)
{
  // Negative position
  obj->position ()->setTicks (-100.0);
  EXPECT_DOUBLE_EQ (obj->position ()->ticks (), -100.0);

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

// Test parent object signal emissions
TEST_F (ArrangerObjectTest, ParentObjectSignals)
{
  // Create object with all features to test signal connections
  auto signal_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());

  // Setup signal spies
  QSignalSpy parentObjectChangedSpy (
    signal_obj.get (), &MockArrangerObject::parentObjectChanged);
  QSignalSpy propertiesChangedSpy (
    signal_obj.get (), &MockArrangerObject::propertiesChanged);

  // Set parent object - should emit both signals
  signal_obj->setParentObject (obj2.get ());
  EXPECT_EQ (parentObjectChangedSpy.count (), 1);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
  parentObjectChangedSpy.clear ();
  propertiesChangedSpy.clear ();

  // Set to null parent - should emit both signals
  signal_obj->setParentObject (nullptr);
  EXPECT_EQ (parentObjectChangedSpy.count (), 1);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
  parentObjectChangedSpy.clear ();
  propertiesChangedSpy.clear ();

  // Set same parent (no change) - should not emit signals
  signal_obj->setParentObject (nullptr);
  EXPECT_EQ (parentObjectChangedSpy.count (), 0);
  EXPECT_EQ (propertiesChangedSpy.count (), 0);
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

// Test serialization/deserialization with parent object
TEST_F (ArrangerObjectTest, SerializationWithParentObject)
{
  // Set up parent-child relationship
  obj2->position ()->setTicks (1920.0);
  obj->setParentObject (obj2.get ());
  obj->position ()->setTicks (480.0); // Relative to parent

  // Serialize
  nlohmann::json j;
  to_json (j, *obj);

  // Create new object from serialized data
  MockArrangerObject new_obj (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());
  from_json (j, new_obj);

  // Verify state - parent object should not be serialized (runtime only)
  EXPECT_EQ (new_obj.get_uuid (), obj->get_uuid ());
  EXPECT_EQ (new_obj.type (), obj->type ());
  EXPECT_DOUBLE_EQ (new_obj.position ()->ticks (), 480.0);
  EXPECT_EQ (new_obj.parentObject (), nullptr); // Parent should not be restored
                                                // from serialization
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

// Test copying with parent object
TEST_F (ArrangerObjectTest, CopyingWithParentObject)
{
  // Set up parent-child relationship
  obj2->position ()->setTicks (3840.0);
  obj->setParentObject (obj2.get ());
  obj->position ()->setTicks (960.0); // Relative to parent

  // Create target object
  auto target_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map,
    MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());

  // Copy - parent object should not be copied (runtime only)
  init_from (*target_obj, *obj, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target_obj->get_uuid (), obj->get_uuid ());
  EXPECT_DOUBLE_EQ (target_obj->position ()->ticks (), 960.0);
  EXPECT_EQ (
    target_obj->parentObject (), nullptr); // Parent should not be copied
}

} // namespace zrythm::structure::arrangement
