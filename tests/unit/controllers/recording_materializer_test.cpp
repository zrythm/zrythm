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
using SessionType = RecordingCoordinator::SessionType;

class RecordingMaterializerTest : public ::testing::Test
{
protected:
  struct MidiNoteCreation
  {
    units::sample_t                      start_position;
    units::sample_t                      end_position;
    int                                  pitch;
    int                                  velocity;
    int                                  channel;
    structure::arrangement::MidiRegion * region;
  };

  struct MidiControlEventCreation
  {
    units::sample_t                                     position;
    structure::arrangement::MidiControlEvent::EventType type;
    int                                                 channel;
    int                                                 controller;
    int                                                 value;
    structure::arrangement::MidiRegion *                region;
  };

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
    midi_region_refs_.clear ();
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
    midi_region_create_count_ = 0;
    midi_note_creations_.clear ();
    midi_control_event_creations_.clear ();

    materializer_ = std::make_unique<RecordingMaterializer> (
      *coordinator_, *undo_stack_,
      RecordingMaterializer::ArrangerObjectCreators{
        .audio_region =
          [this] (
            TrackUuid track_id, units::sample_t start_position,
            const utils::audio::AudioBuffer &initial_frames, size_t lane_index)
          -> RecordingMaterializer::RegionCreationResult {
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
        .midi_region =
          [this] (
            TrackUuid track_id, units::sample_t start_position,
            size_t lane_index) -> RecordingMaterializer::RegionCreationResult {
          midi_region_create_count_++;
          last_midi_track_id_ = track_id;
          last_midi_start_position_ = start_position;
          last_midi_lane_index_ = lane_index;
          auto region_ref =
            utils::create_object<structure::arrangement::MidiRegion> (
              registry_, *tempo_map_, registry_);
          region_ref.get ()->position ()->setSamples (
            static_cast<double> (start_position.in (units::samples)));
          midi_region_refs_.push_back (region_ref);
          return RecordingMaterializer::CreatedRegion{
            std::move (region_ref), lane_index
          };
        },
        .midi_note =
          [this] (
            structure::arrangement::MidiRegion &region,
            units::sample_t start_position, units::sample_t end_position,
            int pitch, int velocity, int channel) {
            midi_note_creations_.push_back (
              { start_position, end_position, pitch, velocity, channel,
                &region });
          },
        .midi_control_event =
          [this] (
            structure::arrangement::MidiRegion &region, units::sample_t position,
            structure::arrangement::MidiControlEvent::EventType type,
            int channel, int controller, int value) {
            midi_control_event_creations_.push_back (
              { position, type, channel, controller, value, &region });
          },
      },
      [mode] () { return mode; });
  }

  void write_and_drain (
    TrackUuid       track_id,
    units::sample_t position,
    bool            transport_recording,
    int             num_frames = 256)
  {
    auto session = coordinator_->session_for_track (track_id);
    ASSERT_TRUE (std::holds_alternative<AudioRecordingSession *> (session));
    auto * s = std::get<AudioRecordingSession *> (session);

    std::vector<float> l (num_frames, 0.5f);
    std::vector<float> r (num_frames, 0.3f);
    s->write (position, transport_recording, l, r);
    coordinator_->process_pending ();
  }

  void write_midi_and_drain (
    TrackUuid             track_id,
    units::sample_t       position,
    bool                  transport_recording,
    dsp::MidiEventVector &events)
  {
    auto session = coordinator_->session_for_track (track_id);
    ASSERT_TRUE (std::holds_alternative<MidiRecordingSession *> (session));
    auto * s = std::get<MidiRecordingSession *> (session);
    s->write (position, transport_recording, events, units::samples (256u));
    coordinator_->process_pending ();
  }

  structure::arrangement::AudioRegion * get_last_created_region () const
  {
    if (region_refs_.empty ())
      return nullptr;
    return region_refs_.back ()
      .get_object_as<structure::arrangement::AudioRegion> ();
  }

  static dsp::FileAudioSource *
  get_clip_for_region (structure::arrangement::AudioRegion * region)
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
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    midi_region_refs_;

  int             region_create_count_ = 0;
  TrackUuid       last_track_id_;
  units::sample_t last_start_position_;
  size_t          last_lane_index_ = 0;

  int                                   midi_region_create_count_ = 0;
  TrackUuid                             last_midi_track_id_;
  units::sample_t                       last_midi_start_position_;
  size_t                                last_midi_lane_index_ = 0;
  std::vector<MidiNoteCreation>         midi_note_creations_;
  std::vector<MidiControlEventCreation> midi_control_event_creations_;
};

TEST_F (RecordingMaterializerTest, TransportRecordingFalseDiscardsData)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), false);

  EXPECT_EQ (region_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, SingleRegionCreatedOnFirstPacket)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_a, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_b, units::samples (256), SessionType::Audio);

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
    RecordingMaterializer::ArrangerObjectCreators{
      .audio_region =
        [] (TrackUuid, units::sample_t, const utils::audio::AudioBuffer &, size_t)
        -> RecordingMaterializer::RegionCreationResult { return std::nullopt; },
      .midi_region = [] (TrackUuid, units::sample_t, size_t)
        -> RecordingMaterializer::RegionCreationResult { return std::nullopt; },
      .midi_note =
        [] (
          structure::arrangement::MidiRegion &, units::sample_t,
          units::sample_t, int, int, int) { },
      .midi_control_event =
        [] (
          structure::arrangement::MidiRegion &, units::sample_t,
          structure::arrangement::MidiControlEvent::EventType, int, int, int) { },
    },
    [] () { return controllers::recording::RecordingMode::Takes; });

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (region_create_count_, 0);

  write_and_drain (track_id, units::samples (256), true);
  EXPECT_EQ (region_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, DisarmFinalizesMacro)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_a, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_b, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_a, units::samples (256), SessionType::Audio);
  coordinator_->arm_track (track_b, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

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
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (last_lane_index_, 0);

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (last_lane_index_, 1);

  coordinator_->disarm_track (track_id);
  coordinator_->process_pending ();

  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (last_lane_index_, 0)
    << "Lane index should reset after session ends";
}

// ============================================================================
// MIDI recording tests
// ============================================================================

TEST_F (RecordingMaterializerTest, MidiNoteOnOffCreatesNote)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector note_on;
  note_on.add_note_on (1, 60, 100, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, note_on);

  EXPECT_EQ (midi_region_create_count_, 1) << "Region created on note-on";

  dsp::MidiEventVector note_off;
  note_off.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (256), true, note_off);

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60);
  EXPECT_EQ (midi_note_creations_[0].velocity, 100);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_region_create_count_, 1);
  EXPECT_EQ (last_midi_track_id_, track_id);
}

TEST_F (RecordingMaterializerTest, MidiNoteOnOffInSamePacket)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_note_on (1, 64, 80, units::samples (0u));
  events.add_note_off (1, 64, units::samples (200u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 64);
  EXPECT_EQ (midi_note_creations_[0].velocity, 80);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_region_create_count_, 1);
}

TEST_F (RecordingMaterializerTest, MidiMultipleNotesCreateMultipleNotes)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_on (1, 64, 90, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  dsp::MidiEventVector events2;
  events2.add_note_off (1, 60, units::samples (100u));
  events2.add_note_off (1, 64, units::samples (150u));
  write_midi_and_drain (track_id, units::samples (256), true, events2);

  ASSERT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60);
  EXPECT_EQ (midi_note_creations_[0].velocity, 100);
  EXPECT_EQ (midi_note_creations_[1].pitch, 64);
  EXPECT_EQ (midi_note_creations_[1].velocity, 90);
  EXPECT_EQ (midi_region_create_count_, 1)
    << "All notes in same contiguous block share one region";
}

TEST_F (RecordingMaterializerTest, MidiNoteOnWithoutOff_ForceCompletedAtDisarm)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_note_on (1, 60, 100, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  coordinator_->disarm_track (track_id);
  coordinator_->process_pending ();

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60);
  EXPECT_EQ (midi_note_creations_[0].velocity, 100);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_note_creations_[0].end_position, units::samples (256))
    << "Incomplete note should be force-completed at last packet end";
}

TEST_F (
  RecordingMaterializerTest,
  MidiNoteOnWithoutOff_ForceCompletedPreservesChannel)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_note_on (3, 60, 100, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  coordinator_->disarm_track (track_id);
  coordinator_->process_pending ();

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].channel, 2)
    << "Force-completed note should preserve original MIDI channel";
}

TEST_F (RecordingMaterializerTest, MidiCCCreatesControlEvent)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_control_change (1, 1, 64, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  using EventType = structure::arrangement::MidiControlEvent::EventType;
  EXPECT_EQ (midi_control_event_creations_[0].type, EventType::ControlChange);
  EXPECT_EQ (midi_control_event_creations_[0].channel, 0);
  EXPECT_EQ (midi_control_event_creations_[0].controller, 1);
  EXPECT_EQ (midi_control_event_creations_[0].value, 64);
  EXPECT_EQ (midi_region_create_count_, 1);
}

TEST_F (RecordingMaterializerTest, MidiPitchBendCreatesControlEvent)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  uint8_t              pb_lsb = 0x00;
  uint8_t              pb_msb = 0x40;
  events.add_simple (0xE0, pb_lsb, pb_msb, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  using EventType = structure::arrangement::MidiControlEvent::EventType;
  EXPECT_EQ (midi_control_event_creations_[0].type, EventType::PitchBend);
  EXPECT_EQ (midi_control_event_creations_[0].channel, 0);
  EXPECT_EQ (midi_control_event_creations_[0].controller, 0);
  EXPECT_EQ (midi_control_event_creations_[0].value, (pb_msb << 7) | pb_lsb);
}

TEST_F (RecordingMaterializerTest, MidiChannelPressureCreatesControlEvent)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_channel_pressure (1, 96, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  using EventType = structure::arrangement::MidiControlEvent::EventType;
  EXPECT_EQ (midi_control_event_creations_[0].type, EventType::ChannelPressure);
  EXPECT_EQ (midi_control_event_creations_[0].channel, 0);
  EXPECT_EQ (midi_control_event_creations_[0].controller, 0);
  EXPECT_EQ (midi_control_event_creations_[0].value, 96);
  EXPECT_EQ (midi_region_create_count_, 1);
}

TEST_F (RecordingMaterializerTest, MidiPolyAftertouchCreatesControlEvent)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_simple (0xA0, 60, 80, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  using EventType = structure::arrangement::MidiControlEvent::EventType;
  EXPECT_EQ (midi_control_event_creations_[0].type, EventType::PolyKeyPressure);
  EXPECT_EQ (midi_control_event_creations_[0].channel, 0);
  EXPECT_EQ (midi_control_event_creations_[0].controller, 60);
  EXPECT_EQ (midi_control_event_creations_[0].value, 80);
}

TEST_F (RecordingMaterializerTest, MidiProgramChangeCreatesControlEvent)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_simple (0xC0, 42, 0, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  using EventType = structure::arrangement::MidiControlEvent::EventType;
  EXPECT_EQ (midi_control_event_creations_[0].type, EventType::ProgramChange);
  EXPECT_EQ (midi_control_event_creations_[0].channel, 0);
  EXPECT_EQ (midi_control_event_creations_[0].controller, 0);
  EXPECT_EQ (midi_control_event_creations_[0].value, 42);
}

TEST_F (RecordingMaterializerTest, MidiTransportRecordingFalseDiscardsEvents)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_note_on (1, 60, 100, units::samples (0u));
  events.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), false, events);

  EXPECT_TRUE (midi_note_creations_.empty ());
  EXPECT_EQ (midi_region_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, MidiDiscontinuityCreatesNewRegion)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (midi_region_create_count_, 1);

  dsp::MidiEventVector events2;
  events2.add_note_on (1, 62, 90, units::samples (0u));
  events2.add_note_off (1, 62, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  EXPECT_EQ (midi_region_create_count_, 2);
  EXPECT_EQ (midi_note_creations_.size (), 2u);
}

TEST_F (RecordingMaterializerTest, MidiTakesMutedModeMutesPrevious)
{
  create_materializer (controllers::recording::RecordingMode::TakesMuted);

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  ASSERT_EQ (midi_region_create_count_, 1);

  auto * first_region =
    midi_region_refs_[0].get_object_as<structure::arrangement::MidiRegion> ();
  ASSERT_NE (first_region, nullptr);
  EXPECT_FALSE (first_region->mute ()->muted ());

  dsp::MidiEventVector events2;
  events2.add_note_on (1, 62, 90, units::samples (0u));
  events2.add_note_off (1, 62, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  EXPECT_EQ (midi_region_create_count_, 2);

  EXPECT_TRUE (first_region->mute ()->muted ())
    << "Previous MIDI region should be muted in TakesMuted mode";
}

TEST_F (RecordingMaterializerTest, MidiLaneIndexIncrementsOnNewRegion)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (last_midi_lane_index_, 0u);

  dsp::MidiEventVector events2;
  events2.add_note_on (1, 62, 90, units::samples (0u));
  events2.add_note_off (1, 62, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  EXPECT_EQ (last_midi_lane_index_, 1u);
}

TEST_F (RecordingMaterializerTest, MidiNoteAndCCShareRegion)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_note_on (1, 60, 100, units::samples (0u));
  events.add_control_change (1, 1, 64, units::samples (50u));
  events.add_note_off (1, 60, units::samples (200u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  EXPECT_EQ (midi_region_create_count_, 1)
    << "Note and CC in same contiguous block share one region";
  EXPECT_EQ (last_midi_start_position_, units::samples (0))
    << "Region should be created at note-on position, not CC position";
  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_GE (midi_note_creations_[0].start_position.in (units::samples), 0)
    << "Note start should not be negative relative to region";
  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  EXPECT_EQ (
    midi_note_creations_[0].region, midi_control_event_creations_[0].region);
}

TEST_F (RecordingMaterializerTest, MidiNotePositionsRecordedAsSamples)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector note_on;
  note_on.add_note_on (1, 60, 100, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, note_on);

  dsp::MidiEventVector note_off;
  note_off.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (256), true, note_off);

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_note_creations_[0].end_position, units::samples (256 + 100));
}

TEST_F (
  RecordingMaterializerTest,
  MidiSamePitchDifferentChannelsTrackedSeparately)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_on (2, 60, 80, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  dsp::MidiEventVector events2;
  events2.add_note_off (1, 60, units::samples (100u));
  events2.add_note_off (2, 60, units::samples (200u));
  write_midi_and_drain (track_id, units::samples (256), true, events2);

  ASSERT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60);
  EXPECT_EQ (midi_note_creations_[0].velocity, 100);
  EXPECT_EQ (midi_note_creations_[0].channel, 0)
    << "Note on MIDI channel 1 should be stored as channel 0";
  EXPECT_EQ (midi_note_creations_[1].pitch, 60);
  EXPECT_EQ (midi_note_creations_[1].velocity, 80);
  EXPECT_EQ (midi_note_creations_[1].channel, 1)
    << "Note on MIDI channel 2 should be stored as channel 1";
}

TEST_F (RecordingMaterializerTest, MidiHighBitDataBytesMasked)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events;
  events.add_note_on (1, 0x80, 0xFF, units::samples (0u));
  events.add_note_off (1, 0x80, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 0x00)
    << "High bit of pitch byte should be masked";
  EXPECT_EQ (midi_note_creations_[0].velocity, 0x7F)
    << "High bit of velocity byte should be masked";
}

TEST_F (RecordingMaterializerTest, MidiContiguousPacketsShareSameRegion)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_off (1, 60, units::samples (100u));
  events1.add_control_change (1, 1, 64, units::samples (150u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (midi_region_create_count_, 1);

  dsp::MidiEventVector events2;
  events2.add_note_on (1, 64, 80, units::samples (0u));
  events2.add_note_off (1, 64, units::samples (200u));
  write_midi_and_drain (track_id, units::samples (256), true, events2);

  EXPECT_EQ (midi_region_create_count_, 1)
    << "Contiguous MIDI packets should share one region";
  EXPECT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_control_event_creations_.size (), 1u);
}

TEST_F (RecordingMaterializerTest, MidiEmptyPacketsMaintainContiguity)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (midi_region_create_count_, 1);

  dsp::MidiEventVector empty_events;
  write_midi_and_drain (track_id, units::samples (256), true, empty_events);

  dsp::MidiEventVector events2;
  events2.add_note_on (1, 62, 90, units::samples (0u));
  events2.add_note_off (1, 62, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (512), true, events2);

  EXPECT_EQ (midi_region_create_count_, 1)
    << "Empty MIDI packets should not break contiguity";
  EXPECT_EQ (midi_note_creations_.size (), 2u);
}

TEST_F (RecordingMaterializerTest, MidiNotePositionsAreRegionRelative)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_off (1, 60, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_note_creations_[0].end_position, units::samples (100));

  dsp::MidiEventVector events2;
  events2.add_note_on (1, 60, 100, units::samples (10u));
  events2.add_note_on (1, 64, 90, units::samples (50u));
  write_midi_and_drain (track_id, units::samples (1000), true, events2);

  dsp::MidiEventVector events3;
  events3.add_note_off (1, 60, units::samples (100u));
  events3.add_note_off (1, 64, units::samples (200u));
  write_midi_and_drain (track_id, units::samples (1256), true, events3);

  ASSERT_EQ (midi_note_creations_.size (), 3u);
  EXPECT_EQ (midi_note_creations_[1].start_position, units::samples (0))
    << "First note in second region starts at region start (1010)";
  EXPECT_EQ (midi_note_creations_[1].end_position, units::samples (346));
  EXPECT_EQ (midi_note_creations_[2].start_position, units::samples (40))
    << "Second note start (1050) relative to region start (1010)";
  EXPECT_EQ (midi_note_creations_[2].end_position, units::samples (446))
    << "Second note end (1456) relative to region start (1010)";
}

TEST_F (RecordingMaterializerTest, MidiCCPositionIsRegionRelative)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_off (1, 60, units::samples (50u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  ASSERT_EQ (midi_region_create_count_, 1);
  ASSERT_EQ (midi_control_event_creations_.size (), 0u);

  dsp::MidiEventVector events2;
  events2.add_control_change (1, 1, 64, units::samples (50u));
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  ASSERT_EQ (midi_region_create_count_, 2);
  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  EXPECT_EQ (midi_control_event_creations_[0].position, units::samples (0))
    << "CC at 1050 relative to region start (1050)";
}

TEST_F (RecordingMaterializerTest, MidiStaleNotesForceCompletedOnDiscontinuity)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  dsp::MidiEventVector events2;
  events2.add_note_on (1, 64, 80, units::samples (0u));
  events2.add_note_off (1, 64, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (1000), true, events2);

  dsp::MidiEventVector events3;
  events3.add_note_off (1, 60, units::samples (0u));
  write_midi_and_drain (track_id, units::samples (1000 + 256), true, events3);

  ASSERT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60)
    << "Stale note from before discontinuity should be force-completed";
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_note_creations_[0].end_position, units::samples (256))
    << "Force-completed at last_end_position before discontinuity";
  EXPECT_EQ (midi_note_creations_[1].pitch, 64);
}

TEST_F (
  RecordingMaterializerTest,
  MidiRepeatedNoteOnSamePitchCreatesSeparateNotesFIFO)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  dsp::MidiEventVector events1;
  events1.add_note_on (1, 60, 100, units::samples (0u));
  events1.add_note_on (1, 60, 80, units::samples (100u));
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  dsp::MidiEventVector events2;
  events2.add_note_off (1, 60, units::samples (50u));
  events2.add_note_off (1, 60, units::samples (150u));
  write_midi_and_drain (track_id, units::samples (256), true, events2);

  ASSERT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60);
  EXPECT_EQ (midi_note_creations_[0].velocity, 100);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_note_creations_[0].end_position, units::samples (256 + 50))
    << "First note ends at first note-off";
  EXPECT_EQ (midi_note_creations_[1].pitch, 60);
  EXPECT_EQ (midi_note_creations_[1].velocity, 80);
  EXPECT_EQ (midi_note_creations_[1].start_position, units::samples (100));
  EXPECT_EQ (midi_note_creations_[1].end_position, units::samples (256 + 150))
    << "Second note ends at second note-off";
  EXPECT_EQ (midi_region_create_count_, 1);
}

}
