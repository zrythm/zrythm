// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_selection_operator.h"
#include "commands/move_arranger_objects_command.h"
#include "commands/remove_arranger_object_command.h"
#include "dsp/content_time_warp.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/variant_helpers.h"

#include "helpers/in_memory_settings_backend.h"

#include "unit/actions/arranger_object_selection_operator_test.h"
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
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    sample_rate_provider = [] () { return units::sample_rate (44100); };
    bpm_provider = [] () { return units::bpm (120.0); };

    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper,
        .registry_ = registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      sample_rate_provider, bpm_provider);

    marker_ref = utils::create_object<structure::arrangement::Marker> (
      registry_, *tempo_map_wrapper,
      structure::arrangement::Marker::MarkerType::Custom);
    note_ref = utils::create_object<structure::arrangement::MidiNote> (
      registry_, *tempo_map_wrapper);
    midi_clip_ref = utils::create_object<structure::arrangement::MidiClip> (
      registry_, *tempo_map_wrapper, registry_);
    audio_clip_ref = utils::create_object<structure::arrangement::AudioClip> (
      registry_, *tempo_map_wrapper, registry_);
    // Not added to test_objects_ (kept out of the selection list model);
    // only used for direct (selection-independent) operations
    automation_clip_ref =
      utils::create_object<structure::arrangement::AutomationClip> (
        registry_, *tempo_map_wrapper, registry_);
    tempo_ref = utils::create_object<structure::arrangement::TempoObject> (
      registry_, *tempo_map_wrapper);
    time_signature_ref =
      utils::create_object<structure::arrangement::TimeSignatureObject> (
        registry_, *tempo_map_wrapper);

    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (marker_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (note_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (midi_clip_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (audio_clip_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (tempo_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (time_signature_ref);

    // Store original positions and set initial values for testing
    marker_ref.get ()->position ()->setTicks (0.0);
    note_ref.get ()->position ()->setTicks (1000.0);
    midi_clip_ref.get ()->position ()->setTicks (2000.0);
    audio_clip_ref.get ()->position ()->setTicks (3000.0);
    tempo_ref.get ()->position ()->setTicks (4000.0);
    time_signature_ref.get ()->position ()->setTicks (5000.0);

    // Set initial length for resize tests
    note_ref.get_object_as<structure::arrangement::MidiNote> ()
      ->length ()
      ->setTicks (4000.0);
    midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
      ->length ()
      ->setTicks (4000.0);
    midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
      ->clipStartPosition ()
      ->setTicks (500.0);
    midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
      ->loopStartPosition ()
      ->setTicks (1000.0);
    midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
      ->loopEndPosition ()
      ->setTicks (3000.0);
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ()
      ->fadeRange ()
      ->endOffset ()
      ->setTicks (300.0);

    original_positions_.push_back (marker_ref.get ()->position ()->ticks ());
    original_positions_.push_back (note_ref.get ()->position ()->ticks ());
    original_positions_.push_back (midi_clip_ref.get ()->position ()->ticks ());
    original_positions_.push_back (audio_clip_ref.get ()->position ()->ticks ());
    original_positions_.push_back (tempo_ref.get ()->position ()->ticks ());
    original_positions_.push_back (
      time_signature_ref.get ()->position ()->ticks ());

    // Create undo stack with a recording callback so tests can assert whether
    // an engine pause was requested for a pushed command.
    engine_pause_requested_ = false;
    undo_stack_ = std::make_unique<undo::UndoStack> (
      [this] (const std::function<void ()> &action, bool) {
        engine_pause_requested_ = true;
        action ();
      });

    // Create selection model and set up with test objects
    selection_model_ = std::make_unique<QItemSelectionModel> (&list_model_);

    // Create mock owner for testing
    mock_owner_ = std::make_unique<MockArrangerObjectOwner> (registry_);

    // Add the objects to the mock owner
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::add_object (marker_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::add_object (note_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiClip>::add_object (midi_clip_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AudioClip>::add_object (audio_clip_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AutomationClip>::add_object (automation_clip_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::TempoObject>::add_object (tempo_ref);
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::TimeSignatureObject>::
      add_object (time_signature_ref);

    // Create mock object owner provider that returns our mock owner
    auto mock_owner_provider =
      [this] (structure::arrangement::ArrangerObjectPtrVariant obj_var)
      -> ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
      return std::visit (
        [&] (auto &&obj)
          -> ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
          using ObjectT = utils::base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
            {
              // Prefer the parent object (e.g. a clip) as owner, like in the
              // project
              if (
                auto * parent_owner =
                  dynamic_cast<structure::arrangement::ArrangerObjectOwner<
                    structure::arrangement::MidiNote> *> (obj->parentObject ()))
                {
                  return parent_owner;
                }
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::MidiNote> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::ChordObject>)
            {
              // Prefer the parent clip as owner, like in the project
              return dynamic_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::ChordObject> *> (obj->parentObject ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::Marker>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::Marker> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiClip>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::MidiClip> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AudioClip>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::AudioClip> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationClip>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::AutomationClip> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::AutomationPoint> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::TempoObject>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::TempoObject> *> (mock_owner_.get ());
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::TimeSignatureObject>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                structure::arrangement::TimeSignatureObject> *> (
                mock_owner_.get ());
            }
          return static_cast<
            structure::arrangement::ArrangerObjectOwner<ObjectT> *> (nullptr);
        },
        obj_var);
    };

    // Create operator
    auto mock_enumerator =
      [this] (ArrangerObjectSelectionOperator::ArrangerObjectVisitor visitor) {
        for (const auto &obj_ref : test_objects_)
          {
            visitor (obj_ref);
          }
      };
    operator_ = std::make_unique<ArrangerObjectSelectionOperator> (
      *undo_stack_, *selection_model_, mock_owner_provider, *factory,
      mock_enumerator);
  }

  // Selects the row of the given object in the list model.
  void select_object (
    const structure::arrangement::ArrangerObjectUuidReference &ref,
    QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Select)
  {
    const int rows = list_model_.rowCount ();
    for (int i = 0; i < rows; ++i)
      {
        const auto variant = list_model_.data (
          list_model_.index (i, 0),
          structure::arrangement::ArrangerObjectListModel::
            ArrangerObjectUuidReferenceRole);
        if (
          auto * obj_ref =
            variant
              .value<structure::arrangement::ArrangerObjectUuidReference *> ();
          obj_ref != nullptr && obj_ref->id () == ref.id ())
          {
            selection_model_->select (list_model_.index (i, 0), flags);
            return;
          }
      }
    FAIL () << "Object not found in list model";
  }

  // Adds a MIDI note with the given content span to the clip and returns its
  // reference.
  structure::arrangement::ArrangerObjectUuidReference add_note_to_clip (
    structure::arrangement::MidiClip &clip,
    double                            content_pos,
    double                            content_len)
  {
    auto note_ref_local = utils::create_object<structure::arrangement::MidiNote> (
      registry_, *tempo_map_wrapper);
    note_ref_local.get ()->position ()->setTicks (content_pos);
    note_ref_local.get ()->length ()->setTicks (content_len);
    clip.structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::add_object (note_ref_local);
    return note_ref_local;
  }

  // Adds a chord at the given content position to the clip and returns its
  // reference.
  structure::arrangement::ArrangerObjectUuidReference
  add_chord_to_clip (structure::arrangement::ChordClip &clip, double content_pos)
  {
    auto chord_ref = utils::create_object<structure::arrangement::ChordObject> (
      registry_, *tempo_map_wrapper);
    chord_ref.get ()->position ()->setTicks (content_pos);
    clip.add_object (chord_ref);
    return chord_ref;
  }

  // Returns the child of the given owner vector at the given position in
  // ticks, or nullptr.
  template <typename ObjectT, typename ChildrenVector>
  static ObjectT *
  find_child_at (const ChildrenVector &children, double pos_ticks)
  {
    const auto it =
      std::ranges::find_if (children, [pos_ticks] (const auto &ref) {
        return ref.get ()->position ()->ticks () == pos_ticks;
      });
    return it != children.end () ? (*it).template get_object_as<ObjectT> () : nullptr;
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 registry_;
  structure::arrangement::ArrangerObjectRefMultiIndexContainer test_objects_;
  std::vector<double>              original_positions_;
  std::unique_ptr<undo::UndoStack> undo_stack_;
  bool                             engine_pause_requested_{ false };
  std::unique_ptr<ArrangerObjectSelectionOperator> operator_;
  structure::arrangement::ArrangerObjectListModel  list_model_{ test_objects_ };
  std::unique_ptr<QItemSelectionModel>             selection_model_;
  std::unique_ptr<MockArrangerObjectOwner>         mock_owner_;
  structure::arrangement::ArrangerObjectFactory::SampleRateProvider
    sample_rate_provider;
  structure::arrangement::ArrangerObjectFactory::BpmProvider     bpm_provider;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory;
  std::unique_ptr<utils::AppSettings>                            app_settings;
  structure::arrangement::ArrangerObjectUuidReference note_ref{ registry_ };
  structure::arrangement::ArrangerObjectUuidReference marker_ref{ registry_ };
  structure::arrangement::ArrangerObjectUuidReference audio_clip_ref{
    registry_
  };
  structure::arrangement::ArrangerObjectUuidReference automation_clip_ref{
    registry_
  };
  structure::arrangement::ArrangerObjectUuidReference midi_clip_ref{ registry_ };
  structure::arrangement::ArrangerObjectUuidReference tempo_ref{ registry_ };
  structure::arrangement::ArrangerObjectUuidReference time_signature_ref{
    registry_
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
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

  const double tick_delta = 100.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Only selected objects (marker and note) should be moved by tick_delta
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get ())
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
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

  // Move objects to position 100 first to allow negative movement
  for (const auto &obj_ref : test_objects_)
    {
      if (auto * obj = obj_ref.get ())
        {
          obj->position ()->setTicks (100.0);
        }
    }

  const double tick_delta = -50.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Only selected objects (marker and note) should be moved backward by
  // tick_delta Check marker (index 0)
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (marker_obj->position ()->ticks (), 50);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), 50);
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test moveByTicks with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksZeroDelta)
{
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

  bool result = operator_->moveByTicks (0.0);
  EXPECT_TRUE (result);

  // Only selected objects (marker and note) should remain at original positions
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0]);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get ())
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
      if (auto * obj = obj_ref.get ())
        {
          obj->position ()->setTicks (0.0);
        }
    }

  const double tick_delta = -50.0; // Would put objects at -50 ticks

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_FALSE (result);

  // Only selected objects (marker and note) should remain at position 0 (no
  // movement) Check marker (index 0)
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (marker_obj->position ()->ticks (), 0.0);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), 0.0);
    }

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test undo/redo functionality
TEST_F (ArrangerObjectSelectionOperatorTest, UndoRedoFunctionality)
{
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

  const double tick_delta = 100.0;

  // Move objects
  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify only selected objects moved
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        note_obj->position ()->ticks (), original_positions_[1] + tick_delta);
    }

  // Undo
  undo_stack_->undo ();
  // Verify only selected objects restored
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0]);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get ())
    {
      EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), original_positions_[1]);
    }

  // Redo
  undo_stack_->redo ();
  // Verify only selected objects moved again
  // Check marker (index 0)
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  // Check note (index 1)
  if (auto * note_obj = note_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        note_obj->position ()->ticks (), original_positions_[1] + tick_delta);
    }
}

// Test moveNotesByPitch functionality
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitch)
{
  // Select note for testing
  select_object (note_ref);

  const int pitch_delta = 5;

  // Store original pitch for MIDI note
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  ASSERT_NE (note_obj, nullptr);
  int original_pitch = note_obj->pitch ();

  bool result = operator_->moveNotesByPitch (pitch_delta);
  EXPECT_TRUE (result);

  // MIDI notes should be moved by pitch_delta
  EXPECT_EQ (note_obj->pitch (), original_pitch + pitch_delta);

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Test undo/redo
  undo_stack_->undo ();
  EXPECT_EQ (note_obj->pitch (), original_pitch);

  undo_stack_->redo ();
  EXPECT_EQ (note_obj->pitch (), original_pitch + pitch_delta);
}

// Test moveAutomationPointsByDelta functionality
TEST_F (ArrangerObjectSelectionOperatorTest, MoveAutomationPointsByDelta)
{
  // Create an automation point for testing
  auto automation_point_ref = utils::create_object<
    structure::arrangement::AutomationPoint> (registry_, *tempo_map_wrapper);

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
  select_object (automation_point_ref);

  const double delta = 0.2;

  bool result = operator_->moveAutomationPointsByDelta (delta);
  EXPECT_TRUE (result);

  // Only selected automation point should be moved by delta
  auto * auto_obj = automation_point_ref.get_object_as<
    structure::arrangement::AutomationPoint> ();
  ASSERT_NE (auto_obj, nullptr);
  EXPECT_FLOAT_EQ (auto_obj->value (), 0.7f); // 0.5 + 0.2

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Test undo/redo
  undo_stack_->undo ();

  const auto * auto_obj_after_undo = automation_point_ref.get_object_as<
    structure::arrangement::AutomationPoint> ();
  ASSERT_NE (auto_obj_after_undo, nullptr);
  EXPECT_FLOAT_EQ (auto_obj_after_undo->value (), 0.5f);

  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (auto_obj_after_undo->value (), 0.7f);
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
  // Select note for testing
  select_object (note_ref);

  bool result = operator_->moveNotesByPitch (0);
  EXPECT_TRUE (result);

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test moveNotesByPitch with invalid pitch (out of range)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchInvalidPitch)
{
  // Select note for testing
  select_object (note_ref);

  // Find MIDI note and set its pitch to 125 (close to max)
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  ASSERT_NE (note_obj, nullptr);
  note_obj->setPitch (125);
  int original_pitch = 125;

  // Try to move by 5 (would result in pitch 130, which is out of range)
  bool result = operator_->moveNotesByPitch (5);
  EXPECT_FALSE (result);

  // Pitch should remain unchanged
  EXPECT_EQ (note_obj->pitch (), original_pitch);

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
  // Create an automation point with value 0.9
  auto automation_point_ref = utils::create_object<
    structure::arrangement::AutomationPoint> (registry_, *tempo_map_wrapper);

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
  select_object (automation_point_ref);

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
  auto automation_point_ref = utils::create_object<
    structure::arrangement::AutomationPoint> (registry_, *tempo_map_wrapper);

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
  select_object (automation_point_ref);

  // Try to move by 0.2 (would result in value 1.1, which is out of range)
  bool result = operator_->moveAutomationPointsByDelta (0.2);
  EXPECT_FALSE (result);

  // Value should remain unchanged
  const auto * auto_obj = automation_point_ref.get_object_as<
    structure::arrangement::AutomationPoint> ();
  ASSERT_NE (auto_obj, nullptr);
  EXPECT_FLOAT_EQ (auto_obj->value (), 0.9f);

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test deleteObjects with valid selection
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsValidSelection)
{
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

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

// Test deleteObject (eraser tool): deletes a single object directly, without
// involving the selection
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectRemovesOwnedObject)
{
  const int  initial_count = undo_stack_->count ();
  const auto note_id = note_ref.id ();

  EXPECT_TRUE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id));

  EXPECT_TRUE (operator_->deleteObject (note_ref.get ()));

  // Exactly one command pushed
  EXPECT_EQ (undo_stack_->count (), initial_count + 1);
  EXPECT_FALSE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id))
    << "Note should have been deleted";

  // Undo restores the object
  undo_stack_->undo ();
  EXPECT_TRUE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id))
    << "Undo should restore the deleted note";
}

TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectNullReturnsFalse)
{
  const int initial_count = undo_stack_->count ();

  EXPECT_FALSE (operator_->deleteObject (nullptr));
  EXPECT_EQ (undo_stack_->count (), initial_count);
}

TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectUndeletableReturnsFalse)
{
  // Start markers are not deletable
  auto start_marker_ref = utils::create_object<structure::arrangement::Marker> (
    registry_, *tempo_map_wrapper,
    structure::arrangement::Marker::MarkerType::Start);

  const int initial_count = undo_stack_->count ();

  EXPECT_FALSE (operator_->deleteObject (start_marker_ref.get ()));
  EXPECT_EQ (undo_stack_->count (), initial_count);
}

TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectNotInOwnerReturnsFalse)
{
  // An object that no owner contains
  auto orphan_note_ref = utils::create_object<structure::arrangement::MidiNote> (
    registry_, *tempo_map_wrapper);

  const int initial_count = undo_stack_->count ();

  EXPECT_FALSE (operator_->deleteObject (orphan_note_ref.get ()));
  EXPECT_EQ (undo_stack_->count (), initial_count);
}

// Test deleteObject with an automation clip (owned by an AutomationTrack in
// the project, not directly by a track — the mock owner mirrors that
// indirection)
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectAutomationClip)
{
  const int  initial_count = undo_stack_->count ();
  const auto clip_id = automation_clip_ref.id ();

  EXPECT_TRUE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AutomationClip>::contains_object (clip_id));

  EXPECT_TRUE (operator_->deleteObject (automation_clip_ref.get ()));

  EXPECT_EQ (undo_stack_->count (), initial_count + 1);
  EXPECT_FALSE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AutomationClip>::contains_object (clip_id))
    << "Automation clip should have been deleted";

  undo_stack_->undo ();
  EXPECT_TRUE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AutomationClip>::contains_object (clip_id))
    << "Undo should restore the deleted automation clip";
}

// Test deleting tempo/time-signature objects (owned by TempoObjectManager, not
// a track) — guards the TempoObject/TimeSignatureObject owner dispatch.
TEST_F (ArrangerObjectSelectionOperatorTest, DeleteObjectsTempoAndTimeSignature)
{
  // tempo_ref is index 4, time_signature_ref is index 5 in the list model.
  select_object (tempo_ref);
  select_object (time_signature_ref);

  const auto tempo_id = tempo_ref.id ();
  const auto ts_id = time_signature_ref.id ();

  EXPECT_TRUE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::TempoObject>::contains_object (tempo_id));
  EXPECT_TRUE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::TimeSignatureObject>::contains_object (ts_id));

  EXPECT_TRUE (operator_->deleteObjects ());

  EXPECT_FALSE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::TempoObject>::contains_object (tempo_id))
    << "Tempo object should have been deleted";
  EXPECT_FALSE (
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::TimeSignatureObject>::contains_object (ts_id))
    << "Time-signature object should have been deleted";
}

// The undo stack must recognize tempo/time-signature object removals as
// requiring the audio engine to be paused (so the tempo map can be rebuilt
// without the RT thread reading torn data).
TEST_F (ArrangerObjectSelectionOperatorTest, RemoveTempoObjectRequestsEnginePause)
{
  bool            engine_pause_requested = false;
  undo::UndoStack stack (
    [&engine_pause_requested] (const std::function<void ()> &action, bool) {
      engine_pause_requested = true;
      action ();
    });

  stack.push (
    new commands::RemoveArrangerObjectCommand<
      structure::arrangement::TempoObject> (*mock_owner_, tempo_ref));

  EXPECT_TRUE (engine_pause_requested)
    << "Removing a tempo object must request an engine pause";
}

// Non-tempo-map-affecting removals must NOT pause the engine.
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  RemoveNonTempoObjectDoesNotRequestEnginePause)
{
  bool            engine_pause_requested = false;
  undo::UndoStack stack (
    [&engine_pause_requested] (const std::function<void ()> &action, bool) {
      engine_pause_requested = true;
      action ();
    });

  stack.push (
    new commands::RemoveArrangerObjectCommand<structure::arrangement::Marker> (
      *mock_owner_, marker_ref));

  EXPECT_FALSE (engine_pause_requested);
}

// Cloning a tempo object must use the tempo-map-affecting add command so the
// engine is paused.
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  CloneObjectsTempoObjectRequestsEnginePause)
{
  // tempo_ref is index 4 in the list model.
  select_object (tempo_ref);

  EXPECT_TRUE (operator_->cloneObjects ());
  EXPECT_TRUE (engine_pause_requested_)
    << "Cloning a tempo object must request an engine pause";
}

// Deleting a tempo object via deleteObjects() must request an engine pause
// (end-to-end check through the selection operator + macro-wrapped removes).
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  DeleteTempoObjectRequestsEnginePauseViaDeleteObjects)
{
  // tempo_ref is index 4 in the list model.
  select_object (tempo_ref);

  EXPECT_TRUE (operator_->deleteObjects ());
  EXPECT_TRUE (engine_pause_requested_)
    << "Deleting a tempo object via deleteObjects() must request an engine "
       "pause";
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
  auto start_marker_ref = utils::create_object<structure::arrangement::Marker> (
    registry_, *tempo_map_wrapper,
    structure::arrangement::Marker::MarkerType::Start);

  // Clear existing objects and add only non-deletable marker
  test_objects_.get<structure::arrangement::random_access_index> ().clear ();
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    start_marker_ref);

  // Update selection to only include non-deletable marker
  selection_model_->clear ();
  select_object (start_marker_ref);

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
  auto start_marker_ref = utils::create_object<structure::arrangement::Marker> (
    registry_, *tempo_map_wrapper,
    structure::arrangement::Marker::MarkerType::Start);

  // Add non-deletable marker to existing objects and to mock owner
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    start_marker_ref);
  mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::Marker>::add_object (start_marker_ref);

  // Update selection to include multiple objects
  select_object (marker_ref);
  select_object (note_ref);
  select_object (start_marker_ref);

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
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

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
  select_object (note_ref); // Select only MidiNote

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects were resized by checking the object directly
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  EXPECT_DOUBLE_EQ (note_obj->length ()->ticks (), 4500.0); // 4000 + 500
}

// Test resizeObjects with bounds resize from start
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsBoundsFromStart)
{
  const double delta = -200.0;

  // Clear selection and only select objects that support bounds resize
  selection_model_->clear ();
  select_object (note_ref); // Select only MidiNote

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromStart, delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects were resized by checking the object directly
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  EXPECT_DOUBLE_EQ (note_obj->position ()->ticks (), 800.0); // 1000 - 200
  EXPECT_DOUBLE_EQ (note_obj->length ()->ticks (), 4200.0);  // 4000 + 200
}

// Test resizeObjects with loop points resize from end
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsLoopPointsFromEnd)
{
  const double delta = 100.0;

  // Clear selection and only select objects that support loop points resize
  selection_model_->clear ();
  select_object (midi_clip_ref); // Select only MidiClip

  bool result = operator_->resizeObjects (
    commands::ResizeType::LoopPoints, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Just verify bounds - actual logic is tested by the command class
  auto * midi_clip_obj =
    midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  EXPECT_DOUBLE_EQ (midi_clip_obj->length ()->ticks (),
                    4100.0); // 4000 + 100
}

// Test resizeObjects with loop points resize from start
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsLoopPointsFromStart)
{
  const double delta = 100.0;

  // Clear selection and only select objects that support loop points resize
  selection_model_->clear ();
  select_object (midi_clip_ref); // Select only MidiClip

  bool result = operator_->resizeObjects (
    commands::ResizeType::LoopPoints, commands::ResizeDirection::FromStart,
    delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Just verify bounds - actual logic is tested by the command class
  auto * midi_clip_obj =
    midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  EXPECT_DOUBLE_EQ (midi_clip_obj->position ()->ticks (), 2100.0); // 2000 + 100
  EXPECT_DOUBLE_EQ (midi_clip_obj->length ()->ticks (),
                    3900.0); // 4000 - 100
}

// Test resizeObjects with fades resize
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsFades)
{
  const double delta = 50.0;

  // Clear selection and only select objects that support fades resize
  selection_model_->clear ();
  select_object (audio_clip_ref); // Select only AudioClip

  bool result = operator_->resizeObjects (
    commands::ResizeType::Fades, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify objects were resized by checking the object directly
  auto * audio_clip_obj =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  EXPECT_DOUBLE_EQ (
    audio_clip_obj->fadeRange ()->endOffset ()->ticks (), 350.0); // 300 + 50
}

// A fade resize that would drive the fade negative must be rejected.
TEST_F (ArrangerObjectSelectionOperatorTest, ResizeObjectsFadesRejectNegative)
{
  selection_model_->clear ();
  select_object (audio_clip_ref); // AudioClip (fadeOut = 300)

  bool result = operator_->resizeObjects (
    commands::ResizeType::Fades, commands::ResizeDirection::FromEnd, -500.0);
  EXPECT_FALSE (result);
  EXPECT_EQ (undo_stack_->index (), 0); // no command pushed

  // Fade must be unchanged.
  auto * audio_clip_obj =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  EXPECT_DOUBLE_EQ (audio_clip_obj->fadeRange ()->endOffset ()->ticks (), 300.0);
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
  select_object (note_ref); // Select only MidiNote

  // Perform resize
  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromEnd, delta);
  EXPECT_TRUE (result);
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify resize by checking the object directly
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  EXPECT_DOUBLE_EQ (note_obj->length ()->ticks (), 4300.0); // 4000 + 300

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (undo_stack_->index (), 0);

  // Verify undo
  EXPECT_DOUBLE_EQ (note_obj->length ()->ticks (), 4000.0); // Original restored

  // Redo
  undo_stack_->redo ();
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify redo
  EXPECT_DOUBLE_EQ (note_obj->length ()->ticks (), 4300.0); // Applied again
}

// Test that non-timeline objects (MIDI notes) can be resized to negative local
// positions
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  ResizeObjectsNonTimelineObjectNegativePosition)
{
  // Clear selection and only select MIDI note (non-timeline object)
  selection_model_->clear ();
  select_object (note_ref); // Select only MidiNote

  // Set MIDI note position to a small positive value first
  if (auto * note_obj = note_ref.get ())
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
      EXPECT_DOUBLE_EQ (note_obj->length ()->ticks (), 4020.0);  // 4000 + 20
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test resizeObjects with bounds resize from start that would make length zero
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  ResizeObjectsBoundsFromStartZeroLength)
{
  // Clear selection and only select objects that support bounds resize
  selection_model_->clear ();
  select_object (note_ref); // Select only MidiNote

  // Set initial length to 100 ticks
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  note_obj->length ()->setTicks (100.0);

  // Try to resize by delta that would make length zero or negative
  const double delta = 150.0; // Would make length -50 (100 - 150)

  bool result = operator_->resizeObjects (
    commands::ResizeType::Bounds, commands::ResizeDirection::FromStart, delta);

  // This should fail because it would make length zero/negative
  EXPECT_FALSE (result)
    << "Resize from start should not allow length to become zero or negative";

  // No command should be pushed for invalid resize
  EXPECT_EQ (undo_stack_->index (), 0);

  // Length should remain unchanged
  EXPECT_DOUBLE_EQ (note_obj->length ()->ticks (), 100.0);
}

// Test resizeObjects with loop points resize from start that would make length
// zero
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  ResizeObjectsLoopPointsFromStartZeroLength)
{
  // Clear selection and only select objects that support loop points resize
  selection_model_->clear ();
  select_object (midi_clip_ref); // Select only MidiClip

  // Set initial length to 100 ticks
  auto * midi_clip_obj =
    midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  midi_clip_obj->length ()->setTicks (100.0);

  // Try to resize by delta that would make length zero or negative
  const double delta = 150.0; // Would make length -50 (100 - 150)

  bool result = operator_->resizeObjects (
    commands::ResizeType::LoopPoints, commands::ResizeDirection::FromStart,
    delta);

  // This should fail because it would make length zero/negative
  EXPECT_FALSE (result)
    << "Loop points resize from start should not allow length to become zero or negative";

  // No command should be pushed for invalid resize
  EXPECT_EQ (undo_stack_->index (), 0);

  // Length should remain unchanged
  EXPECT_DOUBLE_EQ (midi_clip_obj->length ()->ticks (), 100.0);
}

// Test moveByTicks with TempoObject - should create
// MoveTempoMapAffectingArrangerObjectsCommand
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksTempoObject)
{
  // Clear selection and only select tempo object
  selection_model_->clear ();
  select_object (tempo_ref); // Tempo object

  const double tick_delta = 100.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify the command type is MoveTempoMapAffectingArrangerObjectsCommand
  const auto * cmd = undo_stack_->command (0);
  EXPECT_EQ (
    cmd->id (),
    commands::MoveTempoMapAffectingArrangerObjectsCommand::CommandId);

  // Verify tempo object was moved
  if (auto * tempo_obj = tempo_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        tempo_obj->position ()->ticks (), original_positions_[4] + tick_delta);
    }
}

// Test moveByTicks with TimeSignatureObject at valid bar boundary
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksTimeSignatureObjectValid)
{
  // Clear selection and only select time signature object
  selection_model_->clear ();
  select_object (time_signature_ref); // Time signature object

  // Set time signature to position 0 (bar boundary)
  time_signature_ref.get ()->position ()->setTicks (0.0);

  const double tick_delta = 3840.0; // Move to next bar (assuming 4/4, 120 BPM)

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify the command type is MoveTempoMapAffectingArrangerObjectsCommand
  const auto * cmd = undo_stack_->command (0);
  EXPECT_EQ (
    cmd->id (),
    commands::MoveTempoMapAffectingArrangerObjectsCommand::CommandId);

  // Verify time signature object was moved
  if (auto * ts_obj = time_signature_ref.get ())
    {
      EXPECT_DOUBLE_EQ (ts_obj->position ()->ticks (), tick_delta);
    }
}

// Test moveByTicks with TimeSignatureObject at invalid position (not bar
// boundary)
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveByTicksTimeSignatureObjectInvalid)
{
  // Clear selection and only select time signature object
  selection_model_->clear ();
  select_object (time_signature_ref); // Time signature object

  // Set time signature to position 0 (bar boundary)
  time_signature_ref.get ()->position ()->setTicks (0.0);

  const double tick_delta = 100.0; // Move to non-bar boundary position

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_FALSE (result);

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);

  // Verify time signature object was not moved
  if (auto * ts_obj = time_signature_ref.get ())
    {
      EXPECT_DOUBLE_EQ (ts_obj->position ()->ticks (), 0.0);
    }
}

// Test moveByTicks with mixed selection including tempo map affecting objects
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksMixedWithTempoObjects)
{
  // Clear selection and select regular object + tempo object
  selection_model_->clear ();
  select_object (marker_ref); // Marker
  select_object (tempo_ref);  // Tempo object

  const double tick_delta = 100.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify the command type is MoveTempoMapAffectingArrangerObjectsCommand
  // (because tempo object is in selection)
  const auto * cmd = undo_stack_->command (0);
  EXPECT_EQ (
    cmd->id (),
    commands::MoveTempoMapAffectingArrangerObjectsCommand::CommandId);

  // Verify both objects were moved
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  if (auto * tempo_obj = tempo_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        tempo_obj->position ()->ticks (), original_positions_[4] + tick_delta);
    }
}

// Test moveByTicks with mixed selection including time signature object at
// valid position
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveByTicksMixedWithTimeSignatureValid)
{
  // Clear selection and select regular object + time signature object
  selection_model_->clear ();
  select_object (marker_ref);         // Marker
  select_object (time_signature_ref); // Time signature object

  // Set time signature to position 0 (bar boundary)
  time_signature_ref.get ()->position ()->setTicks (0.0);

  const double tick_delta = 3840.0; // Move to next bar

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Check that command was pushed to undo stack
  EXPECT_EQ (undo_stack_->index (), 1);

  // Verify the command type is MoveTempoMapAffectingArrangerObjectsCommand
  // (because time signature object is in selection)
  const auto * cmd = undo_stack_->command (0);
  EXPECT_EQ (
    cmd->id (),
    commands::MoveTempoMapAffectingArrangerObjectsCommand::CommandId);

  // Verify both objects were moved
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0] + tick_delta);
    }
  if (auto * ts_obj = time_signature_ref.get ())
    {
      EXPECT_DOUBLE_EQ (ts_obj->position ()->ticks (), tick_delta);
    }
}

// Test moveByTicks with mixed selection including time signature object at
// invalid position
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveByTicksMixedWithTimeSignatureInvalid)
{
  // Clear selection and select regular object + time signature object
  selection_model_->clear ();
  select_object (marker_ref);         // Marker
  select_object (time_signature_ref); // Time signature object

  // Set time signature to position 0 (bar boundary)
  time_signature_ref.get ()->position ()->setTicks (0.0);

  const double tick_delta = 100.0; // Move to non-bar boundary position

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_FALSE (result);

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->index (), 0);

  // Verify neither object was moved
  if (auto * marker_obj = marker_ref.get ())
    {
      EXPECT_DOUBLE_EQ (
        marker_obj->position ()->ticks (), original_positions_[0]);
    }
  if (auto * ts_obj = time_signature_ref.get ())
    {
      EXPECT_DOUBLE_EQ (ts_obj->position ()->ticks (), 0.0);
    }
}

// Test cloneObjects with valid selection
TEST_F (ArrangerObjectSelectionOperatorTest, CloneObjectsValidSelection)
{
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Store UUIDs of original objects
  const auto marker_id = marker_ref.id ();
  const auto note_id = note_ref.id ();

  // Verify objects exist before cloning
  bool marker_found_before =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_before =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_TRUE (marker_found_before);
  EXPECT_TRUE (note_found_before);

  // Clone selected objects
  bool result = operator_->cloneObjects ();
  EXPECT_TRUE (result);

  // Should have created a macro command with individual add commands
  EXPECT_GT (undo_stack_->count (), initial_count);

  // Verify original objects still exist
  bool marker_found_after_original =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::Marker>::contains_object (marker_id);
  bool note_found_after_original =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote>::contains_object (note_id);
  EXPECT_TRUE (marker_found_after_original);
  EXPECT_TRUE (note_found_after_original);

  // Verify cloned objects were added (they should have new UUIDs)
  // We can't easily verify the exact cloned objects without exposing more
  // internals, but we can verify that more objects exist in the owners
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::Marker>::get_children_vector ()
      .size (),
    2);
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>::get_children_vector ()
      .size (),
    2);
}

// Test cloneObjects with no selection
TEST_F (ArrangerObjectSelectionOperatorTest, CloneObjectsNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Attempt to clone with no selection
  bool result = operator_->cloneObjects ();
  EXPECT_FALSE (result);

  // No commands should be pushed for no selection
  EXPECT_EQ (undo_stack_->index (), initial_count);
}

// Test cloneObjects with uncloneable objects
TEST_F (ArrangerObjectSelectionOperatorTest, CloneObjectsUncloneableObject)
{
  // Create a non-deletable marker (start marker) - using same logic as
  // cloneable check
  auto start_marker_ref = utils::create_object<structure::arrangement::Marker> (
    registry_, *tempo_map_wrapper,
    structure::arrangement::Marker::MarkerType::Start);

  // Clear existing objects and add only uncloneable marker
  test_objects_.get<structure::arrangement::random_access_index> ().clear ();
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    start_marker_ref);

  // Update list model and selection
  selection_model_->clear ();
  select_object (start_marker_ref);

  // Store initial undo stack count
  const int initial_count = undo_stack_->count ();

  // Attempt to clone uncloneable object
  bool result = operator_->cloneObjects ();
  EXPECT_FALSE (result);

  // No commands should be pushed for uncloneable objects
  EXPECT_EQ (undo_stack_->index (), initial_count);
}

// Test cloneObjects undo/redo functionality
TEST_F (ArrangerObjectSelectionOperatorTest, CloneObjectsUndoRedo)
{
  // Select marker and note for testing
  select_object (marker_ref);
  select_object (note_ref);

  // Store initial counts
  const int  initial_undo_count = undo_stack_->count ();
  const auto initial_marker_count =
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::Marker>::get_children_vector ()
      .size ();
  const auto initial_note_count =
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>::get_children_vector ()
      .size ();

  // Clone objects
  bool result = operator_->cloneObjects ();
  EXPECT_TRUE (result);

  const int after_clone_count = undo_stack_->count ();
  EXPECT_GT (after_clone_count, initial_undo_count);

  // Verify cloned objects were added
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::Marker>::get_children_vector ()
      .size (),
    initial_marker_count * 2);
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>::get_children_vector ()
      .size (),
    initial_note_count * 2);

  // Undo cloning
  undo_stack_->undo ();
  EXPECT_EQ (undo_stack_->index (), after_clone_count - 1);

  // Verify objects are back to original count after undo
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::Marker>::get_children_vector ()
      .size (),
    initial_marker_count);
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>::get_children_vector ()
      .size (),
    initial_note_count);

  // Redo cloning
  undo_stack_->redo ();
  EXPECT_EQ (undo_stack_->index (), after_clone_count);

  // Verify objects are cloned again after redo
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::Marker>::get_children_vector ()
      .size (),
    initial_marker_count * 2);
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiNote>::get_children_vector ()
      .size (),
    initial_note_count * 2);
}

// Test cloneObjects with audio clip to verify audio source cloning
TEST_F (ArrangerObjectSelectionOperatorTest, CloneObjectsAudioClip)
{
  // Select audio clip for testing
  selection_model_->clear ();
  select_object (audio_clip_ref);

  // Store initial counts
  const int  initial_undo_count = undo_stack_->count ();
  const auto initial_audio_clip_count =
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AudioClip>::get_children_vector ()
      .size ();

  // Clone audio clip
  bool result = operator_->cloneObjects ();
  EXPECT_TRUE (result);

  // Verify command was pushed
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Verify audio clip was cloned
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AudioClip>::get_children_vector ()
      .size (),
    initial_audio_clip_count * 2);

  // Test undo/redo to ensure proper audio source handling
  undo_stack_->undo ();
  EXPECT_EQ (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AudioClip>::get_children_vector ()
      .size (),
    initial_audio_clip_count);

  undo_stack_->redo ();
  EXPECT_GT (
    mock_owner_
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::AudioClip>::get_children_vector ()
      .size (),
    initial_audio_clip_count);
}

// Test changeVelocities functionality
TEST_F (ArrangerObjectSelectionOperatorTest, ChangeVelocities)
{
  // Select note for testing
  select_object (note_ref);

  const int velocity_delta = 15;

  // Store original velocity for MIDI note
  auto * note_obj = note_ref.get_object_as<structure::arrangement::MidiNote> ();
  ASSERT_NE (note_obj, nullptr);
  int original_velocity = note_obj->velocity ();

  bool result = operator_->changeVelocities (velocity_delta);
  EXPECT_TRUE (result);

  // MIDI notes should have velocity changed
  EXPECT_EQ (note_obj->velocity (), original_velocity + velocity_delta);

  // Undo should restore original velocity
  undo_stack_->undo ();
  EXPECT_EQ (note_obj->velocity (), original_velocity);

  // Redo should apply velocity change again
  undo_stack_->redo ();
  EXPECT_EQ (note_obj->velocity (), original_velocity + velocity_delta);
}

// Test changeVelocities with no selection (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, ChangeVelocitiesNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  bool result = operator_->changeVelocities (10);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test changeVelocities with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, ChangeVelocitiesZeroDelta)
{
  // Select note for testing
  select_object (note_ref);

  bool result = operator_->changeVelocities (0);
  EXPECT_TRUE (result);

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test rampVelocities linear interpolation across selected notes
TEST_F (ArrangerObjectSelectionOperatorTest, RampVelocitiesLinearInterpolation)
{
  auto ramp_note_ref1 = utils::create_object<structure::arrangement::MidiNote> (
    registry_, *tempo_map_wrapper);
  auto ramp_note_ref2 = utils::create_object<structure::arrangement::MidiNote> (
    registry_, *tempo_map_wrapper);
  auto ramp_note_ref3 = utils::create_object<structure::arrangement::MidiNote> (
    registry_, *tempo_map_wrapper);
  for (
    const auto &ramp_note_ref :
    { ramp_note_ref1, ramp_note_ref2, ramp_note_ref3 })
    {
      ramp_note_ref.get_object_as<structure::arrangement::MidiNote> ()
        ->setVelocity (64);
      test_objects_.get<structure::arrangement::random_access_index> ()
        .push_back (ramp_note_ref);
    }
  ramp_note_ref1.get ()->position ()->setTicks (1000.0);
  ramp_note_ref2.get ()->position ()->setTicks (2000.0);
  ramp_note_ref3.get ()->position ()->setTicks (3000.0);

  select_object (ramp_note_ref1);
  select_object (ramp_note_ref2);
  select_object (ramp_note_ref3);

  bool result = operator_->rampVelocities (nullptr, 1000.0, 10.0, 3000.0, 110.0);
  EXPECT_TRUE (result);

  EXPECT_EQ (
    ramp_note_ref1.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    10);
  EXPECT_EQ (
    ramp_note_ref2.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    60);
  EXPECT_EQ (
    ramp_note_ref3.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    110);

  // One macro for the whole ramp
  EXPECT_EQ (undo_stack_->index (), 1);

  // Undo restores all original velocities, redo re-applies the ramp
  undo_stack_->undo ();
  EXPECT_EQ (
    ramp_note_ref1.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    64);
  EXPECT_EQ (
    ramp_note_ref2.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    64);
  EXPECT_EQ (
    ramp_note_ref3.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    64);
  undo_stack_->redo ();
  EXPECT_EQ (
    ramp_note_ref2.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    60);
}

// Test rampVelocities clamps notes outside the line's span to the nearest
// endpoint value, including when the line is dragged right-to-left
TEST_F (ArrangerObjectSelectionOperatorTest, RampVelocitiesClampsOutsideSpan)
{
  auto ramp_note_ref1 = utils::create_object<structure::arrangement::MidiNote> (
    registry_, *tempo_map_wrapper);
  auto ramp_note_ref2 = utils::create_object<structure::arrangement::MidiNote> (
    registry_, *tempo_map_wrapper);
  for (const auto &ramp_note_ref : { ramp_note_ref1, ramp_note_ref2 })
    {
      ramp_note_ref.get_object_as<structure::arrangement::MidiNote> ()
        ->setVelocity (64);
      test_objects_.get<structure::arrangement::random_access_index> ()
        .push_back (ramp_note_ref);
    }
  ramp_note_ref1.get ()->position ()->setTicks (0.0);
  ramp_note_ref2.get ()->position ()->setTicks (5000.0);

  select_object (ramp_note_ref1);
  select_object (ramp_note_ref2);

  // Reversed drag (end before start): each note outside the span gets the
  // value of its nearest endpoint (10 at tick 1000, 110 at tick 3000)
  bool result = operator_->rampVelocities (nullptr, 3000.0, 110.0, 1000.0, 10.0);
  EXPECT_TRUE (result);

  EXPECT_EQ (
    ramp_note_ref1.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    10);
  EXPECT_EQ (
    ramp_note_ref2.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    110);
}

// Test rampVelocities with identical start and end positions (vertical line)
TEST_F (ArrangerObjectSelectionOperatorTest, RampVelocitiesEqualEndpoints)
{
  note_ref.get_object_as<structure::arrangement::MidiNote> ()->setVelocity (64);
  select_object (note_ref);

  bool result = operator_->rampVelocities (nullptr, 1000.0, 30.0, 1000.0, 90.0);
  EXPECT_TRUE (result);

  EXPECT_EQ (
    note_ref.get_object_as<structure::arrangement::MidiNote> ()->velocity (),
    90);
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test rampVelocities with no selection
TEST_F (ArrangerObjectSelectionOperatorTest, RampVelocitiesNoSelection)
{
  selection_model_->clear ();

  bool result = operator_->rampVelocities (nullptr, 0.0, 0.0, 1000.0, 127.0);
  EXPECT_FALSE (result);
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test rampVelocities ignores selected objects that are not MIDI notes
TEST_F (ArrangerObjectSelectionOperatorTest, RampVelocitiesIgnoresNonNotes)
{
  select_object (marker_ref);

  bool result = operator_->rampVelocities (nullptr, 0.0, 0.0, 1000.0, 127.0);
  EXPECT_FALSE (result);

  // No command should be pushed when nothing changed
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test rampVelocities applies to all notes inside the line's span when no
// notes are selected
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  RampVelocitiesClipScopeWhenNoSelection)
{
  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);

  // Clip at timeline 2000 with identity warp: notes at timeline 2000, 3000
  // and 7000
  auto note1_ref = add_note_to_clip (*clip, 0.0, 100.0);
  auto note2_ref = add_note_to_clip (*clip, 1000.0, 100.0);
  auto note3_ref = add_note_to_clip (*clip, 5000.0, 100.0);
  for (const auto &ramp_note_ref : { note1_ref, note2_ref, note3_ref })
    {
      ramp_note_ref.get_object_as<structure::arrangement::MidiNote> ()
        ->setVelocity (64);
    }

  selection_model_->clear ();

  bool result = operator_->rampVelocities (clip, 2000.0, 10.0, 3000.0, 110.0);
  EXPECT_TRUE (result);

  // Notes inside the span get the line's velocities
  EXPECT_EQ (
    note1_ref.get_object_as<structure::arrangement::MidiNote> ()->velocity (),
    10);
  EXPECT_EQ (
    note2_ref.get_object_as<structure::arrangement::MidiNote> ()->velocity (),
    110);
  // Note outside the span is untouched
  EXPECT_EQ (
    note3_ref.get_object_as<structure::arrangement::MidiNote> ()->velocity (),
    64);

  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test rampVelocities only affects the selected notes even when a clip is
// given
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  RampVelocitiesSelectionTakesPrecedenceOverClip)
{
  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);

  // Selected note at timeline 2000 (outside the ramp span), unselected note
  // at timeline 7000 (inside the ramp span)
  auto selected_note_ref = add_note_to_clip (*clip, 0.0, 100.0);
  auto other_note_ref = add_note_to_clip (*clip, 5000.0, 100.0);
  for (const auto &ramp_note_ref : { selected_note_ref, other_note_ref })
    {
      ramp_note_ref.get_object_as<structure::arrangement::MidiNote> ()
        ->setVelocity (64);
    }
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    selected_note_ref);
  select_object (selected_note_ref);

  bool result = operator_->rampVelocities (clip, 5000.0, 20.0, 7000.0, 80.0);
  EXPECT_TRUE (result);

  // Selected note outside the span gets the nearest endpoint's value
  EXPECT_EQ (
    selected_note_ref.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    20);
  // Unselected clip note inside the span is untouched
  EXPECT_EQ (
    other_note_ref.get_object_as<structure::arrangement::MidiNote> ()
      ->velocity (),
    64);
}

// Test changeVelocities with invalid velocity (out of range)
// Test toggleMute mutes unmuted objects
TEST_F (ArrangerObjectSelectionOperatorTest, ToggleMuteMutesUnmutedObjects)
{
  selection_model_->clear ();
  select_object (note_ref);
  select_object (midi_clip_ref);

  EXPECT_FALSE (note_ref.get ()->mute ()->muted ());
  EXPECT_FALSE (midi_clip_ref.get ()->mute ()->muted ());

  bool result = operator_->toggleMute ();
  EXPECT_TRUE (result);

  EXPECT_TRUE (note_ref.get ()->mute ()->muted ());
  EXPECT_TRUE (midi_clip_ref.get ()->mute ()->muted ());
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test toggleMute unmutes muted objects
TEST_F (ArrangerObjectSelectionOperatorTest, ToggleMuteUnmutesMutedObjects)
{
  note_ref.get ()->mute ()->setMuted (true);
  midi_clip_ref.get ()->mute ()->setMuted (true);

  selection_model_->clear ();
  select_object (note_ref);
  select_object (midi_clip_ref);

  bool result = operator_->toggleMute ();
  EXPECT_TRUE (result);

  EXPECT_FALSE (note_ref.get ()->mute ()->muted ());
  EXPECT_FALSE (midi_clip_ref.get ()->mute ()->muted ());
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test toggleMute with no selection
TEST_F (ArrangerObjectSelectionOperatorTest, ToggleMuteNoSelection)
{
  selection_model_->clear ();

  bool result = operator_->toggleMute ();
  EXPECT_FALSE (result);
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test toggleMute undo/redo
TEST_F (ArrangerObjectSelectionOperatorTest, ToggleMuteUndoRedo)
{
  selection_model_->clear ();
  select_object (midi_clip_ref);

  EXPECT_FALSE (midi_clip_ref.get ()->mute ()->muted ());

  operator_->toggleMute ();
  EXPECT_TRUE (midi_clip_ref.get ()->mute ()->muted ());

  undo_stack_->undo ();
  EXPECT_FALSE (midi_clip_ref.get ()->mute ()->muted ());

  undo_stack_->redo ();
  EXPECT_TRUE (midi_clip_ref.get ()->mute ()->muted ());
}

// Test toggleMute skips non-muteable objects (markers have no Mute feature)
TEST_F (ArrangerObjectSelectionOperatorTest, ToggleMuteSkipsNonMuteableObjects)
{
  selection_model_->clear ();
  select_object (marker_ref);
  select_object (note_ref);

  EXPECT_EQ (marker_ref.get ()->mute (), nullptr);
  EXPECT_FALSE (note_ref.get ()->mute ()->muted ());

  bool result = operator_->toggleMute ();
  EXPECT_TRUE (result);

  EXPECT_TRUE (note_ref.get ()->mute ()->muted ());
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test toggleMute with only non-muteable objects
TEST_F (ArrangerObjectSelectionOperatorTest, ToggleMuteOnlyNonMuteableObjects)
{
  selection_model_->clear ();
  select_object (marker_ref);

  bool result = operator_->toggleMute ();
  EXPECT_FALSE (result);
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test setStretchAlgorithm on a selected AudioClip
TEST_F (ArrangerObjectSelectionOperatorTest, SetStretchAlgorithmOnAudioClip)
{
  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  ASSERT_NE (clip, nullptr);
  ASSERT_EQ (
    clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Polyphonic);

  selection_model_->clear ();
  select_object (audio_clip_ref);

  bool result =
    operator_->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Beats);
  EXPECT_TRUE (result);
  EXPECT_EQ (clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Beats);
  EXPECT_EQ (undo_stack_->index (), 1);

  undo_stack_->undo ();
  EXPECT_EQ (
    clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Polyphonic);

  undo_stack_->redo ();
  EXPECT_EQ (clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Beats);
}

// Test setStretchAlgorithm skips non-audio clips
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  SetStretchAlgorithmSkipsNonAudioClips)
{
  selection_model_->clear ();
  // Select marker (index 0) and audio clip (index 3)
  select_object (marker_ref);
  select_object (audio_clip_ref);

  bool result =
    operator_->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Monophonic);
  EXPECT_TRUE (result);

  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  EXPECT_EQ (
    clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Monophonic);
}

// Test setStretchAlgorithm with no selection
TEST_F (ArrangerObjectSelectionOperatorTest, SetStretchAlgorithmNoSelection)
{
  selection_model_->clear ();

  bool result =
    operator_->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Beats);
  EXPECT_FALSE (result);
  EXPECT_EQ (undo_stack_->index (), 0);
}

// Test setTimebaseOverride on selected clips
TEST_F (ArrangerObjectSelectionOperatorTest, SetTimebaseOverrideOnClip)
{
  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  ASSERT_NE (clip, nullptr);
  ASSERT_NE (clip->timebaseProvider (), nullptr);
  auto initial = clip->timebaseProvider ()->effectiveTimebase ();

  selection_model_->clear ();
  select_object (audio_clip_ref);

  bool result = operator_->setTimebaseOverride (dsp::Timebase::Absolute);
  EXPECT_TRUE (result);
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);

  undo_stack_->undo ();
  EXPECT_EQ (clip->timebaseProvider ()->effectiveTimebase (), initial);

  undo_stack_->redo ();
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
}

// Test clearTimebaseOverride removes an existing override
TEST_F (ArrangerObjectSelectionOperatorTest, ClearTimebaseOverrideOnClip)
{
  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();

  selection_model_->clear ();
  select_object (audio_clip_ref);

  operator_->setTimebaseOverride (dsp::Timebase::Absolute);
  EXPECT_TRUE (clip->timebaseProvider ()->hasOverride ());

  operator_->clearTimebaseOverride ();
  EXPECT_FALSE (clip->timebaseProvider ()->hasOverride ());

  // Undo clears, then redo clears again
  undo_stack_->undo ();
  EXPECT_TRUE (clip->timebaseProvider ()->hasOverride ());
  undo_stack_->redo ();
  EXPECT_FALSE (clip->timebaseProvider ()->hasOverride ());
}

// Test selectionHasTimebaseProviders
TEST_F (ArrangerObjectSelectionOperatorTest, SelectionHasTimebaseProviders)
{
  selection_model_->clear ();
  EXPECT_FALSE (operator_->selectionHasTimebaseProviders ());

  // Audio clip (index 3) has a timebase provider
  select_object (audio_clip_ref);
  EXPECT_TRUE (operator_->selectionHasTimebaseProviders ());
}

// changeVelocities must clamp to [0,127] instead of rejecting, so a single
// deferred commit on drag release never no-ops when overshooting.
TEST_F (ArrangerObjectSelectionOperatorTest, ChangeVelocitiesClampsToMax)
{
  note_ref.get_object_as<structure::arrangement::MidiNote> ()->setVelocity (120);
  select_object (note_ref);

  const bool result = operator_->changeVelocities (20); // 120 + 20 = 140

  EXPECT_TRUE (result);
  EXPECT_EQ (
    note_ref.get_object_as<structure::arrangement::MidiNote> ()->velocity (),
    127);
  EXPECT_EQ (undo_stack_->index (), 1);
}

TEST_F (ArrangerObjectSelectionOperatorTest, ChangeVelocitiesClampsToMin)
{
  note_ref.get_object_as<structure::arrangement::MidiNote> ()->setVelocity (5);
  select_object (note_ref);

  const bool result = operator_->changeVelocities (-20); // 5 - 20 = -15

  EXPECT_TRUE (result);
  EXPECT_EQ (
    note_ref.get_object_as<structure::arrangement::MidiNote> ()->velocity (), 0);
  EXPECT_EQ (undo_stack_->index (), 1);
}

// ========================================================================
// Cut tool tests
// ========================================================================

// Test cutObjectsAt splits a looped MIDI clip transparently
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtSplitsLoopedMidiClip)
{
  select_object (midi_clip_ref);

  // Fixture MIDI clip: position 2000, length 4000, clip start 500, loop
  // [1000, 3000) - spans timeline [2000, 6000), cut at 4500.
  // Custom loop ranges imply bounds-tracking is off (as in production)
  midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
    ->setTrackBounds (false);
  const bool result = operator_->cutObjectsAt (4500.0);
  EXPECT_TRUE (result);

  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);

  // Left half: original resized to end at the cut, loop range untouched
  EXPECT_DOUBLE_EQ (clip->position ()->ticks (), 2000.0);
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 2500.0);
  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 3000.0);

  // Right half added to the same owner
  const auto &children = mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiClip>::get_children_vector ();
  ASSERT_EQ (children.size (), 2);
  auto * right =
    find_child_at<structure::arrangement::MidiClip> (children, 4500.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 1500.0);
  // Content position at the cut mapped through the loop:
  // clip_start + offset = 500 + 2500 = 3000 >= loop_end (3000), so wrap by
  // the loop size (2000) -> 1000
  EXPECT_DOUBLE_EQ (right->clipStartPosition ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (right->loopStartPosition ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (right->loopEndPosition ()->ticks (), 3000.0);
  EXPECT_FALSE (right->trackBounds ());

  // Single undo step (macro)
  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test cutObjectsAt on a clip with default (tracked) loop bounds
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtSplitsTrackBoundsClip)
{
  auto clip_ref = utils::create_object<structure::arrangement::MidiClip> (
    registry_, *tempo_map_wrapper, registry_);
  auto * clip = clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  clip->position ()->setTicks (0.0);
  clip->length ()->setTicks (2000.0); // trackBounds -> loop [0, 2000]
  mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiClip>::add_object (clip_ref);
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    clip_ref);

  select_object (clip_ref);

  const bool result = operator_->cutObjectsAt (800.0);
  EXPECT_TRUE (result);

  // Left half: resized, bounds re-tracked (loop = [0, new length])
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 800.0);
  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 800.0);
  EXPECT_TRUE (clip->trackBounds ());

  // Right half: starts at the cut, keeps the original loop range
  const auto &children = mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiClip>::get_children_vector ();
  ASSERT_EQ (children.size (), 3);
  auto * right =
    find_child_at<structure::arrangement::MidiClip> (children, 800.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 1200.0);
  EXPECT_DOUBLE_EQ (right->clipStartPosition ()->ticks (), 800.0);
  EXPECT_DOUBLE_EQ (right->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (right->loopEndPosition ()->ticks (), 2000.0);
  EXPECT_FALSE (right->trackBounds ());
}

// Test cutObjectsAt splits a MIDI note inside a clip
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtSplitsMidiNote)
{
  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);

  // Note at content [500, 1500) -> timeline [2500, 3500) (clip at 2000,
  // identity warp)
  auto   note_in_clip_ref = add_note_to_clip (*clip, 500.0, 1000.0);
  auto * note_in_clip =
    note_in_clip_ref.get_object_as<structure::arrangement::MidiNote> ();
  note_in_clip->setPitch (64);
  note_in_clip->setVelocity (100);
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    note_in_clip_ref);

  select_object (note_in_clip_ref);

  const bool result = operator_->cutObjectsAt (3000.0);
  EXPECT_TRUE (result);

  // Left half: truncated at the cut
  EXPECT_DOUBLE_EQ (note_in_clip->position ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (note_in_clip->length ()->ticks (), 500.0);

  // Right half inside the same clip, keeping pitch/velocity
  const auto &notes = clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiNote>::get_children_vector ();
  ASSERT_EQ (notes.size (), 2);
  // Content position at the cut = 3000 - 2000 = 1000
  auto * right = find_child_at<structure::arrangement::MidiNote> (notes, 1000.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 500.0);
  EXPECT_EQ (right->pitch (), 64);
  EXPECT_EQ (right->velocity (), 100);

  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test that notes not inside a clip are not cut
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtSkipsMidiNoteWithoutClip)
{
  // Fixture note: position 1000, length 4000, not inside a clip
  select_object (note_ref);

  const bool result = operator_->cutObjectsAt (3000.0);
  EXPECT_FALSE (result);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test cutAllObjectsAt cuts across multiple owners via the enumerator
TEST_F (ArrangerObjectSelectionOperatorTest, CutAllObjectsAtAcrossOwners)
{
  // Fixture audio clip spans [3000, 5000)
  audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ()
    ->length ()
    ->setTicks (2000.0);

  // Cut at 4500: hits the MIDI clip [2000, 6000) and the audio clip
  // [3000, 5000); skips the marker/tempo/time signature (point objects) and
  // the note (not inside a clip).
  // Custom loop ranges imply bounds-tracking is off (as in production)
  midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
    ->setTrackBounds (false);
  const bool result = operator_->cutAllObjectsAt (4500.0, nullptr);
  EXPECT_TRUE (result);

  const auto &midi_children =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiClip>::get_children_vector ();
  EXPECT_EQ (midi_children.size (), 2);
  const auto &audio_children =
    mock_owner_->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AudioClip>::get_children_vector ();
  EXPECT_EQ (audio_children.size (), 2);

  // Single undo step for the whole operation
  EXPECT_EQ (undo_stack_->index (), 1);

  // Undo restores both originals
  undo_stack_->undo ();
  EXPECT_EQ (midi_children.size (), 1);
  EXPECT_EQ (audio_children.size (), 1);
  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4000.0);
  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 3000.0);
}

// Test cutAllObjectsAt cuts the bounded children of a clip (editor context)
TEST_F (ArrangerObjectSelectionOperatorTest, CutAllObjectsAtInsideClip)
{
  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);

  // Note A at content [500, 1500) -> timeline [2500, 3500) - spans the cut
  auto note_a_ref = add_note_to_clip (*clip, 500.0, 1000.0);
  // Note B at content [1500, 2500) -> timeline [3500, 4500) - after the cut
  add_note_to_clip (*clip, 1500.0, 1000.0);

  const bool result = operator_->cutAllObjectsAt (3000.0, clip);
  EXPECT_TRUE (result);

  // Note A split into two, note B untouched
  const auto &notes = clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiNote>::get_children_vector ();
  ASSERT_EQ (notes.size (), 3);
  EXPECT_DOUBLE_EQ (note_a_ref.get ()->length ()->ticks (), 500.0);
  auto * right = find_child_at<structure::arrangement::MidiNote> (notes, 1000.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 500.0);

  EXPECT_EQ (undo_stack_->index (), 1);
}

// Test cutAllObjectsAt splits a chord object: chords are unbounded but
// effectively span until the next chord or the clip's end
TEST_F (ArrangerObjectSelectionOperatorTest, CutAllObjectsAtSplitsChordObject)
{
  auto chord_clip_ref = utils::create_object<structure::arrangement::ChordClip> (
    registry_, *tempo_map_wrapper, registry_);
  auto * clip =
    chord_clip_ref.get_object_as<structure::arrangement::ChordClip> ();
  clip->position ()->setTicks (2000.0);
  clip->length ()->setTicks (4000.0);

  // Chord A at content 500 (effective span [500, 2500)), chord B at 2500
  // (effective span [2500, clip end))
  auto chord_a_ref = add_chord_to_clip (*clip, 500.0);
  auto chord_b_ref = add_chord_to_clip (*clip, 2500.0);

  // Cut at timeline 3000 = content 1000 (identity warp, clip at 2000):
  // inside chord A's effective span
  EXPECT_TRUE (operator_->cutAllObjectsAt (3000.0, clip));

  const auto &chords = clip->get_children_vector ();
  ASSERT_EQ (chords.size (), 3);
  // Originals untouched
  EXPECT_DOUBLE_EQ (chord_a_ref.get ()->position ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (chord_b_ref.get ()->position ()->ticks (), 2500.0);
  // New chord starts at the cut (content 1000) and carries a copy of chord
  // A's descriptor (no bass)
  auto * new_chord =
    find_child_at<structure::arrangement::ChordObject> (chords, 1000.0);
  ASSERT_NE (new_chord, nullptr);
  EXPECT_NE (
    new_chord->chordDescriptor (),
    chord_a_ref.get_object_as<structure::arrangement::ChordObject> ()
      ->chordDescriptor ());
  EXPECT_EQ (
    new_chord->chordDescriptor ()->hasBass (),
    chord_a_ref.get_object_as<structure::arrangement::ChordObject> ()
      ->chordDescriptor ()
      ->hasBass ());

  EXPECT_EQ (undo_stack_->index (), 1);
  undo_stack_->undo ();
  EXPECT_EQ (chords.size (), 2);
}

// Test the boundaries of a chord object's effective span
TEST_F (ArrangerObjectSelectionOperatorTest, CutAllObjectsAtChordBoundaries)
{
  auto chord_clip_ref = utils::create_object<structure::arrangement::ChordClip> (
    registry_, *tempo_map_wrapper, registry_);
  auto * clip =
    chord_clip_ref.get_object_as<structure::arrangement::ChordClip> ();
  clip->position ()->setTicks (2000.0);
  clip->length ()->setTicks (4000.0);

  add_chord_to_clip (*clip, 500.0);
  add_chord_to_clip (*clip, 2500.0);

  // Before chord A's start (content 100 -> timeline 2100): no chord spans
  EXPECT_FALSE (operator_->cutAllObjectsAt (2100.0, clip));
  // Exactly at chord B's start (content 2500 -> timeline 4500)
  EXPECT_FALSE (operator_->cutAllObjectsAt (4500.0, clip));
  // Exactly at the clip's end (timeline 6000)
  EXPECT_FALSE (operator_->cutAllObjectsAt (6000.0, clip));
  EXPECT_EQ (clip->get_children_vector ().size (), 2);
  EXPECT_EQ (undo_stack_->count (), 0);

  // After chord B, within the clip (content 3000 -> timeline 5000): inside
  // chord B's effective span (no next chord, ends at the clip's end)
  EXPECT_TRUE (operator_->cutAllObjectsAt (5000.0, clip));
  const auto &chords = clip->get_children_vector ();
  ASSERT_EQ (chords.size (), 3);
  auto * new_chord =
    find_child_at<structure::arrangement::ChordObject> (chords, 3000.0);
  ASSERT_NE (new_chord, nullptr);
}

// Test that cutting at or outside object boundaries is a no-op
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtBoundariesIsNoOp)
{
  select_object (midi_clip_ref);

  EXPECT_FALSE (operator_->cutObjectsAt (2000.0)); // exactly at start
  EXPECT_FALSE (operator_->cutObjectsAt (6000.0)); // exactly at end
  EXPECT_FALSE (operator_->cutObjectsAt (1000.0)); // outside
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test that cutting an audio clip shifts the right half's fade offsets
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtAudioClipAdjustsFades)
{
  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  ASSERT_NE (clip, nullptr);
  clip->length ()->setTicks (2000.0); // spans [3000, 5000)
  clip->fadeRange ()->startOffset ()->setTicks (1500.0);
  clip->fadeRange ()->endOffset ()->setTicks (1800.0);

  select_object (audio_clip_ref);

  const bool result = operator_->cutObjectsAt (4000.0);
  EXPECT_TRUE (result);

  // Left half fades unchanged
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (clip->fadeRange ()->startOffset ()->ticks (), 1500.0);
  EXPECT_DOUBLE_EQ (clip->fadeRange ()->endOffset ()->ticks (), 1800.0);

  // Right half fades shifted by the cut offset (1000)
  const auto &children = mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::AudioClip>::get_children_vector ();
  ASSERT_EQ (children.size (), 2);
  auto * right =
    find_child_at<structure::arrangement::AudioClip> (children, 4000.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (right->fadeRange ()->startOffset ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (right->fadeRange ()->endOffset ()->ticks (), 800.0);
}

// Test undo/redo of a cut restores the original object exactly
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtUndoRedo)
{
  select_object (midi_clip_ref);
  midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
    ->setTrackBounds (false);
  ASSERT_TRUE (operator_->cutObjectsAt (4500.0));

  const auto &children = mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiClip>::get_children_vector ();
  ASSERT_EQ (children.size (), 2);

  undo_stack_->undo ();
  EXPECT_EQ (children.size (), 1);
  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4000.0);
  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 3000.0);
  EXPECT_FALSE (clip->trackBounds ());

  undo_stack_->redo ();
  ASSERT_EQ (children.size (), 2);
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 2500.0);
  auto * right =
    find_child_at<structure::arrangement::MidiClip> (children, 4500.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 1500.0);
  EXPECT_DOUBLE_EQ (right->clipStartPosition ()->ticks (), 1000.0);
}

// Test that a failed owner lookup leaves the object untouched and reports
// failure (no partial cut)
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtWithMissingOwnerIsNoOp)
{
  auto null_owner_provider =
    [] (structure::arrangement::ArrangerObjectPtrVariant)
    -> ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
    return static_cast<structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiClip> *> (nullptr);
  };
  ArrangerObjectSelectionOperator op_with_no_owner (
    *undo_stack_, *selection_model_, null_owner_provider, *factory);

  select_object (midi_clip_ref);
  midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ()
    ->setTrackBounds (false);

  EXPECT_FALSE (op_with_no_owner.cutObjectsAt (4500.0));

  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4000.0);
  const auto &children = mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiClip>::get_children_vector ();
  EXPECT_EQ (children.size (), 1);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test that cutting a Source-mode clip (non-linear warp from a tempo change)
// computes the right half's clip start in the unwound content domain,
// matching playback
TEST_F (ArrangerObjectSelectionOperatorTest, CutObjectsAtWarpedLoopedClip)
{
  // Tempo change mid-clip: 120 BPM until tick 1920 (1 second), then 240 BPM
  tempo_map_wrapper->addTempoEvent (
    1920.0, 240.0, dsp::TempoEventWrapper::CurveType::Constant);

  auto clip_ref = utils::create_object<structure::arrangement::MidiClip> (
    registry_, *tempo_map_wrapper, registry_);
  auto * clip = clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  clip->position ()->setTicks (0.0);
  clip->length ()->setTicks (3840.0);
  // Source mode: content is anchored to wall-clock time at the source tempo
  clip->set_source_bpm (units::bpm (120.0));
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (1920.0) });
  mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiClip>::add_object (clip_ref);
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    clip_ref);
  select_object (clip_ref);

  // The clip spans timeline [0, 5760): content [0,1920] (1s at 120 BPM) ->
  // [0,1920), content [1920,3840] (1s at 240 BPM) -> [1920,5760).
  // Cut at 4800: 1.75s into the source -> unwound content position 3360,
  // wrapped by the loop size (1920) -> 1440
  ASSERT_TRUE (operator_->cutObjectsAt (4800.0));

  // Left half ends at the cut (content 3360)
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 3360.0);

  const auto &children = mock_owner_->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiClip>::get_children_vector ();
  ASSERT_EQ (children.size (), 3);
  auto * right =
    find_child_at<structure::arrangement::MidiClip> (children, 4800.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->clipStartPosition ()->ticks (), 1440.0);
  EXPECT_DOUBLE_EQ (right->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (right->loopEndPosition ()->ticks (), 1920.0);
  // Ends at the original's timeline end: 5760 is 2s into the source, the
  // clone's start (4800) is 1.75s, so 0.25s = 480 content ticks
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 480.0);
}

// Test that cutting a note in a clip with a nonzero clip start splits the
// note at the unwound content position under the cut (the clip editor's
// content coordinate space), regardless of the clip start
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  CutObjectsAtSplitsMidiNoteWithClipStart)
{
  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);
  // Fixture clip: position 2000, length 4000; playback starts at content 500
  // and the loop covers the whole content so no wrapping occurs
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (500.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (4000.0) });

  // Note at content [1000, 2000) -> displayed at timeline [3000, 4000)
  auto note_in_clip_ref = add_note_to_clip (*clip, 1000.0, 1000.0);
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    note_in_clip_ref);
  select_object (note_in_clip_ref);

  // Cut at 3250: the unwound content position under the cut is 1250
  ASSERT_TRUE (operator_->cutObjectsAt (3250.0));

  // Left half ends at the cut
  EXPECT_DOUBLE_EQ (note_in_clip_ref.get ()->position ()->ticks (), 1000.0);
  EXPECT_DOUBLE_EQ (note_in_clip_ref.get ()->length ()->ticks (), 250.0);

  // Right half starts at the cut
  const auto &notes = clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiNote>::get_children_vector ();
  ASSERT_EQ (notes.size (), 2);
  auto * right = find_child_at<structure::arrangement::MidiNote> (notes, 1250.0);
  ASSERT_NE (right, nullptr);
  EXPECT_DOUBLE_EQ (right->length ()->ticks (), 750.0);
}

// Test that cutting a chord in a clip with a nonzero clip start places the
// clone at the unwound content position under the cut (the clip editor's
// content coordinate space), regardless of the clip start
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  CutAllObjectsAtSplitsChordWithClipStart)
{
  auto chord_clip_ref = utils::create_object<structure::arrangement::ChordClip> (
    registry_, *tempo_map_wrapper, registry_);
  auto * clip =
    chord_clip_ref.get_object_as<structure::arrangement::ChordClip> ();
  clip->position ()->setTicks (2000.0);
  clip->length ()->setTicks (4000.0);
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (500.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (4000.0) });
  add_chord_to_clip (*clip, 1000.0);

  // Cut at timeline 3250: the unwound content position under the cut is
  // 1250, inside the chord's effective span [1000, clip end)
  ASSERT_TRUE (operator_->cutAllObjectsAt (3250.0, clip));

  const auto &chords = clip->get_children_vector ();
  ASSERT_EQ (chords.size (), 2);
  auto * new_chord =
    find_child_at<structure::arrangement::ChordObject> (chords, 1250.0);
  EXPECT_NE (new_chord, nullptr);
}

} // namespace zrythm::actions
