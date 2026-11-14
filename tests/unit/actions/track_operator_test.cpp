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
#include "operations/track_operations.h"
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

// Set comment through the registry, with undo and redo
TEST_F (TrackOperatorDispatchTest, SetCommentAppliesUndoesAndRedoes)
{
  track_->setComment ("Initial comment");
  track_operator_->setComment ("New comment");

  EXPECT_EQ (track_->comment (), QString ("New comment"));
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QString ("Change Track Comment"));

  undo_stack_->undo ();
  EXPECT_EQ (track_->comment (), QString ("Initial comment"));

  undo_stack_->redo ();
  EXPECT_EQ (track_->comment (), QString ("New comment"));
}

// Setting the comment a track already stores is not a change
TEST_F (TrackOperatorDispatchTest, SetCommentToStoredCommentPushesNothing)
{
  track_->setComment ("Same comment");
  track_operator_->setComment ("Same comment");

  EXPECT_EQ (undo_stack_->count (), 0);
}

// An empty comment clears the comment and undo restores it
TEST_F (TrackOperatorDispatchTest, EmptyCommentClearsAndUndoes)
{
  track_->setComment ("Initial comment");
  track_operator_->setComment ("");
  EXPECT_TRUE (track_->comment ().isEmpty ());
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_EQ (track_->comment (), QString ("Initial comment"));
}

// A comment over the size limit is refused
TEST_F (TrackOperatorDispatchTest, OverlongCommentIsRefused)
{
  track_operator_->setComment (
    QString (operations::kMaxTrackCommentBytes + 1, 'a'));

  EXPECT_TRUE (track_->comment ().isEmpty ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

// The comment limit counts UTF-8 bytes, not characters: 2048 two-byte
// characters fit, 2049 do not
TEST_F (TrackOperatorDispatchTest, CommentLimitCountsBytesNotCharacters)
{
  const QString at_limit (2048, QChar (0x00E9));
  track_operator_->setComment (at_limit);
  EXPECT_EQ (track_->comment (), at_limit);
  EXPECT_EQ (undo_stack_->count (), 1);

  const QString over_limit (2049, QChar (0x00E9));
  track_operator_->setComment (over_limit);
  EXPECT_EQ (track_->comment (), at_limit);
  EXPECT_EQ (undo_stack_->count (), 1);
}

// Set timebase through the registry, with undo and redo. An audio
// track stores an Absolute override from construction.
TEST_F (TrackOperatorDispatchTest, SetTimebaseAppliesUndoesAndRedoes)
{
  ASSERT_EQ (
    track_->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
  ASSERT_EQ (
    track_->timebaseProvider ()->overrideValue (),
    std::optional{ dsp::Timebase::Absolute });

  track_operator_->setTimebase (dsp::Timebase::Musical);
  EXPECT_EQ (
    track_->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);
  EXPECT_EQ (
    track_->timebaseProvider ()->overrideValue (),
    std::optional{ dsp::Timebase::Musical });
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QString ("Change Timebase"));

  undo_stack_->undo ();
  EXPECT_EQ (
    track_->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
  EXPECT_EQ (
    track_->timebaseProvider ()->overrideValue (),
    std::optional{ dsp::Timebase::Absolute });

  undo_stack_->redo ();
  EXPECT_EQ (
    track_->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);
}

// Setting the override a track already stores is not a change
TEST_F (TrackOperatorDispatchTest, SetTimebaseToStoredOverridePushesNothing)
{
  track_operator_->setTimebase (dsp::Timebase::Musical);
  ASSERT_EQ (undo_stack_->count (), 1);

  track_operator_->setTimebase (dsp::Timebase::Musical);

  EXPECT_EQ (undo_stack_->count (), 1);
}

// Comment and timebase changes without a dispatcher wired do nothing
TEST_F (TrackOperatorDispatchTest, CommentAndTimebaseWithoutDispatcherAreIgnored)
{
  track_operator_->setOperationDispatcher (nullptr);

  track_operator_->setComment ("New comment");
  track_operator_->setTimebase (dsp::Timebase::Musical);

  EXPECT_TRUE (track_->comment ().isEmpty ());
  EXPECT_EQ (
    track_->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Route the track through the registry, with undo and redo
TEST_F (TrackOperatorDispatchTest, SetOutputTrackAppliesUndoesAndRedoes)
{
  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  track_operator_->setOutputTrack (dest);
  EXPECT_FALSE (track_routing_->getOutputTrack (track_).isNull ());
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_TRUE (track_routing_->getOutputTrack (track_).isNull ());

  undo_stack_->redo ();
  EXPECT_FALSE (track_routing_->getOutputTrack (track_).isNull ());
}

// Unrouting clears the route and undo restores it
TEST_F (TrackOperatorDispatchTest, SetOutputTrackToNullClearsAndUndoes)
{
  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();
  track_operator_->setOutputTrack (dest);
  ASSERT_FALSE (track_routing_->getOutputTrack (track_).isNull ());

  track_operator_->setOutputTrack (nullptr);
  EXPECT_TRUE (track_routing_->getOutputTrack (track_).isNull ());
  EXPECT_EQ (undo_stack_->count (), 2);

  undo_stack_->undo ();
  EXPECT_FALSE (track_routing_->getOutputTrack (track_).isNull ());
}

// Unrouting a track without a route is not a change
TEST_F (TrackOperatorDispatchTest, UnrouteWithoutRoutePushesNothing)
{
  track_operator_->setOutputTrack (nullptr);

  EXPECT_EQ (undo_stack_->count (), 0);
}

// Routing to the track's current target is not a change
TEST_F (TrackOperatorDispatchTest, RouteToCurrentTargetPushesNothing)
{
  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();
  track_operator_->setOutputTrack (dest);
  ASSERT_EQ (undo_stack_->count (), 1);

  track_operator_->setOutputTrack (dest);

  EXPECT_EQ (undo_stack_->count (), 1);
}

// A routing that would close a cycle is refused without a command
TEST_F (TrackOperatorDispatchTest, SetOutputTrackRefusesCycle)
{
  auto group1_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * group1 =
    group1_ref.get_object_as<structure::tracks::AudioGroupTrack> ();
  auto group2_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * group2 =
    group2_ref.get_object_as<structure::tracks::AudioGroupTrack> ();

  // Chain: track_ -> group1 -> group2
  track_operator_->setOutputTrack (group1);
  ASSERT_EQ (undo_stack_->count (), 1);

  track_operator_->setTrack (group1);
  track_operator_->setOutputTrack (group2);
  ASSERT_EQ (undo_stack_->count (), 2);

  // Routing group2 back to group1 would close the cycle
  track_operator_->setTrack (group2);
  track_operator_->setOutputTrack (group1);

  EXPECT_TRUE (track_routing_->getOutputTrack (group2).isNull ());
  EXPECT_EQ (undo_stack_->count (), 2);
}

// A routing between incompatible signal types is refused without a command
TEST_F (TrackOperatorDispatchTest, SetOutputTrackRefusesSignalTypeMismatch)
{
  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::MidiGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::MidiGroupTrack> ();

  track_operator_->setOutputTrack (dest);

  EXPECT_TRUE (track_routing_->getOutputTrack (track_).isNull ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Routing to a track that is not a routing target is refused
TEST_F (TrackOperatorDispatchTest, SetOutputTrackRefusesNonTargetTrack)
{
  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioTrack> ();

  track_operator_->setOutputTrack (dest);

  EXPECT_TRUE (track_routing_->getOutputTrack (track_).isNull ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Routing to the same track is refused
TEST_F (TrackOperatorDispatchTest, SetOutputTrackRefusesSelf)
{
  track_operator_->setOutputTrack (track_);

  EXPECT_TRUE (track_routing_->getOutputTrack (track_).isNull ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Routing changes without a dispatcher wired do nothing
TEST_F (TrackOperatorDispatchTest, RoutingWithoutDispatcherIsIgnored)
{
  auto dest_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * dest = dest_ref.get_object_as<structure::tracks::AudioGroupTrack> ();
  track_operator_->setOperationDispatcher (nullptr);

  track_operator_->setOutputTrack (dest);

  EXPECT_TRUE (track_routing_->getOutputTrack (track_).isNull ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

} // namespace zrythm::actions
