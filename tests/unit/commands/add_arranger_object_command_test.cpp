// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_arranger_object_command.h"
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

class AddArrangerObjectCommandTest : public ::testing::Test
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
TEST_F (AddArrangerObjectCommandTest, InitialState)
{
  AddArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // Initially, the object should not be in the owner
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);
}

// Test redo operation adds object to owner
TEST_F (AddArrangerObjectCommandTest, RedoAddsObject)
{
  AddArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  command.redo ();

  // After redo, the object should be in the owner
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);
}

// Test undo operation removes object from owner
TEST_F (AddArrangerObjectCommandTest, UndoRemovesObject)
{
  AddArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // First add the object
  command.redo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  // Then undo should remove it
  command.undo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);
}

// Test multiple undo/redo cycles
TEST_F (AddArrangerObjectCommandTest, MultipleUndoRedoCycles)
{
  AddArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // First cycle
  command.redo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  command.undo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);

  // Second cycle
  command.redo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  command.undo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);
}

// Test command text
TEST_F (AddArrangerObjectCommandTest, CommandText)
{
  AddArrangerObjectCommand<structure::arrangement::MidiNote> command (
    *mock_owner, test_object_ref);

  // The command should have the default text "Add Object"
  EXPECT_EQ (command.text (), QString ("Add Object"));
}

// Test with different object types
TEST_F (AddArrangerObjectCommandTest, DifferentObjectTypes)
{
  // Test with Marker
  auto marker_ref =
    object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Custom);
  MockArrangerObjectOwner<structure::arrangement::Marker> marker_owner{
    object_registry, file_audio_source_registry, dummy_qobject
  };

  AddArrangerObjectCommand<structure::arrangement::Marker> marker_command (
    marker_owner, marker_ref);

  marker_command.redo ();
  EXPECT_TRUE (marker_owner.contains (marker_ref.id ()));
  EXPECT_EQ (marker_owner.size (), 1);

  marker_command.undo ();
  EXPECT_FALSE (marker_owner.contains (marker_ref.id ()));
  EXPECT_EQ (marker_owner.size (), 0);

  // Test with AutomationPoint
  auto automation_point_ref = object_registry.create_object<
    structure::arrangement::AutomationPoint> (*tempo_map);
  MockArrangerObjectOwner<structure::arrangement::AutomationPoint>
    automation_point_owner{
      object_registry, file_audio_source_registry, dummy_qobject
    };

  AddArrangerObjectCommand<structure::arrangement::AutomationPoint>
    automation_command (automation_point_owner, automation_point_ref);

  automation_command.redo ();
  EXPECT_TRUE (automation_point_owner.contains (automation_point_ref.id ()));
  EXPECT_EQ (automation_point_owner.size (), 1);

  automation_command.undo ();
  EXPECT_FALSE (automation_point_owner.contains (automation_point_ref.id ()));
  EXPECT_EQ (automation_point_owner.size (), 0);
}

// Test adding multiple objects to same owner
TEST_F (AddArrangerObjectCommandTest, MultipleObjectsSameOwner)
{
  // Create second object
  auto second_note_ref =
    object_registry.create_object<structure::arrangement::MidiNote> (*tempo_map);

  AddArrangerObjectCommand<structure::arrangement::MidiNote> command1 (
    *mock_owner, test_object_ref);
  AddArrangerObjectCommand<structure::arrangement::MidiNote> command2 (
    *mock_owner, second_note_ref);

  // Add first object
  command1.redo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_FALSE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  // Add second object
  command2.redo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_TRUE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 2);

  // Undo second object
  command2.undo ();
  EXPECT_TRUE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_FALSE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 1);

  // Undo first object
  command1.undo ();
  EXPECT_FALSE (mock_owner->contains (test_object_ref.id ()));
  EXPECT_FALSE (mock_owner->contains (second_note_ref.id ()));
  EXPECT_EQ (mock_owner->size (), 0);
}

} // namespace zrythm::commands
