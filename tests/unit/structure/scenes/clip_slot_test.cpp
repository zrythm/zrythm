// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>
#include <chrono>
#include <thread>

#include "structure/arrangement/midi_clip.h"
#include "structure/scenes/clip_slot.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/track_collection.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>
#include <QTest>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::scenes
{

class ClipSlotTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    track_collection_ = std::make_unique<tracks::TrackCollection> (*registry_);
    clip_slot_ = std::make_unique<ClipSlot> (*registry_);
  }

  void TearDown () override
  {
    clip_slot_.reset ();
    clip_refs_.clear ();
    track_collection_.reset ();
    tempo_map_wrapper_.reset ();
    tempo_map_.reset ();
    registry_.reset ();
  }

  arrangement::MidiClip * create_midi_clip ()
  {
    auto clip_ref = utils::create_object<arrangement::MidiClip> (
      *registry_, *tempo_map_wrapper_, *registry_);
    auto clip = clip_ref.get_object_as<arrangement::MidiClip> ();

    // Set basic properties
    clip->position ()->setTicks (0.0);
    clip->length ()->setTicks (100.0);

    // Keep a reference to the clip
    clip_refs_.push_back (std::move (clip_ref));

    return clip;
  }

  std::unique_ptr<ClipSlot>                             clip_slot_;
  std::unique_ptr<tracks::TrackCollection>              track_collection_;
  std::unique_ptr<utils::ObjectRegistry>                registry_;
  std::unique_ptr<dsp::TempoMap>                        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper_;
  std::vector<arrangement::ArrangerObjectUuidReference> clip_refs_;
};

TEST_F (ClipSlotTest, InitialState)
{
  // Initial state should be Stopped
  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::Stopped);

  // Should have no clip initially
  EXPECT_EQ (clip_slot_->clip (), nullptr);
}

TEST_F (ClipSlotTest, StateChanges)
{
  // Test setting state to PlayQueued
  QSignalSpy stateSpy (clip_slot_.get (), &ClipSlot::stateChanged);
  clip_slot_->setState (ClipSlot::ClipState::PlayQueued);

  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::PlayQueued);
  EXPECT_EQ (stateSpy.count (), 1);
  EXPECT_EQ (
    stateSpy.at (0).at (0).value<ClipSlot::ClipState> (),
    ClipSlot::ClipState::PlayQueued);

  // Test setting state to Playing
  clip_slot_->setState (ClipSlot::ClipState::Playing);

  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::Playing);
  EXPECT_EQ (stateSpy.count (), 2);
  EXPECT_EQ (
    stateSpy.at (1).at (0).value<ClipSlot::ClipState> (),
    ClipSlot::ClipState::Playing);

  // Test setting state to StopQueued
  clip_slot_->setState (ClipSlot::ClipState::StopQueued);

  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::StopQueued);
  EXPECT_EQ (stateSpy.count (), 3);
  EXPECT_EQ (
    stateSpy.at (2).at (0).value<ClipSlot::ClipState> (),
    ClipSlot::ClipState::StopQueued);

  // Test setting state back to Stopped
  clip_slot_->setState (ClipSlot::ClipState::Stopped);

  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::Stopped);
  EXPECT_EQ (stateSpy.count (), 4);
  EXPECT_EQ (
    stateSpy.at (3).at (0).value<ClipSlot::ClipState> (),
    ClipSlot::ClipState::Stopped);
}

TEST_F (ClipSlotTest, StateChangeToSameState)
{
  // Setting to the same state should not emit signal
  QSignalSpy stateSpy (clip_slot_.get (), &ClipSlot::stateChanged);
  clip_slot_->setState (ClipSlot::ClipState::Stopped);

  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::Stopped);
  EXPECT_EQ (stateSpy.count (), 0);
}

TEST_F (ClipSlotTest, ClipManagement)
{
  // Test setting a clip
  auto       clip = create_midi_clip ();
  QSignalSpy clipSpy (clip_slot_.get (), &ClipSlot::clipObjectChanged);

  clip_slot_->setClip (clip);

  EXPECT_EQ (clip_slot_->clip (), clip);
  EXPECT_EQ (clipSpy.count (), 1);
  EXPECT_EQ (
    clipSpy.at (0).at (0).value<arrangement::ArrangerObject *> (), clip);
}

TEST_F (ClipSlotTest, ClipChangeToSameClip)
{
  // Setting the same clip should not emit signal
  auto clip = create_midi_clip ();
  clip_slot_->setClip (clip);

  QSignalSpy clipSpy (clip_slot_.get (), &ClipSlot::clipObjectChanged);
  clip_slot_->setClip (clip);

  EXPECT_EQ (clip_slot_->clip (), clip);
  EXPECT_EQ (clipSpy.count (), 0);
}

TEST_F (ClipSlotTest, ClearClip)
{
  // Set a clip first
  auto clip = create_midi_clip ();
  clip_slot_->setClip (clip);
  EXPECT_EQ (clip_slot_->clip (), clip);

  // Clear the clip
  QSignalSpy clipSpy (clip_slot_.get (), &ClipSlot::clipObjectChanged);
  clip_slot_->clearClip ();

  EXPECT_EQ (clip_slot_->clip (), nullptr);
  EXPECT_EQ (clipSpy.count (), 1);
  EXPECT_EQ (
    clipSpy.at (0).at (0).value<arrangement::ArrangerObject *> (), nullptr);
}

TEST_F (ClipSlotTest, ClearClipWhenEmpty)
{
  // Clearing when already empty should not emit signal
  QSignalSpy clipSpy (clip_slot_.get (), &ClipSlot::clipObjectChanged);
  clip_slot_->clearClip ();

  EXPECT_EQ (clip_slot_->clip (), nullptr);
  EXPECT_EQ (clipSpy.count (), 0);
}

// Regression: replacing a clip must detach the previous clip's timebase
// provider from the slot (clearClip detaches, setClip used not to).
TEST_F (ClipSlotTest, SetClipDetachesPreviousClipTimebase)
{
  dsp::TimebaseProvider slot_provider;
  slot_provider.setOverride (dsp::Timebase::Absolute);
  clip_slot_->setTimebaseProvider (&slot_provider);

  auto * clip_a = create_midi_clip ();
  auto * clip_b = create_midi_clip ();

  clip_slot_->setClip (clip_a);
  ASSERT_EQ (
    clip_a->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);

  clip_slot_->setClip (clip_b);

  // clip_a must no longer follow the slot's Absolute override.
  EXPECT_NE (
    clip_a->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
}

TEST_F (ClipSlotTest, AtomicStateAccess)
{
  // Test that state is atomic by accessing from multiple threads
  std::atomic<bool> thread1_done{ false };

  // Thread 1: Set state to PlayQueued, then Playing
  {
    std::jthread thread1 ([&] (std::stop_token) {
      clip_slot_->setState (ClipSlot::ClipState::PlayQueued);
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
      clip_slot_->setState (ClipSlot::ClipState::Playing);
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
      thread1_done = true;
    });

    // Thread 2: Check state changes
    std::jthread thread2 ([&] (std::stop_token) {
      bool found_play_queued = false;
      bool found_playing = false;

      while (!thread1_done)
        {
          auto state = clip_slot_->state ();
          if (state == ClipSlot::ClipState::PlayQueued)
            found_play_queued = true;
          else if (state == ClipSlot::ClipState::Playing)
            found_playing = true;

          std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }

      EXPECT_TRUE (found_play_queued);
      EXPECT_TRUE (found_playing);
    });
  }
  // jthreads automatically join when they go out of scope

  // Final state should be Playing
  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::Playing);
}

class ClipSlotListTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    track_collection_ = std::make_unique<tracks::TrackCollection> (*registry_);

    create_test_tracks ();

    clip_slot_list_ =
      std::make_unique<ClipSlotList> (*registry_, *track_collection_, nullptr);
  }

  void create_test_tracks ()
  {
    for (int i = 0; i < 3; ++i)
      {
        tracks::FinalTrackDependencies deps{
          *tempo_map_wrapper_, *registry_, [] { return false; }, {}
        };

        auto track_ref = utils::create_object<tracks::FolderTrack> (
          *registry_, std::move (deps));
        track_collection_->add_track (track_ref);
      }
  }

  void TearDown () override
  {
    clip_slot_list_.reset ();
    track_collection_.reset ();
    tempo_map_.reset ();
    registry_.reset ();
  }

  std::unique_ptr<ClipSlotList>                   clip_slot_list_;
  std::unique_ptr<tracks::TrackCollection>        track_collection_;
  std::unique_ptr<utils::ObjectRegistry>          registry_;
  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>           tempo_map_wrapper_;
  std::unique_ptr<dsp::graph_test::MockTransport> transport_;
};

TEST_F (ClipSlotListTest, InitialState)
{
  // Should have clip slots matching the track count (initially 3)
  EXPECT_EQ (clip_slot_list_->rowCount (), 3);
}

TEST_F (ClipSlotListTest, RoleNames)
{
  auto roles = clip_slot_list_->roleNames ();

  EXPECT_TRUE (roles.contains (ClipSlotList::ClipSlotPtrRole));
  EXPECT_EQ (roles[ClipSlotList::ClipSlotPtrRole], "clipSlot");
}

TEST_F (ClipSlotListTest, DataAccess)
{
  // Test accessing data at valid indices
  for (int i = 0; i < 3; ++i)
    {
      auto index = clip_slot_list_->index (i, 0);
      ASSERT_TRUE (index.isValid ());

      auto clipSlot =
        clip_slot_list_->data (index, ClipSlotList::ClipSlotPtrRole)
          .value<ClipSlot *> ();
      EXPECT_NE (clipSlot, nullptr);
      EXPECT_EQ (clipSlot->state (), ClipSlot::ClipState::Stopped);
      EXPECT_EQ (clipSlot->clip (), nullptr);
    }

  // Test accessing data at invalid indices
  auto invalidIndex = clip_slot_list_->index (10, 0);
  EXPECT_FALSE (invalidIndex.isValid ());

  auto invalidData =
    clip_slot_list_->data (invalidIndex, ClipSlotList::ClipSlotPtrRole);
  EXPECT_FALSE (invalidData.isValid ());
}

TEST_F (ClipSlotListTest, OutOfBoundsAccess)
{
  // Test accessing data beyond the row count
  auto outOfBoundsIndex = clip_slot_list_->index (100, 0);
  EXPECT_FALSE (outOfBoundsIndex.isValid ());

  auto outOfBoundsData =
    clip_slot_list_->data (outOfBoundsIndex, ClipSlotList::ClipSlotPtrRole);
  EXPECT_FALSE (outOfBoundsData.isValid ());
}

}
