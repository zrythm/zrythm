// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_operator.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "undo/undo_stack.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/dsp/graph_helpers.h"
#include "unit/structure/tracks/mock_track.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{
class TrackOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track = mock_track_factory.createMockTrack (
      structure::tracks::Track::Type::Audio);
    track->setName ("Original Track Name");
    track->setColor (QColor ("red"));
    undo_stack = create_mock_undo_stack ();
    track_operator = std::make_unique<TrackOperator> ();
    track_operator->setTrack (track.get ());
    track_operator->setUndoStack (undo_stack.get ());
  }

  structure::tracks::MockTrackFactory           mock_track_factory;
  std::unique_ptr<structure::tracks::MockTrack> track;
  std::unique_ptr<undo::UndoStack>              undo_stack;
  std::unique_ptr<TrackOperator>                track_operator;
};

class TrackOperatorRoutingTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Set up dependencies for TrackFactory
    track_registry_ = std::make_unique<structure::tracks::TrackRegistry> ();
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (*track_registry_);

    // Create track factory with dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      file_audio_source_registry_,
      plugin_registry_,
      port_registry_,
      param_registry_,
      obj_registry_,
      *track_registry_,
      transport_,
      soloed_tracks_exist_getter_,
    };
    track_factory_ =
      std::make_unique<structure::tracks::TrackFactory> (factory_deps);

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create track operator
    track_operator_ = std::make_unique<TrackOperator> ();
    track_operator_->setTrackRouting (track_routing_.get ());
    track_operator_->setUndoStack (undo_stack_.get ());
  }

  // Create minimal dependencies for track creation
  dsp::TempoMap                   tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper            tempo_map_wrapper_{ tempo_map_ };
  dsp::FileAudioSourceRegistry    file_audio_source_registry_;
  plugins::PluginRegistry         plugin_registry_;
  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  structure::arrangement::ArrangerObjectRegistry obj_registry_;
  dsp::graph_test::MockTransport                 transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::TrackRegistry> track_registry_;
  std::unique_ptr<structure::tracks::TrackRouting>  track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>  track_factory_;
  std::unique_ptr<undo::UndoStack>                  undo_stack_;
  std::unique_ptr<TrackOperator>                    track_operator_;
};

// ============================================================================
// Basic TrackOperator Tests (using MockTrack)
// ============================================================================

// Test initial state
TEST_F (TrackOperatorTest, InitialState)
{
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
  EXPECT_EQ (undo_stack->count (), 0);
  EXPECT_EQ (undo_stack->index (), 0);
}

// Test basic rename operation
TEST_F (TrackOperatorTest, BasicRename)
{
  track_operator->rename ("New Track Name");

  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test undo after rename
TEST_F (TrackOperatorTest, UndoAfterRename)
{
  track_operator->rename ("New Track Name");
  undo_stack->undo ();

  EXPECT_EQ (track->name (), QString ("Original Track Name"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 0);
  EXPECT_FALSE (undo_stack->canUndo ());
  EXPECT_TRUE (undo_stack->canRedo ());
}

// Test redo after undo
TEST_F (TrackOperatorTest, RedoAfterUndo)
{
  track_operator->rename ("New Track Name");
  undo_stack->undo ();
  undo_stack->redo ();

  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test multiple consecutive renames
TEST_F (TrackOperatorTest, MultipleConsecutiveRenames)
{
  track_operator->rename ("First New Name");
  EXPECT_EQ (track->name (), QString ("First New Name"));
  EXPECT_EQ (undo_stack->count (), 1);

  track_operator->rename ("Second New Name");
  EXPECT_EQ (track->name (), QString ("Second New Name"));
  EXPECT_EQ (undo_stack->count (), 2);

  track_operator->rename ("Third New Name");
  EXPECT_EQ (track->name (), QString ("Third New Name"));
  EXPECT_EQ (undo_stack->count (), 3);

  // Undo all changes
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Second New Name"));

  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("First New Name"));

  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test undo/redo cycles
TEST_F (TrackOperatorTest, UndoRedoCycles)
{
  track_operator->rename ("New Name");

  // First cycle
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));

  undo_stack->redo ();
  EXPECT_EQ (track->name (), QString ("New Name"));

  // Second cycle
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));

  undo_stack->redo ();
  EXPECT_EQ (track->name (), QString ("New Name"));
}

// Test command text in undo stack
TEST_F (TrackOperatorTest, CommandTextInUndoStack)
{
  track_operator->rename ("Test Name");

  EXPECT_EQ (undo_stack->text (0), QString ("Rename Track"));
}

// Test basic setColor operation
TEST_F (TrackOperatorTest, BasicSetColor)
{
  track->setColor (QColor ("#FF0000"));          // Red
  track_operator->setColor (QColor ("#00FF00")); // Green

  EXPECT_EQ (track->color (), QColor ("#00FF00"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test undo after setColor
TEST_F (TrackOperatorTest, UndoAfterSetColor)
{
  track->setColor (QColor ("#FF0000"));
  track_operator->setColor (QColor ("#00FF00"));
  undo_stack->undo ();

  EXPECT_EQ (track->color (), QColor ("#FF0000"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 0);
  EXPECT_FALSE (undo_stack->canUndo ());
  EXPECT_TRUE (undo_stack->canRedo ());
}

// Test redo after undo for setColor
TEST_F (TrackOperatorTest, RedoAfterUndoSetColor)
{
  track->setColor (QColor ("#FF0000"));
  track_operator->setColor (QColor ("#00FF00"));
  undo_stack->undo ();
  undo_stack->redo ();

  EXPECT_EQ (track->color (), QColor ("#00FF00"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test command text in undo stack for setColor
TEST_F (TrackOperatorTest, CommandTextInUndoStackSetColor)
{
  track_operator->setColor (QColor ("#123456"));

  EXPECT_EQ (undo_stack->text (0), QString ("Change Track Color"));
}

// Test mixed operations (rename and setColor)
TEST_F (TrackOperatorTest, MixedOperations)
{
  track_operator->rename ("New Track Name");
  track_operator->setColor (QColor ("#00FF00"));

  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (track->color (), QColor ("#00FF00"));
  EXPECT_EQ (undo_stack->count (), 2);

  // Undo color change
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (track->color (), QColor ("red"));

  // Undo rename
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
  EXPECT_EQ (track->color (), QColor ("red"));
}

// Test basic setComment operation with undo/redo
TEST_F (TrackOperatorTest, SetCommentWithUndoRedo)
{
  track->setComment ("Initial comment");
  track_operator->setComment ("New comment");

  EXPECT_EQ (track->comment (), QString ("New comment"));
  EXPECT_EQ (undo_stack->text (0), QString ("Change Track Comment"));

  undo_stack->undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));

  undo_stack->redo ();
  EXPECT_EQ (track->comment (), QString ("New comment"));
}

// ============================================================================
// Track Routing Tests (using actual track types via TrackFactory)
// ============================================================================

// Test basic setOutputTrack operation with actual tracks
TEST_F (TrackOperatorRoutingTest, BasicSetOutputTrack)
{
  // Create source track (Audio) using TrackFactory
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  // Create destination track (AudioGroup) using TrackFactory
  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  // Initially no output track
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_TRUE (output.isNull ());

  // Set output track
  track_operator_->setOutputTrack (dest);

  // Verify output track is set
  output = track_routing_->getOutputTrack (source);
  EXPECT_FALSE (output.isNull ());

  // Verify undo stack has command
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canUndo ());
}

// Test undo after setOutputTrack
TEST_F (TrackOperatorRoutingTest, UndoAfterSetOutputTrack)
{
  // Create source and destination tracks
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  track_operator_->setOutputTrack (dest);
  EXPECT_FALSE (track_routing_->getOutputTrack (source).isNull ());

  undo_stack_->undo ();

  // After undo, should have no output track
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_TRUE (output.isNull ());

  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canRedo ());
}

// Test redo after undo for setOutputTrack
TEST_F (TrackOperatorRoutingTest, RedoAfterUndoSetOutputTrack)
{
  // Create source and destination tracks
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  track_operator_->setOutputTrack (dest);
  undo_stack_->undo ();
  undo_stack_->redo ();

  // After redo, should have output track again
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_FALSE (output.isNull ());

  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canUndo ());
  EXPECT_FALSE (undo_stack_->canRedo ());
}

// Test setOutputTrack to null (clear routing)
TEST_F (TrackOperatorRoutingTest, SetOutputTrackToNull)
{
  // Create source and destination tracks
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  // First set an output track
  track_operator_->setOutputTrack (dest);
  EXPECT_FALSE (track_routing_->getOutputTrack (source).isNull ());

  // Now clear it by setting to null
  track_operator_->setOutputTrack (nullptr);
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_TRUE (output.isNull ());

  // Undo should restore the previous routing
  undo_stack_->undo ();
  EXPECT_FALSE (track_routing_->getOutputTrack (source).isNull ());
}

// Test invalid routing is rejected (Audio to MIDI)
TEST_F (TrackOperatorRoutingTest, InvalidRoutingRejected)
{
  // Create Audio source track
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  // Create MIDI group track
  auto midi_ref =
    track_factory_->create_empty_track<structure::tracks::MidiGroupTrack> ();
  auto * midi = midi_ref.get_object_as<structure::tracks::MidiGroupTrack> ();

  // Audio track cannot route to MIDI group (incompatible signal types)
  track_operator_->setOutputTrack (midi);

  // Should not have been routed
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_TRUE (output.isNull ());

  // No command should have been pushed
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test routing to self is rejected
TEST_F (TrackOperatorRoutingTest, RoutingToSelfRejected)
{
  // Create source track
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  // Track cannot route to itself
  track_operator_->setOutputTrack (source);

  // Should not have been routed
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_TRUE (output.isNull ());

  // No command should have been pushed
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test multiple routing changes
TEST_F (TrackOperatorRoutingTest, MultipleRoutingChanges)
{
  // Create source track
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  // Create first destination
  auto dest1_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest1 = dest1_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  // Create second destination
  auto dest2_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest2 = dest2_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  // Route to first destination
  track_operator_->setOutputTrack (dest1);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Route to second destination
  track_operator_->setOutputTrack (dest2);
  EXPECT_EQ (undo_stack_->count (), 2);

  // Undo second routing
  undo_stack_->undo ();
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_FALSE (output.isNull ());

  // Undo first routing
  undo_stack_->undo ();
  output = track_routing_->getOutputTrack (source);
  EXPECT_TRUE (output.isNull ());
}

// Test routing to non-group target is rejected (Audio to Audio)
TEST_F (TrackOperatorRoutingTest, RoutingToNonGroupTargetRejected)
{
  // Create source track
  auto source_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * source = source_ref.get_object_as<structure::tracks::AudioTrack> ();
  track_operator_->setTrack (source);

  // Create another Audio track (not a group target)
  auto other_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * other = other_ref.get_object_as<structure::tracks::AudioTrack> ();

  // Audio track cannot route to another Audio track (not a group target)
  track_operator_->setOutputTrack (other);

  // Should not have been routed
  auto output = track_routing_->getOutputTrack (source);
  EXPECT_TRUE (output.isNull ());

  // No command should have been pushed
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test circular route detection
TEST_F (TrackOperatorRoutingTest, CircularRouteRejected)
{
  // Create chain: Audio -> AudioGroup1 -> AudioGroup2
  auto audio_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * audio = audio_ref.get_object_as<structure::tracks::AudioTrack> ();

  auto group1_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * group1 =
    group1_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  auto group2_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * group2 =
    group2_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  // Set up routing: Audio -> Group1 -> Group2
  track_operator_->setTrack (audio);
  track_operator_->setOutputTrack (group1);
  EXPECT_EQ (undo_stack_->count (), 1);

  track_operator_->setTrack (group1);
  track_operator_->setOutputTrack (group2);
  EXPECT_EQ (undo_stack_->count (), 2);

  // Now try to route Group2 to Audio - should be rejected (would create cycle)
  track_operator_->setTrack (group2);
  track_operator_->setOutputTrack (audio);

  // Should not have been routed
  auto output = track_routing_->getOutputTrack (group2);
  EXPECT_TRUE (output.isNull ());

  // No additional command should have been pushed
  EXPECT_EQ (undo_stack_->count (), 2);

  // Also try Group2 to Group1 - should be rejected
  track_operator_->setOutputTrack (group1);
  output = track_routing_->getOutputTrack (group2);
  EXPECT_TRUE (output.isNull ());
  EXPECT_EQ (undo_stack_->count (), 2);
}

} // namespace zrythm::actions
