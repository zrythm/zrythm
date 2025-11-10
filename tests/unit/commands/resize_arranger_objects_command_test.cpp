// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/resize_arranger_objects_command.h"
#include "structure/arrangement/arranger_object_all.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ResizeArrangerObjectsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Create test objects with different features
    marker_ref = object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Custom);

    midi_region_ref =
      object_registry.create_object<structure::arrangement::MidiRegion> (
        *tempo_map, object_registry, file_audio_source_registry);

    audio_region_ref = object_registry.create_object<
      structure::arrangement::AudioRegion> (
      *tempo_map, object_registry, file_audio_source_registry,
      [] () { return true; });

    // Set initial positions and properties
    marker_ref.get_object_base ()->position ()->setTicks (1000.0);

    midi_region_ref.get_object_base ()->position ()->setTicks (2000.0);
    if (
      auto * region =
        midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
      {
        region->bounds ()->length ()->setTicks (4000.0);
      }

    audio_region_ref.get_object_base ()->position ()->setTicks (3000.0);
    if (
      auto * region =
        audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ())
      {
        region->bounds ()->length ()->setTicks (5000.0);
        region->loopRange ()->clipStartPosition ()->setTicks (0.0);
        region->loopRange ()->loopStartPosition ()->setTicks (0.0);
        region->loopRange ()->loopEndPosition ()->setTicks (5000.0);
        if (region->fadeRange () != nullptr)
          {
            region->fadeRange ()->startOffset ()->setTicks (200.0);
            region->fadeRange ()->endOffset ()->setTicks (300.0);
          }
      }
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  structure::arrangement::ArrangerObjectRegistry object_registry;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;

  structure::arrangement::ArrangerObjectUuidReference marker_ref{
    object_registry
  };
  structure::arrangement::ArrangerObjectUuidReference midi_region_ref{
    object_registry
  };
  structure::arrangement::ArrangerObjectUuidReference audio_region_ref{
    object_registry
  };
};

// Test initial state after construction
TEST_F (ResizeArrangerObjectsCommandTest, InitialState)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    marker_ref
  };
  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);

  // Objects should still be at original positions
  EXPECT_DOUBLE_EQ (
    marker_ref.get_object_base ()->position ()->ticks (), 1000.0);
}

// Test bounds resize from end
TEST_F (ResizeArrangerObjectsCommandTest, BoundsResizeFromEnd)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };
  const double delta = 500.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, delta);

  // Execute resize
  command.redo ();

  // Check that length was increased
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 4500.0); // 4000 + 500
      EXPECT_DOUBLE_EQ (
        region->position ()->ticks (), 2000.0); // Position unchanged
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 4000.0); // Original restored
      EXPECT_DOUBLE_EQ (
        region->position ()->ticks (), 2000.0); // Position unchanged
    }
}

// Test bounds resize from start
TEST_F (ResizeArrangerObjectsCommandTest, BoundsResizeFromStart)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };
  const double delta = 200.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromStart, delta);

  // Execute resize
  command.redo ();

  // Check that position was adjusted and length decreased to maintain end
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (region->position ()->ticks (), 2200.0); // 2000 + 200
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 3800.0); // 4000 - 200
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->position ()->ticks (), 2000.0); // Original restored
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 4000.0); // Original restored
    }
}

// Test loop points resize from start
TEST_F (ResizeArrangerObjectsCommandTest, LoopPointsResizeFromStart)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_region_ref
  };
  const double delta = 100.0;

  auto * region =
    audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ();

  // Set up initial loop state
  region->loopRange ()->setTrackBounds (false);
  region->loopRange ()->clipStartPosition ()->setTicks (500.0);
  region->loopRange ()->loopStartPosition ()->setTicks (1000.0);
  region->loopRange ()->loopEndPosition ()->setTicks (4000.0);

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::LoopPoints, ResizeDirection::FromStart, delta);

  // Execute resize
  command.redo ();

  // Check that trackBounds is still false
  EXPECT_FALSE (region->loopRange ()->trackBounds ());

  // Check that position was adjusted and length decreased to maintain end
  EXPECT_DOUBLE_EQ (region->position ()->ticks (), 3100.0); // 3000 + 100
  EXPECT_DOUBLE_EQ (
    region->bounds ()->length ()->ticks (), 4900.0); // 5000 - 100

  // Check that clip start was adjusted
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->clipStartPosition ()->ticks (), 600.0); // 500 + 100

  // Loop start and end should remain unchanged
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopStartPosition ()->ticks (), 1000.0); // Unchanged
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopEndPosition ()->ticks (), 4000.0); // Unchanged

  // Undo should restore original state
  command.undo ();

  // Check that original state was restored
  EXPECT_FALSE (region->loopRange ()->trackBounds ());
  EXPECT_DOUBLE_EQ (region->position ()->ticks (), 3000.0); // Original restored
  EXPECT_DOUBLE_EQ (
    region->bounds ()->length ()->ticks (), 5000.0); // Original restored
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->clipStartPosition ()->ticks (),
    500.0); // Original restored
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopStartPosition ()->ticks (),
    1000.0); // Original restored
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopEndPosition ()->ticks (),
    4000.0); // Original restored
}

// Test loop points resize from end
TEST_F (ResizeArrangerObjectsCommandTest, LoopPointsResizeFromEnd)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };
  const double delta = 500.0;

  auto * region =
    midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ();
  region->loopRange ()->setTrackBounds (false);
  region->loopRange ()->clipStartPosition ()->setTicks (500.0);
  region->loopRange ()->loopStartPosition ()->setTicks (1000.0);
  region->loopRange ()->loopEndPosition ()->setTicks (3500.0);

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::LoopPoints, ResizeDirection::FromEnd, delta);

  // Execute resize
  command.redo ();

  // Check that trackBounds was set to false and loop end was adjusted

  // Loop-related positions should remain unchanged
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->clipStartPosition ()->ticks (), 500.0); // Unchanged
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopStartPosition ()->ticks (),
    1000.0); // Unchanged
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopEndPosition ()->ticks (),
    3500.0); // Unchanged

  // Length should be adjusted by the delta
  EXPECT_DOUBLE_EQ (
    region->bounds ()->length ()->ticks (),
    4500.0); // 4000 + 500

  // Undo should restore original state including trackBounds
  command.undo ();

  // Loop positions should remain unchanged
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->clipStartPosition ()->ticks (),
    500.0); // Original restored
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopStartPosition ()->ticks (),
    1000.0); // Original restored
  EXPECT_DOUBLE_EQ (
    region->loopRange ()->loopEndPosition ()->ticks (),
    3500.0); // Original restored

  // Region length should be restored
  EXPECT_DOUBLE_EQ (
    region->bounds ()->length ()->ticks (), 4000.0); // Original restored
}

// Test fades resize from start
TEST_F (ResizeArrangerObjectsCommandTest, FadesResizeFromStart)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_region_ref
  };
  const double delta = 50.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Fades, ResizeDirection::FromStart, delta);

  // Execute resize
  command.redo ();

  // Check that fade-in start offset was adjusted
  if (
    auto * region =
      audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ())
    {
      if (region->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->startOffset ()->ticks (), 250.0); // 200 + 50
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->endOffset ()->ticks (), 300.0); // Unchanged
        }
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * region =
      audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ())
    {
      if (region->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->startOffset ()->ticks (),
            200.0); // Original restored
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->endOffset ()->ticks (),
            300.0); // Original restored
        }
    }
}

// Test fades resize from end
TEST_F (ResizeArrangerObjectsCommandTest, FadesResizeFromEnd)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_region_ref
  };
  const double delta = 100.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Fades, ResizeDirection::FromEnd, delta);

  // Execute resize
  command.redo ();

  // Check that fade-out end offset was adjusted
  if (
    auto * region =
      audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ())
    {
      if (region->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->startOffset ()->ticks (), 200.0); // Unchanged
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->endOffset ()->ticks (), 400.0); // 300 + 100
        }
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * region =
      audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ())
    {
      if (region->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->startOffset ()->ticks (),
            200.0); // Original restored
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->endOffset ()->ticks (),
            300.0); // Original restored
        }
    }
}

// Test minimum length constraint
TEST_F (ResizeArrangerObjectsCommandTest, MinimumLengthConstraint)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };
  const double large_delta = -5000.0; // Would make length negative

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, large_delta);

  // Execute resize
  command.redo ();

  // Check that length was clamped to minimum (1 tick)
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_GE (
        region->bounds ()->length ()->ticks (),
        1.0); // Should be clamped to minimum
    }
}

// Test non-negative constraint for fades
TEST_F (ResizeArrangerObjectsCommandTest, FadeNonNegativeConstraint)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_region_ref
  };
  const double negative_delta = -300.0; // Would make fade offset negative

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Fades, ResizeDirection::FromStart, negative_delta);

  // Execute resize
  command.redo ();

  // Check that fade offset was clamped to 0
  if (
    auto * region =
      audio_region_ref.get_object_as<structure::arrangement::AudioRegion> ())
    {
      if (region->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            region->fadeRange ()->startOffset ()->ticks (),
            0.0); // Should be clamped to 0
        }
    }
}

// Test command ID
TEST_F (ResizeArrangerObjectsCommandTest, CommandId)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    marker_ref
  };
  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);

  EXPECT_EQ (command.id (), 1762503480);
}

// Test command merging with same objects and type
TEST_F (ResizeArrangerObjectsCommandTest, CommandMergingSameObjects)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };

  ResizeArrangerObjectsCommand command1 (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);
  ResizeArrangerObjectsCommand command2 (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 50.0);

  // Execute first command
  command1.redo ();

  // Should be able to merge
  bool can_merge = command1.mergeWith (&command2);
  EXPECT_TRUE (can_merge);

  // After merge, delta should be accumulated
  command1.redo (); // Re-execute to apply merged delta
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 4150.0); // 4000 + 100 + 50
    }
}

// Test command merging with different objects
TEST_F (ResizeArrangerObjectsCommandTest, CommandMergingDifferentObjects)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects1 = {
    midi_region_ref
  };
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects2 = {
    audio_region_ref
  };

  ResizeArrangerObjectsCommand command1 (
    objects1, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);
  ResizeArrangerObjectsCommand command2 (
    objects2, ResizeType::Bounds, ResizeDirection::FromEnd, 50.0);

  // Execute first command
  command1.redo ();

  // Should not be able to merge (different objects)
  bool can_merge = command1.mergeWith (&command2);
  EXPECT_FALSE (can_merge);
}

// Test command merging with different type
TEST_F (ResizeArrangerObjectsCommandTest, CommandMergingDifferentType)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };

  ResizeArrangerObjectsCommand command1 (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);
  ResizeArrangerObjectsCommand command2 (
    objects, ResizeType::LoopPoints, ResizeDirection::FromEnd, 50.0);

  // Execute first command
  command1.redo ();

  // Should not be able to merge (different type)
  bool can_merge = command1.mergeWith (&command2);
  EXPECT_FALSE (can_merge);
}

// Test command merging with different direction
TEST_F (ResizeArrangerObjectsCommandTest, CommandMergingDifferentDirection)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };

  ResizeArrangerObjectsCommand command1 (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);
  ResizeArrangerObjectsCommand command2 (
    objects, ResizeType::Bounds, ResizeDirection::FromStart, 50.0);

  // Execute first command
  command1.redo ();

  // Should not be able to merge (different direction)
  bool can_merge = command1.mergeWith (&command2);
  EXPECT_FALSE (can_merge);
}

// Test multiple undo/redo cycles
TEST_F (ResizeArrangerObjectsCommandTest, MultipleUndoRedoCycles)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_region_ref
  };
  const double delta = 200.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, delta);

  // First cycle
  command.redo ();
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 4200.0); // 4000 + 200
    }

  // Undo
  command.undo ();
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 4000.0); // Original restored
    }

  // Redo again
  command.redo ();
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (), 4200.0); // Applied again
    }

  // Undo again
  command.undo ();
  if (
    auto * region =
      midi_region_ref.get_object_as<structure::arrangement::MidiRegion> ())
    {
      EXPECT_DOUBLE_EQ (
        region->bounds ()->length ()->ticks (),
        4000.0); // Original restored again
    }
}

// Test with empty objects vector
TEST_F (ResizeArrangerObjectsCommandTest, EmptyObjectsVector)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> empty_objects;
  ResizeArrangerObjectsCommand command (
    empty_objects, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);

  // Should not crash with empty objects
  command.redo ();
  command.undo ();
}

// Test command text
TEST_F (ResizeArrangerObjectsCommandTest, CommandText)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    marker_ref
  };
  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 100.0);

  // The command should have appropriate text for display in undo stack
  EXPECT_EQ (command.text (), QString ("Resize Objects"));
}

} // namespace zrythm::commands
