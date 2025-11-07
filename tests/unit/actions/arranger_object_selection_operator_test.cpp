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
    marker_ref = object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Custom);
    note_ref = object_registry.create_object<structure::arrangement::MidiNote> (
      *tempo_map);
    midi_region_ref =
      object_registry.create_object<structure::arrangement::MidiRegion> (
        *tempo_map, object_registry, file_audio_source_registry);
    audio_region_ref = object_registry.create_object<
      structure::arrangement::AudioRegion> (
      *tempo_map, object_registry, file_audio_source_registry,
      [] () { return true; });

    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (marker_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (note_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (midi_region_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (audio_region_ref);

    // Store original positions and set initial values for testing
    marker_ref.get_object_base ()->position ()->setTicks (0.0);
    note_ref.get_object_base ()->position ()->setTicks (1000.0);
    midi_region_ref.get_object_base ()->position ()->setTicks (2000.0);
    audio_region_ref.get_object_base ()->position ()->setTicks (3000.0);

    // Set initial length for resize tests
    note_ref.get_object_as<structure::arrangement::MidiNote> ()
      ->bounds ()
      ->length ()
      ->setTicks (4000.0);
    midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ()
      ->bounds ()
      ->length ()
      ->setTicks (4000.0);
    midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ()
      ->loopRange ()
      ->clipStartPosition ()
      ->setTicks (500.0);
    midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ()
      ->loopRange ()
      ->loopStartPosition ()
      ->setTicks (1000.0);
    midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ()
      ->loopRange ()
      ->loopEndPosition ()
      ->setTicks (3000.0);
    audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ()
      ->fadeRange ()
      ->endOffset ()
      ->setTicks (300.0);

    original_positions_.push_back (
      marker_ref.get_object_base ()->position ()->ticks ());
    original_positions_.push_back (
      note_ref.get_object_base ()->position ()->ticks ());
    original_positions_.push_back (
      midi_region_ref.get_object_base ()->position ()->ticks ());
    original_positions_.push_back (
      audio_region_ref.get_object_base ()->position ()->ticks ());

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create selection model and set up with test objects
    selection_model_ = std::make_unique<QItemSelectionModel> (&list_model_);

    // Create mock owner for testing
    mock_owner_ = std::make_unique<MockArrangerObjectOwner> (
      object_registry, file_audio_source_registry);

    // Add the objects to the mock owner
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::add_object (marker_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::add_object (note_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiRegion>::add_object (midi_region_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AudioRegion>::add_object (audio_region_ref);

    // Create mock object owner provider that returns our mock owner
    auto mock_owner_provider =
      [this] (structure::arrangement::ArrangerObjectPtrVariant obj_var)
      -> ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
      return std::visit (
        [&] (auto &&obj)
          -> ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::MidiNote> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::Marker>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::Marker> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiRegion>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::MidiRegion> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AudioRegion>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::AudioRegion> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::AutomationPoint> *> (mock_owner_.get ());
            }
          return static_cast<
            structure::arrangement::ArrangerObjectOwner<ObjectT> *> (nullptr);
        },
        obj_var);
    };

    // Create operator
    operator_ = std::make_unique<ArrangerObjectSelectionOperator> (
      *undo_stack_, *selection_model_, mock_owner_provider);

    // Select first two objects (marker and note) for movement tests
    selection_model_->select (
      list_model_.index (0, 0), QItemSelectionModel::Select);
    selection_model_->select (
      list_model_.index (1, 0), QItemSelectionModel::Select);
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;
  structure::arrangement::ArrangerObjectRegistry object_registry;
  structure::arrangement::ArrangerObjectRefMultiIndexContainer test_objects_;
  std::vector<double>                              original_positions_;
  std::unique_ptr<undo::UndoStack>                 undo_stack_;
  std::unique_ptr<ArrangerObjectSelectionOperator> operator_;
  structure::arrangement::ArrangerObjectListModel  list_model_{ test_objects_ };
  std::unique_ptr<QItemSelectionModel>             selection_model_;
  std::unique_ptr<MockArrangerObjectOwner>         mock_owner_;
  structure::arrangement::ArrangerObjectUuidReference note_ref{
    object_registry
  };
  structure::arrangement::ArrangerObjectUuidReference marker_ref{
    object_registry
  };
  structure::arrangement::ArrangerObjectUuidReference audio_region_ref{
    object_registry
  };
  structure::arrangement::ArrangerObjectUuidReference midi_region_ref{
    object_registry
  };
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

  // Only selected objects (marker and note) should be moved by tick_delta
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        note_obj->position ()->ticks (), original_positions_[1] + tick_delta);
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

  // Only selected objects (marker and note) should be moved backward by
  // tick_delta Check marker (index 0)
  if (auto * marker_obj = marker_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (marker_obj->position ()->ticks (), 50);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), 50);
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test moveByTicks with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksZeroDelta)
{
  bool result = operator_->moveByTicks (0.0);
  EXPECT_TRUE (result);

  // Only selected objects (marker and note) should remain at original positions
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0]);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), original_positions_[1]);
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

  // Only selected objects (marker and note) should remain at position 0 (no
  // movement) Check marker (index 0)
  if (auto * marker_obj = marker_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (marker_obj->position ()->ticks (), 0.0);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), 0.0);
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

  // Verify only selected objects moved
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        note_obj->position ()->ticks (), original_positions_[1] + tick_delta);
    }

  // Undo
  undo_stack_->undo ();
  // Verify only selected objects restored
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0]);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), original_positions_[1]);
    }

  // Redo
  undo_stack_->redo ();
  // Verify only selected objects moved again
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get_object_base ())
    {
      EXPECT_DOUBLE_EQ (
        note_obj->position ()->ticks (), original_positions_[1] + tick_delta);
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

  // Add to mock owner as well
  mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::AutomationPoint>::add_object (automation_point_ref);

  // Update list model and select automation point
  selection_model_->select (
    list_model_.index (4, 0), QItemSelectionModel::Select);

  const double delta = 0.2;

  bool result = operator_->moveAutomationPointsByDelta (delta);
  EXPECT_TRUE (result);

  // Only selected automation point should be moved by delta
  if (
    auto * auto_obj =
      automation_point_ref
        .get_object_as<structure::arrangement::AutomationPoint> ())
    {
      EXPECT_FLOAT_EQ (auto_obj->value (), 0.7f); // 0.5 + 0.2
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Test undo/redo
  undo_stack_->undo ();

  const auto * auto_obj = automation_point_ref.get_object_as<
    structure::arrangement::AutomationPoint> ();
  EXPECT_FLOAT_EQ (auto_obj->value (), 0.5f);

  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (auto_obj->value (), 0.7f);
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

  // Add to mock owner as well
  mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::AutomationPoint>::add_object (automation_point_ref);

  // Update list model and select automation point
  selection_model_->select (
    list_model_.index (4, 0), QItemSelectionModel::Select);

  // Try to move by 0.2 (would result in value 1.1, which is out of range)
  bool result = operator_->moveAutomationPointsByDelta (0.2);
  EXPECT_FALSE (result);

  // Value should remain unchanged
  const auto * auto_obj = automation_point_ref.get_object_as<
    structure::arrangement::AutomationPoint> ();
  EXPECT_FLOAT_EQ (auto_obj->value (), 0.9f);

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test deleteObjects with valid selection
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsValidSelection)
{
  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Store the UUIDs of the selected objects (marker and note)
  const auto marker_id = marker_ref.id ();
  const auto note_id = note_ref.id ();

  // Verify objects exist before deletion
  bool marker_found_before =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_before =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_TRUE (marker_found_before);
  EXPECT_TRUE (note_found_before);

  // Delete selected objects
  bool result = operator_->deleteObjects ();
  EXPECT_TRUE (result);

  // Should have created a macro command with individual remove commands
  EXPECT_GT (undo_stack_->count (), initial_count);

  // Verify objects are no longer in their owners
  bool marker_found_after =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_after =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_FALSE (marker_found_after) << "Marker should have been deleted";
  EXPECT_FALSE (note_found_after) << "Note should have been deleted";
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
{
  // Create a non-deletable marker (start marker)
  auto start_marker_ref =
    object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Start);

  // Add non-deletable marker to existing objects and to mock owner
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    start_marker_ref);
  mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::Marker>::add_object (start_marker_ref);

  // Update selection to include all objects
  selection_model_->select (
    list_model_.index (4, 0), QItemSelectionModel::Select);

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
  // Store the UUIDs of the selected objects (marker and note)
  const auto marker_id = marker_ref.id ();
  const auto note_id = note_ref.id ();

  // Verify objects exist before deletion
  bool marker_found_before =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_before =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_TRUE (marker_found_before);
  EXPECT_TRUE (note_found_before);

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Delete objects
  bool result = operator_->deleteObjects ();
  EXPECT_TRUE (result);

  const int after_delete_count = undo_stack_->count ();
  EXPECT_GT (after_delete_count, initial_count);

  // Verify objects are deleted
  bool marker_found_after_delete =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_after_delete =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_FALSE (marker_found_after_delete) << "Marker should have been deleted";
  EXPECT_FALSE (note_found_after_delete) << "Note should have been deleted";

  // Undo the deletion
  undo_stack_->undo ();
  EXPECT_EQ (undo_stack_->index (), after_delete_count - 1);

  // Verify objects are restored after undo
  bool marker_found_after_undo =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_after_undo =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_TRUE (marker_found_after_undo)
    << "Marker should have been restored after undo";
  EXPECT_TRUE (note_found_after_undo)
    << "Note should have been restored after undo";

  // Redo the deletion
  undo_stack_->redo ();
  EXPECT_EQ (undo_stack_->index (), after_delete_count);

  // Verify objects are deleted again after redo
  bool marker_found_after_redo =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_after_redo =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_FALSE (marker_found_after_redo)
    << "Marker should have been deleted again after redo";
  EXPECT_FALSE (note_found_after_redo)
    << "Note should have been deleted again after redo";
}

// Test resizeObjects with bounds resize from end
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsBoundsFromEnd)
{
  const double delta = 500.0;

  // Clear selection and only select objects that support bounds resize
  selection_model_->clear ();
  selection_model_->select (
    list_model_.index (1, 0),
    QItemSelectionModel::Select); // Select only MidiNote

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects were resized by checking the object directly
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  EXPECT_DOUBLE_EQ (
    note_obj->bounds ()->length ()->ticks (), 4500.0); // 4000 + 500
}

// Test resizeObjects with bounds resize from start
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsBoundsFromStart)
{
  const double delta = -200.0;

  // Clear selection and only select objects that support bounds resize
  selection_model_->clear ();
  selection_model_->select (
    list_model_.index (1, 0),
    QItemSelectionModel::Select); // Select only MidiNote

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromStart, delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects were resized by checking the object directly
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), 800.0); // 1000 - 200
  EXPECT_DOUBLE_EQ (
    note_obj->bounds ()->length ()->ticks (), 4200.0); // 4000 + 200
}

// Test resizeObjects with loop points resize
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsLoopPoints)
{
  const double delta = 100.0;

  // Clear selection and only select objects that support loop points resize
  selection_model_->clear ();
  selection_model_->select (
    list_model_.index (2, 0),
    QItemSelectionModel::Select); // Select only MidiRegion

  bool result = operator_->resizeObjects (
    commands::ResizeType::LoopPoints, commands::ResizeDirection::FromStart,
    delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects were resized by checking the object directly
  auto * midi_region_obj =
    midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ();
  EXPECT_DOUBLE_EQ (
    midi_region_obj->loopRange ()->clipStartPosition ()->ticks (),
    600.0); // 500 + 100
  EXPECT_DOUBLE_EQ (
    midi_region_obj->loopRange ()->loopStartPosition ()->ticks (),
    1100.0); // 1000 + 100
}

// Test resizeObjects with fades resize
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsFades)
{
  const double delta = 50.0;

  // Clear selection and only select objects that support fades resize
  selection_model_->clear ();
  selection_model_->select (
    list_model_.index (3, 0),
    QItemSelectionModel::Select); // Select only AudioRegion

  bool result = operator_->resizeObjects (
    commands::ResizeType::Fades, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects were resized by checking the object directly
  auto * audio_region_obj =
    audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ();
  EXPECT_DOUBLE_EQ (
    audio_region_obj->fadeRange ()->endOffset ()->ticks (), 350.0); // 300 + 50
}

// Test resizeObjects with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsZeroDelta)
{
  const double delta = 0.0;

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test resizeObjects with no selection
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  const double delta = 100.0;

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromEnd, delta);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test resizeObjects undo/redo functionality
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsUndoRedo)
{
  const double delta = 300.0;

  // Clear selection and only select objects that support bounds resize
  selection_model_->clear ();
  selection_model_->select (
    list_model_.index (1, 0),
    QItemSelectionModel::Select); // Select only MidiNote

  // Perform resize
  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify resize by checking the object directly
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  EXPECT_DOUBLE_EQ (
    note_obj->bounds ()->length ()->ticks (), 4300.0); // 4000 + 300

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (undo_stack_->index (), 0);

  // Verify undo
  EXPECT_DOUBLE_EQ (
    note_obj->bounds ()->length ()->ticks (), 4000.0); // Original restored

  // Redo
  undo_stack_->redo ();
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify redo
  EXPECT_DOUBLE_EQ (
    note_obj->bounds ()->length ()->ticks (), 4300.0); // Applied again
}

// Test that non-timeline objects (MIDI notes) can be resized to negative local
// positions
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  ResizeObjectsNonTimelineObjectNegativePosition)
{
  // Clear selection and only select MIDI note (non-timeline object)
  selection_model_->clear ();
  selection_model_->select (
    list_model_.index (1, 0),
    QItemSelectionModel::Select); // Select only MidiNote

  // Set MIDI note position to a small positive value first
  if (auto * note_obj = note_ref.get_object_base ())
    {
      note_obj->position ()->setTicks (10.0);
    }

  const double delta = -20.0; // Would put MIDI note start position at -10 ticks

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromStart, delta);

  // This should succeed for non-timeline objects (MIDI notes)
  // The test will currently fail, but we expect it to pass after implementation
  // fix
  EXPECT_TRUE (result)
    << "Non-timeline objects (MIDI notes) should be allowed to have negative local positions when resized from start";

  // Check that MIDI note was resized to negative position
  if (
    auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), -10.0); // 10 - 20
      EXPECT_DOUBLE_EQ (
        note_obj->bounds ()->length ()->ticks (), 4020.0); // 4000 + 20
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);
}

} // namespace zrythm::actions
