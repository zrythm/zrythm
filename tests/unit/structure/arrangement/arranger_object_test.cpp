// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include <QSignalSpy>

#include "helpers/mock_qobject.h"

#include "./arranger_object_test.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
class ArrangerObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    parent = std::make_unique<MockQObject> ();

    // Create objects with proper parenting
    obj = std::make_unique<MockArrangerObject> (
      ArrangerObject::Type::Marker, *tempo_map_wrapper,
      MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());
    obj2 = std::make_unique<MockArrangerObject> (
      ArrangerObject::Type::MidiNote, *tempo_map_wrapper,
      MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<MockQObject>          parent;
  std::unique_ptr<MockArrangerObject>   obj;
  std::unique_ptr<MockArrangerObject>   obj2;
};

// Test initial state
TEST_F (ArrangerObjectTest, InitialState)
{
  EXPECT_EQ (obj->type (), ArrangerObject::Type::Marker);
  EXPECT_EQ (obj->position ()->ticks (), 0);
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

  obj->position ()->setTicks (1920.0);
  EXPECT_DOUBLE_EQ (obj->position ()->ticks (), 1920.0);
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

// Test UUID functionality
TEST_F (ArrangerObjectTest, UuidFunctionality)
{
  // Test uniqueness
  EXPECT_NE (obj->get_uuid (), obj2->get_uuid ());

  // Test persistence through cloning
  const auto         original_uuid = obj->get_uuid ();
  MockArrangerObject temp (
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
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
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
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
  EXPECT_DOUBLE_EQ (obj->position ()->ticks (), -100.0);

  // Large position
  obj->position ()->setTicks (1e9);
  EXPECT_GT (obj->position ()->ticks (), 0.0);
}

// Test subcomponent creation based on feature flags
TEST_F (ArrangerObjectTest, SubcomponentCreation)
{
  // Test with only bounds feature
  auto bounds_only_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
    MockArrangerObject::ArrangerObjectFeatures::Bounds, parent.get ());

  EXPECT_NE (bounds_only_obj->length (), nullptr);
  EXPECT_EQ (bounds_only_obj->name (), nullptr);
  EXPECT_EQ (bounds_only_obj->color (), nullptr);
  EXPECT_EQ (bounds_only_obj->mute (), nullptr);

  // Test with all base features (bounds, name, color, mute, clip-owned)
  auto full_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
    MockArrangerObject::ArrangerObjectFeatures::Bounds
      | MockArrangerObject::ArrangerObjectFeatures::Name
      | MockArrangerObject::ArrangerObjectFeatures::Color
      | MockArrangerObject::ArrangerObjectFeatures::Mute
      | MockArrangerObject::ArrangerObjectFeatures::ClipOwned,
    parent.get ());

  EXPECT_NE (full_obj->length (), nullptr);
  EXPECT_NE (full_obj->name (), nullptr);
  EXPECT_NE (full_obj->color (), nullptr);
  EXPECT_NE (full_obj->mute (), nullptr);
}

// Test signal connections
TEST_F (ArrangerObjectTest, SignalConnections)
{
  // Create object with all features to test signal connections
  auto signal_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
    MockArrangerObject::ArrangerObjectFeatures::Bounds
      | MockArrangerObject::ArrangerObjectFeatures::Name
      | MockArrangerObject::ArrangerObjectFeatures::Color
      | MockArrangerObject::ArrangerObjectFeatures::Mute
      | MockArrangerObject::ArrangerObjectFeatures::ClipOwned,
    parent.get ());

  // Setup signal spy
  QSignalSpy propertiesChangedSpy (
    signal_obj.get (), &MockArrangerObject::propertiesChanged);

  // Test position change signal
  signal_obj->position ()->setTicks (100.0);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
  propertiesChangedSpy.clear ();

  // Test bounds length change signal (may produce > 1 signals due to trackBounds)
  signal_obj->length ()->setTicks (200.0);
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
}

// Test parent object signal emissions
TEST_F (ArrangerObjectTest, ParentObjectSignals)
{
  // Create object with all features to test signal connections
  auto signal_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
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
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
    MockArrangerObject::ArrangerObjectFeatures::Bounds
      | MockArrangerObject::ArrangerObjectFeatures::Name
      | MockArrangerObject::ArrangerObjectFeatures::Color
      | MockArrangerObject::ArrangerObjectFeatures::Mute
      | MockArrangerObject::ArrangerObjectFeatures::ClipOwned,
    parent.get ());

  // Set values on all subcomponents
  full_obj->position ()->setTicks (1920.0);
  full_obj->length ()->setTicks (960.0);
  full_obj->name ()->setName ("Test Object");
  full_obj->color ()->setColor (QColor (128, 64, 32));
  full_obj->mute ()->setMuted (true);

  // Serialize
  nlohmann::json j;
  to_json (j, *full_obj);

  // Create new object from serialized data
  MockArrangerObject new_obj (
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
    MockArrangerObject::ArrangerObjectFeatures::Bounds
      | MockArrangerObject::ArrangerObjectFeatures::Name
      | MockArrangerObject::ArrangerObjectFeatures::Color
      | MockArrangerObject::ArrangerObjectFeatures::Mute
      | MockArrangerObject::ArrangerObjectFeatures::ClipOwned,
    parent.get ());
  from_json (j, new_obj);

  // Verify state
  EXPECT_EQ (new_obj.get_uuid (), full_obj->get_uuid ());
  EXPECT_EQ (new_obj.type (), full_obj->type ());
  EXPECT_DOUBLE_EQ (new_obj.position ()->ticks (), 1920.0);
  EXPECT_DOUBLE_EQ (new_obj.length ()->ticks (), 960.0);
  EXPECT_EQ (new_obj.name ()->name (), "Test Object");
  EXPECT_EQ (new_obj.color ()->color (), QColor (128, 64, 32));
  EXPECT_TRUE (new_obj.mute ()->muted ());
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
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
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
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
    MockArrangerObject::ArrangerObjectFeatures::Bounds
      | MockArrangerObject::ArrangerObjectFeatures::Name
      | MockArrangerObject::ArrangerObjectFeatures::Color
      | MockArrangerObject::ArrangerObjectFeatures::Mute
      | MockArrangerObject::ArrangerObjectFeatures::ClipOwned,
    parent.get ());

  // Set values on all subcomponents
  source_obj->position ()->setTicks (3840.0);
  source_obj->length ()->setTicks (1920.0);
  source_obj->name ()->setName ("Source Object");
  source_obj->color ()->setColor (QColor (64, 128, 192));
  source_obj->mute ()->setMuted (false);

  // Create target object
  auto target_obj = std::make_unique<MockArrangerObject> (
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
    MockArrangerObject::ArrangerObjectFeatures::Bounds
      | MockArrangerObject::ArrangerObjectFeatures::Name
      | MockArrangerObject::ArrangerObjectFeatures::Color
      | MockArrangerObject::ArrangerObjectFeatures::Mute
      | MockArrangerObject::ArrangerObjectFeatures::ClipOwned,
    parent.get ());

  // Copy
  init_from (*target_obj, *source_obj, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target_obj->get_uuid (), source_obj->get_uuid ());
  EXPECT_DOUBLE_EQ (target_obj->position ()->ticks (), 3840.0);
  EXPECT_DOUBLE_EQ (target_obj->length ()->ticks (), 1920.0);
  EXPECT_EQ (target_obj->name ()->name (), "Source Object");
  EXPECT_EQ (target_obj->color ()->color (), QColor (64, 128, 192));
  EXPECT_FALSE (target_obj->mute ()->muted ());
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
    ArrangerObject::Type::Marker, *tempo_map_wrapper,
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
