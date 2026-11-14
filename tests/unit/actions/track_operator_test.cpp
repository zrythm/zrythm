// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_operator.h"
#include "structure/tracks/singleton_tracks.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"
#include "utils/utf8_string.h"

#include "operations/builtin_operations.h"
#include "operations/operation_dispatcher.h"
#include "operations/operation_environment.h"
#include "operations/operation_registry.h"
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
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (registry_);

    // Create track factory with dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      registry_,
      soloed_tracks_exist_getter_,
      {},
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] () -> structure::tracks::FinalTrackDependencies {
        return factory_deps;
      });

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create track operator
    track_operator_ = std::make_unique<TrackOperator> ();
    track_operator_->setTrackRouting (track_routing_.get ());
    track_operator_->setUndoStack (undo_stack_.get ());
  }

  dsp::TempoMap                  tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper           tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry          registry_;
  dsp::graph_test::MockTransport transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::TrackRouting> track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory> track_factory_;
  std::unique_ptr<undo::UndoStack>                 undo_stack_;
  std::unique_ptr<TrackOperator>                   track_operator_;
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

// Test setTimebase with undo/redo
TEST_F (TrackOperatorTest, SetTimebaseWithUndoRedo)
{
  // MockTrack defaults to Musical (base Track default)
  ASSERT_EQ (
    track->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);

  track_operator->setTimebase (dsp::Timebase::Absolute);
  EXPECT_EQ (
    track->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
  EXPECT_EQ (undo_stack->count (), 1);

  undo_stack->undo ();
  EXPECT_EQ (
    track->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);

  undo_stack->redo ();
  EXPECT_EQ (
    track->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
}

// ============================================================================
// Methods dispatching through the operations registry (real track services)
// ============================================================================

class TrackOperatorDispatchTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_, object_registry_, soloed_tracks_exist_getter_, {}
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] { return factory_deps; });
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (object_registry_);
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (object_registry_);

    undo_stack_ = std::make_unique<undo::UndoStack> ([] (auto callback, bool) {
      callback ();
    });

    auto track_ref =
      track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
    track_ = track_ref.get ();
    track_->setName ("Original Track Name");
    track_collection_->add_track (track_ref);

    operations::register_builtin_operations (registry_);
    environment_ = std::make_unique<operations::OperationEnvironment> (
      object_registry_, *undo_stack_, *track_collection_, *track_factory_,
      *track_routing_, singleton_tracks_);
    dispatcher_ = std::make_unique<operations::OperationDispatcher> (
      registry_, *environment_);

    track_operator_ = std::make_unique<TrackOperator> ();
    track_operator_->setTrack (track_);
    track_operator_->setUndoStack (undo_stack_.get ());
    track_operator_->setOperationDispatcher (dispatcher_.get ());
  }

  dsp::TempoMap         tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper  tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry object_registry_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };
  structure::tracks::SingletonTracks singleton_tracks_;

  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>    track_routing_;
  std::unique_ptr<undo::UndoStack>                    undo_stack_;
  operations::OperationRegistry                       registry_;
  std::unique_ptr<operations::OperationEnvironment>   environment_;
  std::unique_ptr<operations::OperationDispatcher>    dispatcher_;
  std::unique_ptr<TrackOperator>                      track_operator_;
  structure::tracks::Track *                          track_{};
};

// Test rename through the registry, with undo and redo
TEST_F (TrackOperatorDispatchTest, RenameAppliesUndoesAndRedoes)
{
  track_operator_->rename ("New Track Name");

  EXPECT_EQ (track_->name (), QString ("New Track Name"));
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canUndo ());

  undo_stack_->undo ();
  EXPECT_EQ (track_->name (), QString ("Original Track Name"));

  undo_stack_->redo ();
  EXPECT_EQ (track_->name (), QString ("New Track Name"));
}

// Test the command text the rename leaves in the history
TEST_F (TrackOperatorDispatchTest, RenameCommandText)
{
  track_operator_->rename ("Test Name");

  EXPECT_EQ (undo_stack_->text (0), QString ("Rename Track"));
}

// An empty name is refused: the name stays unchanged and the history
// stays empty
TEST_F (TrackOperatorDispatchTest, RenameRefusesEmptyName)
{
  track_operator_->rename ("");

  EXPECT_EQ (track_->name (), QString ("Original Track Name"));
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Rename without a dispatcher wired does nothing
TEST_F (TrackOperatorDispatchTest, RenameWithoutDispatcherIsIgnored)
{
  track_operator_->setOperationDispatcher (nullptr);

  track_operator_->rename ("New Track Name");

  EXPECT_EQ (track_->name (), QString ("Original Track Name"));
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Set color through the registry, with undo and redo
TEST_F (TrackOperatorDispatchTest, SetColorAppliesUndoesAndRedoes)
{
  track_->setColor (QColor ("#FF0000"));
  track_operator_->setColor (QColor ("#00FF00"));

  EXPECT_EQ (track_->color (), QColor ("#00FF00"));
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_EQ (track_->color (), QColor ("#FF0000"));

  undo_stack_->redo ();
  EXPECT_EQ (track_->color (), QColor ("#00FF00"));
}

// Test the command text a color change leaves in the history
TEST_F (TrackOperatorDispatchTest, SetColorCommandText)
{
  track_operator_->setColor (QColor ("#123456"));

  EXPECT_EQ (undo_stack_->text (0), QString ("Change Track Color"));
}

// Resetting the color clears the stored color and undo restores it
TEST_F (TrackOperatorDispatchTest, UnsetColorClearsStoredColorAndUndoes)
{
  track_->setColor (QColor ("#FF0000"));
  track_operator_->unsetColor ();

  EXPECT_FALSE (track_->useColor ());
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QString ("Reset Track Color"));

  undo_stack_->undo ();
  EXPECT_TRUE (track_->useColor ());
  EXPECT_EQ (track_->color (), QColor ("#FF0000"));
}

// A track without a stored color has nothing to reset
TEST_F (TrackOperatorDispatchTest, UnsetColorWithoutStoredColorPushesNothing)
{
  ASSERT_FALSE (track_->useColor ());

  track_operator_->unsetColor ();

  EXPECT_EQ (undo_stack_->count (), 0);
}

// Setting the color a track already stores is not a change
TEST_F (TrackOperatorDispatchTest, SetColorToStoredColorPushesNothing)
{
  track_->setColor (QColor ("#00FF00"));
  track_operator_->setColor (QColor ("#00FF00"));

  EXPECT_EQ (undo_stack_->count (), 0);
}

// Color changes without a dispatcher wired do nothing
TEST_F (TrackOperatorDispatchTest, ColorChangesWithoutDispatcherAreIgnored)
{
  track_operator_->setOperationDispatcher (nullptr);

  track_operator_->setColor (QColor ("#00FF00"));
  track_operator_->unsetColor ();

  EXPECT_FALSE (track_->useColor ());
  EXPECT_EQ (undo_stack_->count (), 0);
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
