// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/recording_materializer.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_fwd.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::controllers
{

using TrackUuid = structure::tracks::TrackUuid;

class RecordingMaterializerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<zrythm::test_helpers::ScopedQCoreApplication> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
    coordinator_ = std::make_unique<RecordingCoordinator> ();
    undo_stack_ = zrythm::actions::create_mock_undo_stack ();
  }

  void TearDown () override
  {
    materializer_.reset ();
    coordinator_.reset ();
    undo_stack_.reset ();
    source_obj_refs_.clear ();
    region_refs_.clear ();
    tempo_map_.reset ();
    app_.reset ();
  }

  std::optional<structure::arrangement::ArrangerObjectUuidReference>
  create_recording_region (
    TrackUuid                        track_id,
    units::sample_t                  start_position,
    const utils::audio::AudioBuffer &initial_frames)
  {
    auto clip_ref = utils::create_object<dsp::FileAudioSource> (
      registry_, initial_frames, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), 120.f, u8"RecordingClip");

    auto source_obj_ref =
      utils::create_object<structure::arrangement::AudioSourceObject> (
        registry_, *tempo_map_, registry_, clip_ref);

    auto region_ref = utils::create_object<structure::arrangement::AudioRegion> (
      registry_, *tempo_map_, registry_, [] () { return true; });

    auto * region = region_ref.get ();
    region->set_source (source_obj_ref);

    source_obj_refs_.push_back (std::move (source_obj_ref));
    region_refs_.push_back (region_ref);

    return region_ref;
  }

  void create_materializer (
    controllers::recording::RecordingMode mode =
      controllers::recording::RecordingMode::Takes)
  {
    region_create_count_ = 0;
    last_track_id_ = TrackUuid ();
    last_start_position_ = units::samples (0);
    last_lane_index_ = 0;

    materializer_ = std::make_unique<RecordingMaterializer> (
      *coordinator_, *undo_stack_,
      [this] (
        TrackUuid track_id, units::sample_t start_position,
        const utils::audio::AudioBuffer &initial_frames,
        size_t lane_index) -> RecordingMaterializer::RegionCreationResult {
        region_create_count_++;
        last_track_id_ = track_id;
        last_start_position_ = start_position;
        last_lane_index_ = lane_index;
        auto region_ref_opt =
          create_recording_region (track_id, start_position, initial_frames);
        if (!region_ref_opt.has_value ())
          return std::nullopt;
        return RecordingMaterializer::CreatedRegion{
          std::move (*region_ref_opt), lane_index
        };
      },
      [mode] () { return mode; });
  }

  void write_and_drain (
    TrackUuid       track_id,
    units::sample_t position,
    bool            transport_recording,
    int             num_frames = 256)
  {
    auto * session = coordinator_->session_for_track (track_id);
    ASSERT_NE (session, nullptr);

    std::vector<float> l (num_frames, 0.5f);
    std::vector<float> r (num_frames, 0.3f);
    session->write_samples (position, transport_recording, l, r);
    coordinator_->process_pending ();
  }

  structure::arrangement::AudioRegion * get_last_created_region () const
  {
    if (region_refs_.empty ())
      return nullptr;
    return region_refs_.back ()
      .get_object_as<structure::arrangement::AudioRegion> ();
  }

  dsp::FileAudioSource *
  get_clip_for_region (structure::arrangement::AudioRegion * region) const
  {
    if (region == nullptr)
      return nullptr;
    auto children = region->get_children_view ();
    if (children.empty ())
      return nullptr;
    auto clip_ref = children.front ()->audio_source_ref ();
    return clip_ref.get ();
  }

  std::unique_ptr<zrythm::test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<dsp::TempoMap>                                tempo_map_;
  std::unique_ptr<RecordingCoordinator>                         coordinator_;
  std::unique_ptr<undo::UndoStack>                              undo_stack_;
  std::unique_ptr<RecordingMaterializer>                        materializer_;

  utils::ObjectRegistry registry_;

  std::vector<structure::arrangement::ArrangerObjectUuidReference> region_refs_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    source_obj_refs_;

  int             region_create_count_ = 0;
  TrackUuid       last_track_id_;
  units::sample_t last_start_position_;
  size_t          last_lane_index_ = 0;
};

TEST_F (RecordingMaterializerTest, TransportRecordingFalseDiscardsData)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), false);

  EXPECT_EQ (region_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, SingleRegionCreatedOnFirstPacket)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);

  EXPECT_EQ (region_create_count_, 1);
  EXPECT_EQ (last_track_id_, track_id);
  EXPECT_EQ (last_start_position_.in (units::samples), 0);

  auto * region = get_last_created_region ();
  ASSERT_NE (region, nullptr);
  auto * clip = get_clip_for_region (region);
  ASSERT_NE (clip, nullptr);
  EXPECT_EQ (clip->get_num_frames (), 256);
  const auto expected_ticks =
    tempo_map_->samples_to_tick (units::samples (256.0));
  EXPECT_DOUBLE_EQ (
    region->bounds ()->length ()->ticks (), expected_ticks.in (units::ticks));
}

TEST_F (RecordingMaterializerTest, ContinuousPacketsSameRegion)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 1);

  write_and_drain (track_id, units::samples (256), true);
  EXPECT_EQ (region_create_count_, 1);

  write_and_drain (track_id, units::samples (512), true);
  EXPECT_EQ (region_create_count_, 1);

  auto * region = get_last_created_region ();
  ASSERT_NE (region, nullptr);
  auto * clip = get_clip_for_region (region);
  ASSERT_NE (clip, nullptr);
  EXPECT_EQ (clip->get_num_frames (), 768);
  const auto expected_ticks =
    tempo_map_->samples_to_tick (units::samples (768.0));
  EXPECT_DOUBLE_EQ (
    region->bounds ()->length ()->ticks (), expected_ticks.in (units::ticks));
}

TEST_F (RecordingMaterializerTest, DiscontinuityCreatesNewRegion)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 1);

  auto * first_region = get_last_created_region ();
  ASSERT_NE (first_region, nullptr);
  auto * first_clip = get_clip_for_region (first_region);
  ASSERT_NE (first_clip, nullptr);
  EXPECT_EQ (first_clip->get_num_frames (), 256);

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (region_create_count_, 2);
  EXPECT_EQ (last_start_position_.in (units::samples), 1000);

  auto * second_region = get_last_created_region ();
  ASSERT_NE (second_region, nullptr);
  auto * second_clip = get_clip_for_region (second_region);
  ASSERT_NE (second_clip, nullptr);
  EXPECT_EQ (second_clip->get_num_frames (), 256);
  const auto expected_ticks =
    tempo_map_->samples_to_tick (units::samples (256.0));
  EXPECT_DOUBLE_EQ (
    second_region->bounds ()->length ()->ticks (),
    expected_ticks.in (units::ticks));
}

TEST_F (RecordingMaterializerTest, TransportStopFinalizesMacro)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 1);

  EXPECT_FALSE (undo_stack_->canUndo ()) << "Macro should still be open";

  coordinator_->disarm_track (track_id);
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized after session ends";
}

TEST_F (RecordingMaterializerTest, MultipleTracksSameMacro)
{
  create_materializer ();

  auto track_a = TrackUuid (QUuid::createUuid ());
  auto track_b = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_a, units::samples (256));
  coordinator_->arm_track (track_b, units::samples (256));

  write_and_drain (track_a, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 1);

  write_and_drain (track_b, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 2);

  EXPECT_FALSE (undo_stack_->canUndo ()) << "Macro should still be open";

  coordinator_->disarm_track (track_a);
  coordinator_->process_pending ();
  EXPECT_FALSE (undo_stack_->canUndo ())
    << "Macro should remain open while track_b is still recording";

  write_and_drain (track_b, units::samples (256), true);
  EXPECT_EQ (region_create_count_, 2)
    << "Continuous write should extend existing region";

  write_and_drain (track_b, units::samples (1000), true);
  EXPECT_EQ (region_create_count_, 3)
    << "Discontinuous write should create new region";

  coordinator_->disarm_track (track_b);
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized only when all tracks done";
}

TEST_F (RecordingMaterializerTest, NullRegionCreatorDoesNotCorruptState)
{
  region_create_count_ = 0;

  materializer_ = std::make_unique<RecordingMaterializer> (
    *coordinator_, *undo_stack_,
    [] (TrackUuid, units::sample_t, const utils::audio::AudioBuffer &, size_t)
      -> RecordingMaterializer::RegionCreationResult { return std::nullopt; },
    [] () { return controllers::recording::RecordingMode::Takes; });

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 0);

  write_and_drain (track_id, units::samples (256), true);
  EXPECT_EQ (region_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, DisarmFinalizesMacro)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 1);

  EXPECT_FALSE (undo_stack_->canUndo ()) << "Macro should still be open";

  coordinator_->disarm_track (track_id);
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized after disarm and process_pending";
}

TEST_F (RecordingMaterializerTest, DisarmOneOfTwoTracksDoesNotFinalizeMacro)
{
  create_materializer ();

  auto track_a = TrackUuid (QUuid::createUuid ());
  auto track_b = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_a, units::samples (256));
  coordinator_->arm_track (track_b, units::samples (256));

  write_and_drain (track_a, units::samples (0), true);
  write_and_drain (track_b, units::samples (0), true);

  coordinator_->disarm_track (track_a);
  coordinator_->process_pending ();

  EXPECT_FALSE (undo_stack_->canUndo ())
    << "Macro should remain open while track_b is still recording";
}

TEST_F (RecordingMaterializerTest, DisarmAllTracksFinalizesMacro)
{
  create_materializer ();

  auto track_a = TrackUuid (QUuid::createUuid ());
  auto track_b = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_a, units::samples (256));
  coordinator_->arm_track (track_b, units::samples (256));

  write_and_drain (track_a, units::samples (0), true);
  write_and_drain (track_b, units::samples (0), true);

  coordinator_->disarm_track (track_a);
  coordinator_->process_pending ();

  coordinator_->disarm_track (track_b);
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized after all tracks disarmed";
}

TEST_F (RecordingMaterializerTest, TakesMutedModeMutesPrevious)
{
  create_materializer (controllers::recording::RecordingMode::TakesMuted);

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 1);

  auto * first_region = get_last_created_region ();
  ASSERT_NE (first_region, nullptr);
  EXPECT_FALSE (first_region->mute ()->muted ());

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (region_create_count_, 2);

  EXPECT_TRUE (first_region->mute ()->muted ())
    << "Previous region should be muted in TakesMuted mode";

  auto * second_region = get_last_created_region ();
  ASSERT_NE (second_region, nullptr);
  EXPECT_FALSE (second_region->mute ()->muted ());
}

TEST_F (RecordingMaterializerTest, TakesModeDoesNotMutePrevious)
{
  create_materializer (controllers::recording::RecordingMode::Takes);

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 1);

  auto * first_region = get_last_created_region ();
  ASSERT_NE (first_region, nullptr);

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (region_create_count_, 2);

  EXPECT_FALSE (first_region->mute ()->muted ())
    << "Previous region should NOT be muted in Takes mode";
}

TEST_F (RecordingMaterializerTest, MultipleLoopBacksMuteAllPrevious)
{
  create_materializer (controllers::recording::RecordingMode::TakesMuted);

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  write_and_drain (track_id, units::samples (1000), true);
  write_and_drain (track_id, units::samples (0), true);
  write_and_drain (track_id, units::samples (1000), true);

  EXPECT_EQ (region_create_count_, 4);

  EXPECT_TRUE (
    region_refs_[0]
      .get_object_as<structure::arrangement::AudioRegion> ()
      ->mute ()
      ->muted ());
  EXPECT_TRUE (
    region_refs_[1]
      .get_object_as<structure::arrangement::AudioRegion> ()
      ->mute ()
      ->muted ());
  EXPECT_TRUE (
    region_refs_[2]
      .get_object_as<structure::arrangement::AudioRegion> ()
      ->mute ()
      ->muted ());
  EXPECT_FALSE (
    region_refs_[3]
      .get_object_as<structure::arrangement::AudioRegion> ()
      ->mute ()
      ->muted ())
    << "Latest region should not be muted";
}

TEST_F (RecordingMaterializerTest, SessionResetClearsLaneIndex)
{
  create_materializer (controllers::recording::RecordingMode::Takes);

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (last_lane_index_, 0);

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (last_lane_index_, 1);

  coordinator_->disarm_track (track_id);
  coordinator_->process_pending ();

  coordinator_->arm_track (track_id, units::samples (256));

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (last_lane_index_, 0)
    << "Lane index should reset after session ends";
}

}
