// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/remove_arranger_object_command.h"
#include "structure/arrangement/arranger_object_all.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

// Simple mock ArrangerObjectOwner for testing
template <structure::arrangement::FinalArrangerObjectSubclass ObjectT>
class MockArrangerObjectOwner
    : public structure::arrangement::ArrangerObjectOwner<ObjectT>
{
public:
  MockArrangerObjectOwner (
    structure::arrangement::ArrangerObjectRegistry &registry,
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
    QObject                                        &derived)
      : structure::arrangement::ArrangerObjectOwner<
          ObjectT> (registry, file_audio_source_registry, derived)
  {
  }

  std::string get_field_name_for_serialization (const ObjectT *) const override
  {
    return "test_objects";
  }

  // Helper methods for testing
  bool contains (const structure::arrangement::ArrangerObjectUuid &id) const
  {
    return std::ranges::any_of (
      this->get_children_vector (),
      [&] (const auto &ref) { return ref.id () == id; });
  }

  size_t size () const { return this->get_children_vector ().size (); }
};

class RemoveArrangerObjectCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Create a test MidiNote
    auto note_ref = object_registry.create_object<
      structure::arrangement::MidiNote> (*tempo_map);
    test_object_ref = note_ref;

    // Initialize mock owner with required dependencies
    mock_owner = std::make_unique<
      MockArrangerObjectOwner<structure::arrangement::MidiNote>> (
      object_registry, file_audio_source_registry, dummy_qobject);

    // Add the object to the owner initially so we can test removal
    mock_owner->add_object (test_object_ref);
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  structure::arrangement::ArrangerObjectRegistry object_registry;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;
  structure::arrangement::ArrangerObjectUuidReference test_object_ref{
    object_registry
  };
  QObject dummy_qobject;
  std::unique_ptr<MockArrangerObjectOwner<structure::arrangement::MidiNote>>
    mock_owner;
};

// Test initial state after construction
TEST_F (RemoveArrangerObjectCommandTest, InitialState)
{
  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // Initially, the object should be in the owner (we added it in SetUp)
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);
}

// Test redo operation removes object from owner
TEST_F (RemoveArrangerObjectCommandTest, RedoRemovesObject)
{
  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  command.redo ();

  // After redo, the object should be removed from the owner
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);
}

// Test undo operation adds object back to owner
TEST_F (RemoveArrangerObjectCommandTest, UndoAddsObject)
{
  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // First remove the object
  command.redo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);

  // Then undo should add it back
  command.undo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);
}

// Test multiple undo/redo cycles
TEST_F (RemoveArrangerObjectCommandTest, MultipleUndoRedoCycles)
{
  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // First cycle
  command.redo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);

  command.undo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  // Second cycle
  command.redo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);

  command.undo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);
}

// Test command text
TEST_F (RemoveArrangerObjectCommandTest, CommandText)
{
  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // The command should have the default text "Remove Object"
  EXPECT_EQ (command.text (), QString ("Remove Object"));
}

// Test with different object types
TEST_F (RemoveArrangerObjectCommandTest, DifferentObjectTypes)
{
  // Test with Marker
  auto marker_ref =
    object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Custom);
  MockArrangerObjectOwner<structure::arrangement::Marker> marker_owner{
    object_registry, file_audio_source_registry, dummy_qobject
  };
  marker_owner.add_object (marker_ref);

  RemoveArrangerObjectCommand<structure::arrangement::Marker> marker_command (
    marker_owner, marker_ref);

  marker_command.redo ();
  EXPECT_FALSE (marker_owner.contains (marker_ref.id ()));
  EXPECT_EQ (marker_owner.size (), 0);

  marker_command.undo ();
  EXPECT_TRUE (marker_owner.contains (marker_ref.id ()));
  EXPECT_EQ (marker_owner.size (), 1);

  // Test with AutomationPoint
  auto automation_point_ref = object_registry.create_object<
    structure::arrangement::AutomationPoint> (*tempo_map);
  MockArrangerObjectOwner<structure::arrangement::AutomationPoint>
    automation_point_owner{
      object_registry, file_audio_source_registry, dummy_qobject
    };
  automation_point_owner.add_object (automation_point_ref);

  RemoveArrangerObjectCommand<structure::arrangement::AutomationPoint>
    automation_command (automation_point_owner, automation_point_ref);

  automation_command.redo ();
  EXPECT_FALSE (automation_point_owner.contains (automation_point_ref.id ()));
  EXPECT_EQ (automation_point_owner.size (), 0);

  automation_command.undo ();
  EXPECT_TRUE (automation_point_owner.contains (automation_point_ref.id ()));
  EXPECT_EQ (automation_point_owner.size (), 1);
}

// Test removing multiple objects from same owner
TEST_F (RemoveArrangerObjectCommandTest, MultipleObjectsSameOwner)
{
  // Create second object
  auto second_note_ref =
    object_registry.create_object<structure::arrangement::MidiNote> (*tempo_map);
  mock_owner->add_object (second_note_ref);

  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command1 (
    *mock_owner, test_object_ref);
  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command2 (
    *mock_owner, second_note_ref);

  // Initially both objects should be present
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_TRUE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 2);

  // Remove first object
  command1.redo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_TRUE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  // Remove second object
  command2.redo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_FALSE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);

  // Undo second object
  command2.undo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_TRUE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  // Undo first object
  command1.undo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_TRUE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 2);
}

// Test that removing non-existent object throws exception
TEST_F (RemoveArrangerObjectCommandTest, RemoveNonExistentObjectThrows)
{
  // Create a command for an object that's not in the owner
  auto orphan_ref =
    object_registry.create_object<structure::arrangement::MidiNote> (*tempo_map);

  MockArrangerObjectOwner<structure::arrangement::MidiNote> empty_owner{
    object_registry, file_audio_source_registry, dummy_qobject
  };

  RemoveArrangerObjectCommand<structure::arrangement::MidiNote> command (
    empty_owner, orphan_ref);

  // Redo should throw because the object doesn't exist in the owner
  EXPECT_THROW (command.redo (), std::runtime_error);
}

} // namespace zrythm::commands
