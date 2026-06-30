// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/resize_arranger_objects_command.h"
#include "dsp/content_time_warp.h"
#include "dsp/timebase.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ResizeArrangerObjectsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper,
        .registry_ = object_registry,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    // Create test objects with different features
    auto marker_builder =
      factory->get_builder<structure::arrangement::Marker> ();
    marker_ref = marker_builder.build_in_registry ();

    auto midi_clip_builder =
      factory->get_builder<structure::arrangement::MidiClip> ();
    midi_clip_ref = midi_clip_builder.build_in_registry ();

    auto audio_clip_builder =
      factory->get_builder<structure::arrangement::AudioClip> ();
    audio_clip_ref = audio_clip_builder.build_in_registry ();

    // Set initial positions and properties
    marker_ref.get ()->position ()->setTicks (1000.0);

    midi_clip_ref.get ()->position ()->setTicks (2000.0);
    if (
      auto * clip =
        midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
      {
        clip->length ()->setTicks (4000.0);
      }

    audio_clip_ref.get ()->position ()->setTicks (3000.0);
    if (
      auto * clip =
        audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ())
      {
        clip->length ()->setTicks (5000.0);
        clip->clipStartPosition ()->setTicks (0.0);
        clip->loopStartPosition ()->setTicks (0.0);
        clip->loopEndPosition ()->setTicks (5000.0);
        if (clip->fadeRange () != nullptr)
          {
            clip->fadeRange ()->startOffset ()->setTicks (200.0);
            clip->fadeRange ()->endOffset ()->setTicks (300.0);
          }
      }
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 object_registry;
  std::unique_ptr<utils::AppSettings>   app_settings;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory;

  structure::arrangement::ArrangerObjectUuidReference marker_ref{
    object_registry
  };
  structure::arrangement::ArrangerObjectUuidReference midi_clip_ref{
    object_registry
  };
  structure::arrangement::ArrangerObjectUuidReference audio_clip_ref{
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
  EXPECT_DOUBLE_EQ (marker_ref.get ()->position ()->ticks (), 1000.0);
}

// Test bounds resize from end
TEST_F (ResizeArrangerObjectsCommandTest, BoundsResizeFromEnd)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_clip_ref
  };
  const double delta = 500.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, delta);

  // Execute resize
  command.redo ();

  // Check that length was increased
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4500.0); // 4000 + 500
      EXPECT_DOUBLE_EQ (
        clip->position ()->ticks (), 2000.0); // Position unchanged
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4000.0); // Original restored
      EXPECT_DOUBLE_EQ (
        clip->position ()->ticks (), 2000.0); // Position unchanged
    }
}

// Test bounds resize from start
TEST_F (ResizeArrangerObjectsCommandTest, BoundsResizeFromStart)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_clip_ref
  };
  const double delta = 200.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromStart, delta);

  // Execute resize
  command.redo ();

  // Check that position was adjusted and length decreased to maintain end
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (clip->position ()->ticks (), 2200.0); // 2000 + 200
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 3800.0);   // 4000 - 200
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (
        clip->position ()->ticks (), 2000.0);               // Original restored
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4000.0); // Original restored
    }
}

// Undo of a FromStart bounds resize must restore child positions
// even when the first child sits at content position 0.
TEST_F (
  ResizeArrangerObjectsCommandTest,
  BoundsResizeFromStartRestoresChildAtZeroPosition)
{
  auto clip_ref =
    factory->get_builder<structure::arrangement::AutomationClip> ()
      .build_in_registry ();
  auto * clip =
    clip_ref.get_object_as<structure::arrangement::AutomationClip> ();
  ASSERT_NE (clip, nullptr);
  clip->position ()->setTicks (2000.0);
  clip->length ()->setTicks (4000.0);

  auto point_ref =
    factory->get_builder<structure::arrangement::AutomationPoint> ()
      .with_start_ticks (units::ticks (0))
      .with_automatable_value (0.5)
      .build_in_registry ();
  clip->ArrangerObjectOwner<structure::arrangement::AutomationPoint>::add_object (
    point_ref);
  auto * point =
    point_ref.get_object_as<structure::arrangement::AutomationPoint> ();
  ASSERT_NE (point, nullptr);
  ASSERT_DOUBLE_EQ (point->position ()->ticks (), 0.0);

  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    clip_ref
  };
  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromStart, 200.0);
  command.redo ();

  // Redo shifts children by -content_delta (identity warp → -200).
  EXPECT_NEAR (point->position ()->ticks (), -200.0, 0.001);

  command.undo ();

  // Undo must restore the point — including when it sat at position 0.
  EXPECT_NEAR (point->position ()->ticks (), 0.0, 0.001);
}

// Test loop points resize from start
TEST_F (ResizeArrangerObjectsCommandTest, LoopPointsResizeFromStart)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_clip_ref
  };
  const double delta = 100.0;

  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();

  // Set up initial loop state
  clip->setTrackBounds (false);
  clip->clipStartPosition ()->setTicks (500.0);
  clip->loopStartPosition ()->setTicks (1000.0);
  clip->loopEndPosition ()->setTicks (4000.0);

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::LoopPoints, ResizeDirection::FromStart, delta);

  // Execute resize
  command.redo ();

  // Check that trackBounds is still false
  EXPECT_FALSE (clip->trackBounds ());

  // Check that position was adjusted and length decreased to maintain end
  EXPECT_DOUBLE_EQ (clip->position ()->ticks (), 3100.0); // 3000 + 100
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4900.0);   // 5000 - 100

  // Check that clip start was adjusted
  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 600.0); // 500 + 100

  // Loop start and end should remain unchanged
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 1000.0); // Unchanged
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 4000.0);   // Unchanged

  // Undo should restore original state
  command.undo ();

  // Check that original state was restored
  EXPECT_FALSE (clip->trackBounds ());
  EXPECT_DOUBLE_EQ (clip->position ()->ticks (), 3000.0); // Original restored
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 5000.0);   // Original restored
  EXPECT_DOUBLE_EQ (
    clip->clipStartPosition ()->ticks (),
    500.0); // Original restored
  EXPECT_DOUBLE_EQ (
    clip->loopStartPosition ()->ticks (),
    1000.0); // Original restored
  EXPECT_DOUBLE_EQ (
    clip->loopEndPosition ()->ticks (),
    4000.0); // Original restored
}

// Test loop points resize from end
TEST_F (ResizeArrangerObjectsCommandTest, LoopPointsResizeFromEnd)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_clip_ref
  };
  const double delta = 500.0;

  auto * clip = midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  clip->setTrackBounds (false);
  clip->clipStartPosition ()->setTicks (500.0);
  clip->loopStartPosition ()->setTicks (1000.0);
  clip->loopEndPosition ()->setTicks (3500.0);

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::LoopPoints, ResizeDirection::FromEnd, delta);

  // Execute resize
  command.redo ();

  // Check that trackBounds was set to false and loop end was adjusted

  // Loop-related positions should remain unchanged
  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 500.0); // Unchanged
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (),
                    1000.0); // Unchanged
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (),
                    3500.0); // Unchanged

  // Length should be adjusted by the delta
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (),
                    4500.0); // 4000 + 500

  // Undo should restore original state including trackBounds
  command.undo ();

  // Loop positions should remain unchanged
  EXPECT_DOUBLE_EQ (
    clip->clipStartPosition ()->ticks (),
    500.0); // Original restored
  EXPECT_DOUBLE_EQ (
    clip->loopStartPosition ()->ticks (),
    1000.0); // Original restored
  EXPECT_DOUBLE_EQ (
    clip->loopEndPosition ()->ticks (),
    3500.0); // Original restored

  // Clip length should be restored
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4000.0); // Original restored
}

// Test fades resize from start
TEST_F (ResizeArrangerObjectsCommandTest, FadesResizeFromStart)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_clip_ref
  };
  const double delta = 50.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Fades, ResizeDirection::FromStart, delta);

  // Execute resize
  command.redo ();

  // Check that fade-in start offset was adjusted
  if (
    auto * clip =
      audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ())
    {
      if (clip->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->startOffset ()->ticks (), 250.0); // 200 + 50
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->endOffset ()->ticks (), 300.0); // Unchanged
        }
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * clip =
      audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ())
    {
      if (clip->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->startOffset ()->ticks (),
            200.0); // Original restored
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->endOffset ()->ticks (),
            300.0); // Original restored
        }
    }
}

// Test fades resize from end
TEST_F (ResizeArrangerObjectsCommandTest, FadesResizeFromEnd)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_clip_ref
  };
  const double delta = 100.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Fades, ResizeDirection::FromEnd, delta);

  // Execute resize
  command.redo ();

  // Check that fade-out end offset was adjusted
  if (
    auto * clip =
      audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ())
    {
      if (clip->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->startOffset ()->ticks (), 200.0); // Unchanged
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->endOffset ()->ticks (), 400.0); // 300 + 100
        }
    }

  // Undo should restore original
  command.undo ();
  if (
    auto * clip =
      audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ())
    {
      if (clip->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->startOffset ()->ticks (),
            200.0); // Original restored
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->endOffset ()->ticks (),
            300.0); // Original restored
        }
    }
}

// Test minimum length constraint
TEST_F (ResizeArrangerObjectsCommandTest, MinimumLengthConstraint)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    midi_clip_ref
  };
  const double large_delta = -5000.0; // Would make length negative

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, large_delta);

  // Execute resize
  command.redo ();

  // Check that length was clamped to minimum (1 tick)
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_GE (
        clip->length ()->ticks (),
        1.0); // Should be clamped to minimum
    }
}

// Test non-negative constraint for fades
TEST_F (ResizeArrangerObjectsCommandTest, FadeNonNegativeConstraint)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_clip_ref
  };
  const double negative_delta = -300.0; // Would make fade offset negative

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Fades, ResizeDirection::FromStart, negative_delta);

  // Execute resize
  command.redo ();

  // Check that fade offset was clamped to 0
  if (
    auto * clip =
      audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ())
    {
      if (clip->fadeRange () != nullptr)
        {
          EXPECT_DOUBLE_EQ (
            clip->fadeRange ()->startOffset ()->ticks (),
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
    midi_clip_ref
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
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4150.0); // 4000 + 100 + 50
    }
}

// Test command merging with different objects
TEST_F (ResizeArrangerObjectsCommandTest, CommandMergingDifferentObjects)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects1 = {
    midi_clip_ref
  };
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects2 = {
    audio_clip_ref
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
    midi_clip_ref
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
    midi_clip_ref
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
    midi_clip_ref
  };
  const double delta = 200.0;

  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, delta);

  // First cycle
  command.redo ();
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4200.0); // 4000 + 200
    }

  // Undo
  command.undo ();
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4000.0); // Original restored
    }

  // Redo again
  command.redo ();
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (clip->length ()->ticks (), 4200.0); // Applied again
    }

  // Undo again
  command.undo ();
  if (
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ())
    {
      EXPECT_DOUBLE_EQ (
        clip->length ()->ticks (),
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

// Test bounds resize from end with Absolute timebase (non-identity warp).
// The resize delta arrives in timeline ticks; the content length is in
// content ticks. The command must convert the delta so that the visual
// (timeline) length change matches the drag distance.
TEST_F (ResizeArrangerObjectsCommandTest, BoundsResizeFromEndAbsoluteMode)
{
  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  ASSERT_NE (clip, nullptr);
  ASSERT_NE (clip->contentWarp (), nullptr);

  // Switch to Absolute timebase and configure a Source warp at 60 BPM.
  // Project tempo is 120 BPM (from the fixture's tempo map), so the warp
  // ratio is project_bpm / source_bpm = 120 / 60 = 2.
  // timeline_ticks = content_ticks * 2
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  clip->contentWarp ()->configure_as_source (units::bpm (60.0));

  const double ratio =
    clip->contentWarp ()->contentToTimelineTicksRelative (1.0);
  ASSERT_GT (ratio, 0.0);

  const double original_content_length = clip->length ()->ticks ();

  // Resize FromEnd by 1000 timeline ticks.
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_clip_ref
  };
  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 1000.0);
  command.redo ();

  // Content length should change by 1000 / ratio content ticks.
  const double expected_delta = 1000.0 / ratio;
  const double actual_content_length = clip->length ()->ticks ();
  EXPECT_NEAR (
    actual_content_length, original_content_length + expected_delta, 1.0);

  // Timeline length should change by exactly 1000 ticks.
  const double timeline_delta =
    clip->timelineLengthTicks () - (original_content_length * ratio);
  EXPECT_NEAR (timeline_delta, 1000.0, 2.0);
}

// Test bounds resize from end with Absolute timebase and a tempo change
// inside the clip. The timeline length must change by exactly the requested
// delta even when the tempo varies across the clip's duration.
TEST_F (
  ResizeArrangerObjectsCommandTest,
  BoundsResizeFromEndAbsoluteModeWithTempoChange)
{
  // Add a tempo change: 120 BPM → 240 BPM at tick 3840 (bar 3).
  tempo_map->add_tempo_event (
    units::ticks (3840), units::bpm (240.0), dsp::TempoMap::CurveType::Constant);

  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  ASSERT_NE (clip, nullptr);
  ASSERT_NE (clip->contentWarp (), nullptr);

  // Clip at position 0, source BPM 120, absolute mode.
  clip->position ()->setTicks (0.0);
  clip->length ()->setTicks (7680.0); // 8 bars at 120 BPM = 7680 ticks
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  clip->contentWarp ()->configure_as_source (units::bpm (120.0));

  // Record the timeline length BEFORE resize.
  const double old_timeline_length = clip->timelineLengthTicks ();

  // Resize FromEnd by 960 timeline ticks.
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_clip_ref
  };
  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromEnd, 960.0);
  command.redo ();

  // The timeline length must change by EXACTLY 960 ticks.
  const double new_timeline_length = clip->timelineLengthTicks ();
  EXPECT_NEAR (new_timeline_length - old_timeline_length, 960.0, 2.0);
}

// Test bounds resize from start with Absolute timebase (non-identity warp).
// FromStart must convert the timeline delta through the warp symmetrically with
// FromEnd so the timeline length shrinks by exactly the requested delta.
TEST_F (ResizeArrangerObjectsCommandTest, BoundsResizeFromStartAbsoluteMode)
{
  auto * clip =
    audio_clip_ref.get_object_as<structure::arrangement::AudioClip> ();
  ASSERT_NE (clip, nullptr);
  ASSERT_NE (clip->contentWarp (), nullptr);

  // Absolute mode, source 60 BPM, project 120 BPM → ratio 2.
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  clip->contentWarp ()->configure_as_source (units::bpm (60.0));

  const double ratio =
    clip->contentWarp ()->contentToTimelineTicksRelative (1.0);
  ASSERT_GT (ratio, 0.0);

  const double old_timeline_length = clip->timelineLengthTicks ();

  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects = {
    audio_clip_ref
  };
  ResizeArrangerObjectsCommand command (
    objects, ResizeType::Bounds, ResizeDirection::FromStart, 1000.0);
  command.redo ();

  // Timeline length should shrink by exactly 1000 ticks.
  const double new_timeline_length = clip->timelineLengthTicks ();
  EXPECT_NEAR (old_timeline_length - new_timeline_length, 1000.0, 2.0);
}

} // namespace zrythm::commands
