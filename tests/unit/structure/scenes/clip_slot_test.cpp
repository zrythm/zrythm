// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>
#include <chrono>
#include <thread>

#include "structure/arrangement/midi_region.h"
#include "structure/scenes/clip_slot.h"
#include "structure/tracks/track_collection.h"

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
    // Create track registry
    track_registry_ = std::make_unique<tracks::TrackRegistry> ();

    // Create test dependencies
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Create object registries
    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();

    // Create track collection
    track_collection_ =
      std::make_unique<tracks::TrackCollection> (*track_registry_);

    // Create a clip slot
    clip_slot_ = std::make_unique<ClipSlot> (*obj_registry_);
  }

  void TearDown () override
  {
    clip_slot_.reset ();
    region_refs_.clear ();
    track_collection_.reset ();
    obj_registry_.reset ();
    tempo_map_.reset ();
    track_registry_.reset ();
  }

  // Helper function to create a MIDI region
  arrangement::MidiRegion * create_midi_region ()
  {
    auto region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
      *tempo_map_, *obj_registry_, file_audio_source_registry_);
    auto region = region_ref.get_object_as<arrangement::MidiRegion> ();

    // Set basic properties
    region->position ()->setTicks (0.0);
    region->bounds ()->length ()->setTicks (100.0);

    // Keep a reference to the region
    region_refs_.push_back (std::move (region_ref));

    return region;
  }

  std::unique_ptr<ClipSlot>                            clip_slot_;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<tracks::TrackCollection>             track_collection_;
  std::unique_ptr<tracks::TrackRegistry>               track_registry_;
  std::unique_ptr<dsp::TempoMap>                       tempo_map_;
  dsp::FileAudioSourceRegistry file_audio_source_registry_;
  std::vector<arrangement::ArrangerObjectUuidReference> region_refs_;
};

TEST_F (ClipSlotTest, InitialState)
{
  // Initial state should be Stopped
  EXPECT_EQ (clip_slot_->state (), ClipSlot::ClipState::Stopped);

  // Should have no region initially
  EXPECT_EQ (clip_slot_->region (), nullptr);
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

TEST_F (ClipSlotTest, RegionManagement)
{
  // Test setting a region
  auto       region = create_midi_region ();
  QSignalSpy regionSpy (clip_slot_.get (), &ClipSlot::regionChanged);

  clip_slot_->setRegion (region);

  EXPECT_EQ (clip_slot_->region (), region);
  EXPECT_EQ (regionSpy.count (), 1);
  EXPECT_EQ (
    regionSpy.at (0).at (0).value<arrangement::ArrangerObject *> (), region);
}

TEST_F (ClipSlotTest, RegionChangeToSameRegion)
{
  // Setting the same region should not emit signal
  auto region = create_midi_region ();
  clip_slot_->setRegion (region);

  QSignalSpy regionSpy (clip_slot_.get (), &ClipSlot::regionChanged);
  clip_slot_->setRegion (region);

  EXPECT_EQ (clip_slot_->region (), region);
  EXPECT_EQ (regionSpy.count (), 0);
}

TEST_F (ClipSlotTest, ClearRegion)
{
  // Set a region first
  auto region = create_midi_region ();
  clip_slot_->setRegion (region);
  EXPECT_EQ (clip_slot_->region (), region);

  // Clear the region
  QSignalSpy regionSpy (clip_slot_.get (), &ClipSlot::regionChanged);
  clip_slot_->clearRegion ();

  EXPECT_EQ (clip_slot_->region (), nullptr);
  EXPECT_EQ (regionSpy.count (), 1);
  EXPECT_EQ (
    regionSpy.at (0).at (0).value<arrangement::ArrangerObject *> (), nullptr);
}

TEST_F (ClipSlotTest, ClearRegionWhenEmpty)
{
  // Clearing when already empty should not emit signal
  QSignalSpy regionSpy (clip_slot_.get (), &ClipSlot::regionChanged);
  clip_slot_->clearRegion ();

  EXPECT_EQ (clip_slot_->region (), nullptr);
  EXPECT_EQ (regionSpy.count (), 0);
}

TEST_F (ClipSlotTest, AtomicStateAccess)
{
  // Test that state is atomic by accessing from multiple threads
  std::atomic<bool> thread1_done{ false };

  // Thread 1: Set state to PlayQueued, then Playing
  {
    std::jthread thread1 ([&] (std::stop_token) {
      clip_slot_->setState (ClipSlot::ClipState::PlayQueued);
      std::this_thread::sleep_for (std::chrono::milliseconds (10));
      clip_slot_->setState (ClipSlot::ClipState::Playing);
      std::this_thread::sleep_for (std::chrono::milliseconds (10));
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

          std::this_thread::sleep_for (std::chrono::milliseconds (1));
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
    // Create track registry
    track_registry_ = std::make_unique<tracks::TrackRegistry> ();

    // Create test dependencies
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    // Create object registries
    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();

    // Create track collection
    track_collection_ =
      std::make_unique<tracks::TrackCollection> (*track_registry_);

    // Create some test tracks
    create_test_tracks ();

    // Create a clip slot list
    clip_slot_list_ = std::make_unique<ClipSlotList> (
      *obj_registry_, *track_collection_, nullptr);
  }

  // Helper to create test tracks
  void create_test_tracks ()
  {
    // Create 3 test tracks
    for (int i = 0; i < 3; ++i)
      {
        tracks::FinalTrackDependencies deps{
          *tempo_map_wrapper_, file_audio_source_registry_,
          plugin_registry_,    port_registry_,
          param_registry_,     *obj_registry_,
          *track_registry_,    transport_,
          [] { return false; }
        };

        auto track_ref = track_registry_->create_object<tracks::FolderTrack> (
          std::move (deps));
        track_collection_->add_track (track_ref);
      }
  }

  void TearDown () override
  {
    clip_slot_list_.reset ();
    track_collection_.reset ();
    obj_registry_.reset ();
    tempo_map_.reset ();
    track_registry_.reset ();
  }

  std::unique_ptr<ClipSlotList>                        clip_slot_list_;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<tracks::TrackCollection>             track_collection_;
  std::unique_ptr<tracks::TrackRegistry>               track_registry_;
  std::unique_ptr<dsp::TempoMap>                       tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                tempo_map_wrapper_;
  dsp::PortRegistry                                    port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  dsp::FileAudioSourceRegistry    file_audio_source_registry_;
  plugins::PluginRegistry         plugin_registry_;
  dsp::graph_test::MockTransport  transport_;
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
      EXPECT_EQ (clipSlot->region (), nullptr);
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
