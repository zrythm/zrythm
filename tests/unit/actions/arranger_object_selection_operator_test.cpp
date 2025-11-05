// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_selection_operator.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/arranger_object_owner.h"

#include "unit/actions/arranger_object_selection_operator_test.h"
#include "unit/actions/mock_undo_stack.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::actions
{

class ArrangerObjectSelectionOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Create test objects
    auto marker_ref =
      object_registry.create_object<structure::arrangement::Marker> (
        *tempo_map, structure::arrangement::Marker::MarkerType::Custom);
    auto note_ref = object_registry.create_object<
      structure::arrangement::MidiNote> (*tempo_map);

    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (marker_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (note_ref);

    // Store original positions
    original_positions_.push_back (
      marker_ref.get_object_base ()->position ()->ticks ());
    original_positions_.push_back (
      note_ref.get_object_base ()->position ()->ticks ());

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create selection model and set up with test objects
    selection_model_ = std::make_unique<QItemSelectionModel> (&list_model_);

    // Create mock owner for testing
    mock_owner_ = std::make_unique<MockArrangerObjectOwner> (
      object_registry,
      // Note: We need a FileAudioSourceRegistry, but for testing purposes
      // we can create a dummy one or use a dependency injection approach
      [] () -> dsp::FileAudioSourceRegistry & {
        static dsp::FileAudioSourceRegistry dummy_registry;
        return dummy_registry;
      }());

    // Add the objects to the mock owner
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::add_object (marker_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::add_object (note_ref);

    // Create mock object owner provider that returns our mock owner
    auto mock_owner_provider =
      [this] (structure::arrangement::ArrangerObjectPtrVariant obj_var)
      -> ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
      return std::visit (
        [&] (auto &&obj)
          -> ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                ObjectT> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::Marker *>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                ObjectT> *> (mock_owner_.get ());
            }
          return static_cast<
            structure::arrangement::ArrangerObjectOwner<ObjectT> *> (nullptr);
        },
        obj_var);
    };

    // Create operator
    operator_ = std::make_unique<ArrangerObjectSelectionOperator> (
      *undo_stack_, *selection_model_, mock_owner_provider);

    // Select all objects
    selection_model_->select (
      list_model_.index (0, 0), QItemSelectionModel::Select);
    selection_model_->select (
      list_model_.index (1, 0), QItemSelectionModel::Select);
  }

  std::unique_ptr<dsp::TempoMap>                               tempo_map;
  structure::arrangement::ArrangerObjectRegistry               object_registry;
  structure::arrangement::ArrangerObjectRefMultiIndexContainer test_objects_;
  std::vector<double>                              original_positions_;
  std::unique_ptr<undo::UndoStack>                 undo_stack_;
  std::unique_ptr<ArrangerObjectSelectionOperator> operator_;
  structure::arrangement::ArrangerObjectListModel  list_model_{ test_objects_ };
  std::unique_ptr<QItemSelectionModel>             selection_model_;
  std::unique_ptr<MockArrangerObjectOwner>         mock_owner_;
};

// Test initial state after construction
TEST_F (ArrangerObjectSelectionOperatorTest, InitialState)
{
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveByTicks with positive delta
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksPositiveDelta)
{
  const double tick_delta = 100.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Objects should be moved by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test moveByTicks with negative delta
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksNegativeDelta)
{
  // Move objects to position 100 first to allow negative movement
  for (const auto &obj_ref : test_objects_)
    {
      if (auto * obj = obj_ref.get_object_base ())
        {
          obj->position ()->setTicks (100.0);
        }
    }

  const double tick_delta = -50.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Objects should be moved backward by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), 50);
        }
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test moveByTicks with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksZeroDelta)
{
  bool result = operator_->moveByTicks (0.0);
  EXPECT_TRUE (result);

  // Objects should remain at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveByTicks with no selection (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  bool result = operator_->moveByTicks (100.0);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveByTicks with invalid movement (before timeline start)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksInvalidMovement)
{
  // Move objects to position 0 first
  for (auto &obj_ref : test_objects_)
    {
      if (auto * obj = obj_ref.get_object_base ())
        {
          obj->position ()->setTicks (0.0);
        }
    }

  const double tick_delta = -50.0; // Would put objects at -50 ticks

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_FALSE (result);

  // Objects should remain at position 0 (no movement)
  for (auto &obj_ref : test_objects_)
    {
      if (auto * obj = obj_ref.get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), 0.0);
        }
    }

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test undo/redo functionality
TEST_F (ArrangerObjectSelectionOperatorTest, UndoRedoFunctionality)
{
  const double tick_delta = 100.0;

  // Move objects
  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects moved
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  // Undo
  undo_stack_->undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }

  // Redo
  undo_stack_->redo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }
}

// Test moveNotesByPitch functionality
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitch)
{
  const int pitch_delta = 5;

  // Store original pitch for MIDI note
  int original_pitch = 0;
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              original_pitch = obj->pitch ();
            }
        },
        obj_ref.get_object ());
    }

  bool result = operator_->moveNotesByPitch (pitch_delta);
  EXPECT_TRUE (result);

  // MIDI notes should be moved by pitch_delta
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch + pitch_delta);
            }
        },
        obj_ref.get_object ());
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Test undo/redo
  undo_stack_->undo ();
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch);
            }
        },
        obj_ref.get_object ());
    }

  undo_stack_->redo ();
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch + pitch_delta);
            }
        },
        obj_ref.get_object ());
    }
}

// Test moveAutomationPointsByDelta functionality
TEST_F (ArrangerObjectSelectionOperatorTest, MoveAutomationPointsByDelta)
{
  // Create an automation point for testing
  auto automation_point_ref = object_registry.create_object<
    structure::arrangement::AutomationPoint> (*tempo_map);

  // Set initial value
  automation_point_ref
    .get_object_as<structure::arrangement::AutomationPoint> ()
    ->setValue (0.5f);

  // Add to test objects and select it
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    automation_point_ref);

  // Update list model and select automation point
  selection_model_->select (
    list_model_.index (2, 0), QItemSelectionModel::Select);

  const double delta = 0.2;

  bool result = operator_->moveAutomationPointsByDelta (delta);
  EXPECT_TRUE (result);

  // Automation point should be moved by delta
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint *>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.7f); // 0.5 + 0.2
            }
        },
        obj_ref.get_object ());
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Test undo/redo
  undo_stack_->undo ();
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint *>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.5f);
            }
        },
        obj_ref.get_object ());
    }

  undo_stack_->redo ();
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint *>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.7f);
            }
        },
        obj_ref.get_object ());
    }
}

// Test moveNotesByPitch with no selection (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  bool result = operator_->moveNotesByPitch (5);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveNotesByPitch with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchZeroDelta)
{
  bool result = operator_->moveNotesByPitch (0);
  EXPECT_TRUE (result);

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveNotesByPitch with invalid pitch (out of range)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchInvalidPitch)
{
  // Find MIDI note and set its pitch to 125 (close to max)
  int original_pitch = 0;
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
            {
              obj->setPitch (125);
              original_pitch = 125;
            }
        },
        obj_ref.get_object ());
    }

  // Try to move by 5 (would result in pitch 130, which is out of range)
  bool result = operator_->moveNotesByPitch (5);
  EXPECT_FALSE (result);

  // Pitch should remain unchanged
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch);
            }
        },
        obj_ref.get_object ());
    }

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveAutomationPointsByDelta with no selection (no-op)
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveAutomationPointsByDeltaNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  bool result = operator_->moveAutomationPointsByDelta (0.1);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveAutomationPointsByDelta with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveAutomationPointsByDeltaZeroDelta)
{
  bool result = operator_->moveAutomationPointsByDelta (0.0);
  EXPECT_TRUE (result);

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveAutomationPointsByDelta with invalid value (out of range)
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveAutomationPointsByDeltaInvalidValue)
{
  // Create an automation point with value 0.9
  auto automation_point_ref = object_registry.create_object<
    structure::arrangement::AutomationPoint> (*tempo_map);

  automation_point_ref
    .get_object_as<structure::arrangement::AutomationPoint> ()
    ->setValue (0.9f);

  // Add to test objects and select it
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    automation_point_ref);

  // Update list model and select automation point
  selection_model_->select (
    list_model_.index (2, 0), QItemSelectionModel::Select);

  // Try to move by 0.2 (would result in value 1.1, which is out of range)
  bool result = operator_->moveAutomationPointsByDelta (0.2);
  EXPECT_FALSE (result);

  // Value should remain unchanged
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.9f);
            }
        },
        obj_ref.get_object ());
    }

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test deleteObjects with valid selection
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsValidSelection)
{
  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Store the UUIDs of objects to be deleted
  std::vector<structure::arrangement::ArrangerObject::Uuid> deleted_object_ids;
  for (const auto &obj_ref : test_objects_)
    {
      deleted_object_ids.push_back (obj_ref.id ());
    }

  // Verify objects exist before deletion
  for (const auto &id : deleted_object_ids)
    {
      // Check if the object exists in either owner
      bool found_in_midi_note_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::MidiNote>::contains_object (id);
      bool found_in_marker_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::Marker>::contains_object (id);
      EXPECT_TRUE (found_in_midi_note_owner || found_in_marker_owner);
    }

  // Delete selected objects
  bool result = operator_->deleteObjects ();
  EXPECT_TRUE (result);

  // Should have created a macro command with individual remove commands
  EXPECT_GT (undo_stack_->count (), initial_count);

  // Verify objects are no longer in their owners
  for (const auto &id : deleted_object_ids)
    {
      // Check if the object exists in either owner
      bool found_in_midi_note_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::MidiNote>::contains_object (id);
      bool found_in_marker_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::Marker>::contains_object (id);
      EXPECT_FALSE (found_in_midi_note_owner && found_in_marker_owner)
        << "Object should have been deleted";
    }
}

// Test deleteObjects with no selection
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Attempt to delete with no selection
  bool result = operator_->deleteObjects ();
  EXPECT_FALSE (result);

  // No commands should be pushed
  EXPECT_EQ (undo_stack_->index (), initial_count);
}

// Test deleteObjects with undeletable objects
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsUndeletableObject)
{ // Create a non-deletable marker (start marker)
  auto start_marker_ref =
    object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Start);

  // Clear existing objects and add only non-deletable marker
  test_objects_.get<structure::arrangement::random_access_index> ().clear ();
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    start_marker_ref);

  // Update selection to only include non-deletable marker
  selection_model_->clear ();
  selection_model_->select (
    list_model_.index (0, 0), QItemSelectionModel::Select);

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Attempt to delete non-deletable object
  bool result = operator_->deleteObjects ();
  EXPECT_FALSE (result);

  // No commands should be pushed for undeletable objects
  EXPECT_EQ (undo_stack_->index (), initial_count);
}

// Test deleteObjects with mixed deletable and undeletable objects
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsMixedObjects)
{ // Create a non-deletable marker (start marker)
  auto start_marker_ref =
    object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Start);

  // Add non-deletable marker to existing objects
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    start_marker_ref);

  // Update selection to include all objects
  selection_model_->select (
    list_model_.index (2, 0), QItemSelectionModel::Select);

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Attempt to delete mixed objects
  bool result = operator_->deleteObjects ();
  EXPECT_FALSE (result);

  // No commands should be pushed when any object is undeletable
  EXPECT_EQ (undo_stack_->index (), initial_count);
}

// Test deleteObjects undo/redo functionality
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsUndoRedo)
{
  // Store the UUIDs of objects to be deleted
  std::vector<structure::arrangement::ArrangerObject::Uuid> deleted_object_ids;
  for (const auto &obj_ref : test_objects_)
    {
      deleted_object_ids.push_back (obj_ref.id ());
    }

  // Verify objects exist before deletion
  for (const auto &id : deleted_object_ids)
    {
      // Check if the object exists in either owner
      bool found_in_midi_note_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::MidiNote>::contains_object (id);
      bool found_in_marker_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::Marker>::contains_object (id);
      EXPECT_TRUE (found_in_midi_note_owner || found_in_marker_owner);
    }

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Delete objects
  bool result = operator_->deleteObjects ();
  EXPECT_TRUE (result);

  const int after_delete_count = undo_stack_->count ();
  EXPECT_GT (after_delete_count, initial_count);

  // Verify objects are deleted
  for (const auto &id : deleted_object_ids)
    {
      // Check if the object exists in either owner
      bool found_in_midi_note_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::MidiNote>::contains_object (id);
      bool found_in_marker_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::Marker>::contains_object (id);
      EXPECT_FALSE (found_in_midi_note_owner && found_in_marker_owner)
        << "Object should have been deleted";
    }

  // Undo the deletion
  undo_stack_->undo ();
  EXPECT_EQ (undo_stack_->index (), after_delete_count - 1);

  // Verify objects are restored after undo
  for (const auto &id : deleted_object_ids)
    {
      // Check if the object exists in either owner
      bool found_in_midi_note_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::MidiNote>::contains_object (id);
      bool found_in_marker_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::Marker>::contains_object (id);
      EXPECT_TRUE (found_in_midi_note_owner || found_in_marker_owner)
        << "Object should have been restored after undo";
    }

  // Redo the deletion
  undo_stack_->redo ();
  EXPECT_EQ (undo_stack_->index (), after_delete_count);

  // Verify objects are deleted again after redo
  for (const auto &id : deleted_object_ids)
    {
      // Check if the object exists in either owner
      bool found_in_midi_note_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::MidiNote>::contains_object (id);
      bool found_in_marker_owner = mock_owner_->ArrangerObjectOwner<
        structure::arrangement::Marker>::contains_object (id);
      EXPECT_FALSE (found_in_midi_note_owner && found_in_marker_owner)
        << "Object should have been deleted again after redo";
    }
}

} // namespace zrythm::actions
