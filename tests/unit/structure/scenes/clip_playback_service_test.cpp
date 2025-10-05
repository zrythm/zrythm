// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/clip_playback_service.h"
#include "structure/scenes/scene.h"
#include "structure/tracks/track_collection.h"

#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#include "helpers/scoped_qcoreapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::scenes
{

class ClipPlaybackServiceTest : public ::testing::Test
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

    // Create test tracks
    create_test_tracks ();

    // Create a clip playback service
    playback_service_ = std::make_unique<ClipPlaybackService> (
      *obj_registry_, *track_collection_);
  }

  void TearDown () override
  {
    playback_service_.reset ();
    track_collection_.reset ();
    region_refs_.clear ();
    scenes_.clear ();
    obj_registry_.reset ();
    tempo_map_wrapper_.reset ();
    tempo_map_.reset ();
    track_registry_.reset ();
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

        auto track_ref =
          track_registry_->create_object<tracks::MidiTrack> (std::move (deps));
        track_collection_->add_track (track_ref);
      }
  }

  // Helper to create a MIDI region
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

  // Helper to create a scene with clips
  Scene * create_scene_with_clips ()
  {
    auto scene =
      utils::make_qobject_unique<Scene> (*obj_registry_, *track_collection_);

    // Add regions to clip slots
    auto * clipSlotList = scene->clipSlots ();
    for (int i = 0; i < 3; ++i)
      {
        auto * track =
          track_collection_->get_track_ref_at_index (i).get_object_base ();
        auto * clipSlot = clipSlotList->clipSlotForTrack (track);
        if (clipSlot != nullptr)
          {
            auto * region = create_midi_region ();
            clipSlot->setRegion (region);
          }
      }

    scenes_.push_back (std::move (scene));
    return scenes_.back ().get ();
  }

  test_helpers::ScopedQCoreApplication                 qapp_;
  std::unique_ptr<ClipPlaybackService>                 playback_service_;
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
  std::vector<arrangement::ArrangerObjectUuidReference> region_refs_;
  std::vector<utils::QObjectUniquePtr<Scene>>           scenes_;
};

TEST_F (ClipPlaybackServiceTest, InitialState)
{
  // Should start with no playing clips
  EXPECT_EQ (playback_service_->getPlayingClips ().size (), 0);
}

TEST_F (ClipPlaybackServiceTest, LaunchClip)
{
  // Get a track and create a clip slot with a region
  auto * track =
    track_collection_->get_track_ref_at_index (0).get_object_base ();
  auto   clipSlot = utils::make_qobject_unique<ClipSlot> (*obj_registry_);
  auto * region = create_midi_region ();
  clipSlot->setRegion (region);

  // Set up signal spy
  QSignalSpy launchedSpy (
    playback_service_.get (), &ClipPlaybackService::clipLaunched);

  // Launch the clip
  playback_service_->launchClip (
    track, clipSlot.get (), structure::tracks::ClipQuantizeOption::Immediate);

  // Should emit the clipLaunched signal
  EXPECT_EQ (launchedSpy.count (), 1);
  EXPECT_EQ (launchedSpy.at (0).at (0).value<tracks::Track *> (), track);
  EXPECT_EQ (launchedSpy.at (0).at (1).value<ClipSlot *> (), clipSlot.get ());

  // Clip slot should be in PlayQueued state initially
  EXPECT_EQ (clipSlot->state (), ClipSlot::ClipState::PlayQueued);
}

TEST_F (ClipPlaybackServiceTest, LaunchClipWithNullParameters)
{
  // Test with null track
  auto clipSlot = utils::make_qobject_unique<ClipSlot> (*obj_registry_);
  playback_service_->launchClip (
    nullptr, clipSlot.get (), structure::tracks::ClipQuantizeOption::Immediate);

  // Test with null clip slot
  auto * track =
    track_collection_->get_track_ref_at_index (0).get_object_base ();
  playback_service_->launchClip (
    track, nullptr, structure::tracks::ClipQuantizeOption::Immediate);

  // Should not crash
  SUCCEED ();
}

TEST_F (ClipPlaybackServiceTest, LaunchClipWithNoRegion)
{
  // Get a track and create a clip slot without a region
  auto * track =
    track_collection_->get_track_ref_at_index (0).get_object_base ();
  auto clipSlot = utils::make_qobject_unique<ClipSlot> (*obj_registry_);

  // Launch the clip
  playback_service_->launchClip (
    track, clipSlot.get (), structure::tracks::ClipQuantizeOption::Immediate);

  // Should not crash and clip should not be playing
  EXPECT_FALSE (playback_service_->isClipPlaying (clipSlot.get ()));
}

TEST_F (ClipPlaybackServiceTest, StopClip)
{
  // Get a track and create a clip slot with a region
  auto * track =
    track_collection_->get_track_ref_at_index (0).get_object_base ();
  auto   clipSlot = utils::make_qobject_unique<ClipSlot> (*obj_registry_);
  auto * region = create_midi_region ();
  clipSlot->setRegion (region);

  // Set up signal spies
  QSignalSpy launchedSpy (
    playback_service_.get (), &ClipPlaybackService::clipLaunched);
  QSignalSpy stoppedSpy (
    playback_service_.get (), &ClipPlaybackService::clipStopped);
  QSignalSpy stateSpy (clipSlot.get (), &ClipSlot::stateChanged);

  // Launch the clip first
  playback_service_->launchClip (
    track, clipSlot.get (), structure::tracks::ClipQuantizeOption::Immediate);
  EXPECT_EQ (launchedSpy.count (), 1);

  // Stop the clip
  playback_service_->stopClip (
    track, clipSlot.get (), structure::tracks::ClipQuantizeOption::Immediate);

  // Should emit the clipStopped signal immediately (when queued)
  EXPECT_EQ (stoppedSpy.count (), 1);
  EXPECT_EQ (stoppedSpy.at (0).at (0).value<tracks::Track *> (), track);
  EXPECT_EQ (stoppedSpy.at (0).at (1).value<ClipSlot *> (), clipSlot.get ());

  // Clip slot should be in StopQueued state initially
  EXPECT_EQ (clipSlot->state (), ClipSlot::ClipState::StopQueued);

  // In a real scenario, the timer would check the audio thread state and
  // eventually change the clip state to Stopped. Since we can't simulate the
  // audio thread in tests, we can't test the actual state change, but we can
  // verify the timer was started and the initial queue state is correct.
}

TEST_F (ClipPlaybackServiceTest, LaunchScene)
{
  // Create a scene with clips
  auto * scene = create_scene_with_clips ();

  // Set up signal spy
  QSignalSpy launchedSpy (
    playback_service_.get (), &ClipPlaybackService::sceneLaunched);

  // Launch the scene
  playback_service_->launchScene (
    scene, structure::tracks::ClipQuantizeOption::Immediate);

  // Should emit the sceneLaunched signal
  EXPECT_EQ (launchedSpy.count (), 1);
  EXPECT_EQ (launchedSpy.at (0).at (0).value<Scene *> (), scene);

  // All clips should be in PlayQueued state
  auto * clipSlotList = scene->clipSlots ();
  for (int i = 0; i < 3; ++i)
    {
      auto * track =
        track_collection_->get_track_ref_at_index (i).get_object_base ();
      auto * clipSlot = clipSlotList->clipSlotForTrack (track);
      if ((clipSlot != nullptr) && (clipSlot->region () != nullptr))
        {
          EXPECT_EQ (clipSlot->state (), ClipSlot::ClipState::PlayQueued);
        }
    }
}

TEST_F (ClipPlaybackServiceTest, StopScene)
{
  // Create a scene with clips
  auto * scene = create_scene_with_clips ();

  // Launch the scene first
  playback_service_->launchScene (
    scene, structure::tracks::ClipQuantizeOption::Immediate);

  // Set up signal spy
  QSignalSpy stoppedSpy (
    playback_service_.get (), &ClipPlaybackService::sceneStopped);

  // Stop the scene
  playback_service_->stopScene (
    scene, structure::tracks::ClipQuantizeOption::Immediate);

  // Should emit the sceneStopped signal
  EXPECT_EQ (stoppedSpy.count (), 1);
  EXPECT_EQ (stoppedSpy.at (0).at (0).value<Scene *> (), scene);
}

TEST_F (ClipPlaybackServiceTest, StopAllClips)
{
  // Create a scene with clips and launch it
  auto * scene = create_scene_with_clips ();
  playback_service_->launchScene (
    scene, structure::tracks::ClipQuantizeOption::Immediate);

  // Stop all clips
  playback_service_->stopAllClips (
    structure::tracks::ClipQuantizeOption::Immediate);

  // All clips should be in StopQueued state
  auto * clipSlotList = scene->clipSlots ();
  for (int i = 0; i < 3; ++i)
    {
      auto * track =
        track_collection_->get_track_ref_at_index (i).get_object_base ();
      auto * clipSlot = clipSlotList->clipSlotForTrack (track);
      if ((clipSlot != nullptr) && (clipSlot->region () != nullptr))
        {
          EXPECT_EQ (clipSlot->state (), ClipSlot::ClipState::StopQueued);
        }
    }
}

TEST_F (ClipPlaybackServiceTest, IsClipPlaying)
{
  // Create a clip slot
  auto clipSlot = utils::make_qobject_unique<ClipSlot> (*obj_registry_);

  // Initially should not be playing
  EXPECT_FALSE (playback_service_->isClipPlaying (clipSlot.get ()));

  // Set state to Playing
  clipSlot->setState (ClipSlot::ClipState::Playing);
  EXPECT_TRUE (playback_service_->isClipPlaying (clipSlot.get ()));

  // Set state to PlayQueued
  clipSlot->setState (ClipSlot::ClipState::PlayQueued);
  EXPECT_TRUE (playback_service_->isClipPlaying (clipSlot.get ()));

  // Set state to Stopped
  clipSlot->setState (ClipSlot::ClipState::Stopped);
  EXPECT_FALSE (playback_service_->isClipPlaying (clipSlot.get ()));

  // Test with null clip slot
  EXPECT_FALSE (playback_service_->isClipPlaying (nullptr));
}

TEST_F (ClipPlaybackServiceTest, GetClipPlaybackPosition)
{
  // Create a clip slot
  auto clipSlot = utils::make_qobject_unique<ClipSlot> (*obj_registry_);

  // Initially should return -1.0
  EXPECT_EQ (playback_service_->getClipPlaybackPosition (clipSlot.get ()), -1.0);

  // Test with null clip slot
  EXPECT_EQ (playback_service_->getClipPlaybackPosition (nullptr), -1.0);
}

TEST_F (ClipPlaybackServiceTest, GetPlayingClips)
{
  // Create a scene with clips
  auto * scene = create_scene_with_clips ();

  // Initially should have no playing clips
  EXPECT_EQ (playback_service_->getPlayingClips ().size (), 0);

  // Launch the scene
  playback_service_->launchScene (
    scene, structure::tracks::ClipQuantizeOption::Immediate);

  // Should have playing clips (in PlayQueued state)
  auto playingClips = playback_service_->getPlayingClips ();
  EXPECT_GT (playingClips.size (), 0);
}

TEST_F (ClipPlaybackServiceTest, SetTrackToClipLauncherMode)
{
  // Get a track
  auto * track =
    track_collection_->get_track_ref_at_index (0).get_object_base ();

  // Set track to clip launcher mode
  playback_service_->setTrackToClipLauncherMode (track);

  // Should not crash
  SUCCEED ();
}

TEST_F (ClipPlaybackServiceTest, SetTrackToTimelineMode)
{
  // Get a track
  auto * track =
    track_collection_->get_track_ref_at_index (0).get_object_base ();

  // Set track to timeline mode
  playback_service_->setTrackToTimelineMode (track);

  // Should not crash
  SUCCEED ();
}

TEST_F (ClipPlaybackServiceTest, CancelPendingClipOperations)
{
  // Get a track and create a clip slot with a region
  auto * track =
    track_collection_->get_track_ref_at_index (0).get_object_base ();
  auto   clipSlot = utils::make_qobject_unique<ClipSlot> (*obj_registry_);
  auto * region = create_midi_region ();
  clipSlot->setRegion (region);

  // Launch the clip
  playback_service_->launchClip (
    track, clipSlot.get (), structure::tracks::ClipQuantizeOption::Immediate);

  // Clip should be in PlayQueued state
  EXPECT_EQ (clipSlot->state (), ClipSlot::ClipState::PlayQueued);

  // Cancel pending operations
  playback_service_->cancelPendingClipOperations (track);

  // Clip should be back to Stopped state
  EXPECT_EQ (clipSlot->state (), ClipSlot::ClipState::Stopped);
}

} // namespace zrythm::structure::scenes
