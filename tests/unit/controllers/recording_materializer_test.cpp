// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/recording_materializer.h"
#include "dsp/midi_event.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_fwd.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"
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
    units::sample_t                    start_position;
    units::sample_t                    end_position;
    int                                pitch;
    int                                velocity;
    int                                channel;
    structure::arrangement::MidiClip * clip;
  };

  struct MidiControlEventCreation
  {
    units::sample_t                                     position;
    structure::arrangement::MidiControlEvent::EventType type;
    int                                                 channel;
    int                                                 controller;
    int                                                 value;
    structure::arrangement::MidiClip *                  clip;
  };

  void SetUp () override
  {
    app_ = std::make_unique<zrythm::test_helpers::ScopedQCoreApplication> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());
    coordinator_ = std::make_unique<RecordingCoordinator> ();
    undo_stack_ = zrythm::actions::create_mock_undo_stack ();
  }

  void TearDown () override
  {
    materializer_.reset ();
    coordinator_.reset ();
    undo_stack_.reset ();
    source_obj_refs_.clear ();
    clip_refs_.clear ();
    midi_clip_refs_.clear ();
    tempo_map_.reset ();
    tempo_map_wrapper_.reset ();
    app_.reset ();
  }

  std::optional<structure::arrangement::ArrangerObjectUuidReference>
  create_recording_clip (
    TrackUuid                        track_id,
    units::sample_t                  start_position,
    const utils::audio::AudioBuffer &initial_frames)
  {
    auto audio_source_ref = utils::create_object<dsp::FileAudioSource> (
      registry_, initial_frames, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), u8"RecordingClip");

    auto source_obj_ref =
      utils::create_object<structure::arrangement::AudioSourceObject> (
        registry_, *tempo_map_wrapper_, registry_, audio_source_ref);

    auto clip_ref = utils::create_object<structure::arrangement::AudioClip> (
      registry_, *tempo_map_wrapper_, registry_);

    auto * clip = clip_ref.get ();
    clip->set_source (source_obj_ref);
    clip->position ()->setTicks (
      tempo_map_->samples_to_tick (start_position).asDouble ());

    source_obj_refs_.push_back (std::move (source_obj_ref));
    clip_refs_.push_back (clip_ref);

    return clip_ref;
  }

  void create_materializer (
    controllers::recording::RecordingMode mode =
      controllers::recording::RecordingMode::Takes)
  {
    clip_create_count_ = 0;
    last_track_id_ = TrackUuid ();
    last_start_position_ = units::samples (0);
    last_lane_index_ = 0;
    midi_clip_create_count_ = 0;
    midi_note_creations_.clear ();
    midi_control_event_creations_.clear ();

    materializer_ = std::make_unique<RecordingMaterializer> (
      *coordinator_, *undo_stack_,
      RecordingMaterializer::ArrangerObjectCreators{
        .audio_clip =
          [this] (
            TrackUuid track_id, units::sample_t start_position,
            const utils::audio::AudioBuffer &initial_frames, size_t lane_index)
          -> RecordingMaterializer::ClipCreationResult {
          clip_create_count_++;
          last_track_id_ = track_id;
          last_start_position_ = start_position;
          last_lane_index_ = lane_index;
          auto clip_ref_opt =
            create_recording_clip (track_id, start_position, initial_frames);
          if (!clip_ref_opt.has_value ())
            return std::nullopt;
          return RecordingMaterializer::CreatedClip{
            std::move (*clip_ref_opt), lane_index
          };
        },
        .midi_clip =
          [this] (
            TrackUuid track_id, units::sample_t start_position,
            size_t lane_index) -> RecordingMaterializer::ClipCreationResult {
          midi_clip_create_count_++;
          last_midi_track_id_ = track_id;
          last_midi_start_position_ = start_position;
          last_midi_lane_index_ = lane_index;
          auto clip_ref = utils::create_object<structure::arrangement::MidiClip> (
            registry_, *tempo_map_wrapper_, registry_);
          clip_ref.get ()->position ()->setTicks (
            tempo_map_
              ->samples_to_tick (
                units::samples (start_position.in (units::samples)))
              .asDouble ());
          midi_clip_refs_.push_back (clip_ref);
          return RecordingMaterializer::CreatedClip{
            std::move (clip_ref), lane_index
          };
        },
        .midi_note =
          [this] (
            structure::arrangement::MidiClip &clip, units::sample_t start_position,
            units::sample_t end_position, int pitch, int velocity, int channel) {
            midi_note_creations_.push_back (
              { start_position, end_position, pitch, velocity, channel, &clip });
          },
        .midi_control_event =
          [this] (
            structure::arrangement::MidiClip &clip, units::sample_t position,
            structure::arrangement::MidiControlEvent::EventType type,
            int channel, int controller, int value) {
            midi_control_event_creations_.push_back (
              { position, type, channel, controller, value, &clip });
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
    TrackUuid                   track_id,
    units::sample_t             position,
    bool                        transport_recording,
    const dsp::MidiEventBuffer &events)
  {
    auto session = coordinator_->session_for_track (track_id);
    ASSERT_TRUE (std::holds_alternative<MidiRecordingSession *> (session));
    auto * s = std::get<MidiRecordingSession *> (session);
    s->write (position, transport_recording, events, units::samples (256u));
    coordinator_->process_pending ();
  }

  structure::arrangement::AudioClip * get_last_created_clip () const
  {
    if (clip_refs_.empty ())
      return nullptr;
    return clip_refs_.back ().get_object_as<structure::arrangement::AudioClip> ();
  }

  static dsp::FileAudioSource *
  get_audio_source_for_clip (structure::arrangement::AudioClip * clip)
  {
    if (clip == nullptr)
      return nullptr;
    auto children = clip->get_children_view ();
    if (children.empty ())
      return nullptr;
    auto clip_ref = children.front ()->audio_source_ref ();
    return clip_ref.get ();
  }

  std::unique_ptr<zrythm::test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<dsp::TempoMap>                                tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>  tempo_map_wrapper_;
  std::unique_ptr<RecordingCoordinator>  coordinator_;
  std::unique_ptr<undo::UndoStack>       undo_stack_;
  std::unique_ptr<RecordingMaterializer> materializer_;

  utils::ObjectRegistry               registry_;
  std::unique_ptr<utils::AppSettings> app_settings_;

  std::vector<structure::arrangement::ArrangerObjectUuidReference> clip_refs_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    source_obj_refs_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    midi_clip_refs_;

  int             clip_create_count_ = 0;
  TrackUuid       last_track_id_;
  units::sample_t last_start_position_;
  size_t          last_lane_index_ = 0;

  int                                   midi_clip_create_count_ = 0;
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

  EXPECT_EQ (clip_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, SingleClipCreatedOnFirstPacket)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);

  EXPECT_EQ (clip_create_count_, 1);
  EXPECT_EQ (last_track_id_, track_id);
  EXPECT_EQ (last_start_position_.in (units::samples), 0);

  auto * clip = get_last_created_clip ();
  ASSERT_NE (clip, nullptr);
  auto * source = get_audio_source_for_clip (clip);
  ASSERT_NE (source, nullptr);
  EXPECT_EQ (source->get_num_frames (), 256);
  const auto expected_ticks =
    tempo_map_->samples_to_tick (units::samples (256.0));
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), expected_ticks.asDouble ());
}

TEST_F (RecordingMaterializerTest, AudioClipLengthWithTempoChangeBeforePosition)
{
  // Tempo: 120 BPM (0..1 s) then 240 BPM afterwards. The clip is recorded
  // entirely within the 240 BPM region, so its length in ticks must be computed
  // at 240 BPM. Treating the frame count as an absolute position from the
  // origin (the old code) converts it through the 120 BPM region instead.
  tempo_map_->add_tempo_event (
    units::ticks (1920), units::bpm (240.0), dsp::TempoMap::CurveType::Constant);
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  // Record 256 frames starting at 2 s (88200 samples) — inside 240 BPM.
  write_and_drain (track_id, units::samples (88200), true);

  auto * clip = get_last_created_clip ();
  ASSERT_NE (clip, nullptr);
  auto * source = get_audio_source_for_clip (clip);
  ASSERT_NE (source, nullptr);
  EXPECT_EQ (source->get_num_frames (), 256);

  // Correct length: tick span of [88200, 88456] at the recording tempo.
  const auto correct_length =
    tempo_map_->samples_to_tick (units::samples (88456.0))
    - tempo_map_->samples_to_tick (units::samples (88200.0));
  EXPECT_NEAR (clip->length ()->ticks (), correct_length.asDouble (), 1.0);
}

TEST_F (RecordingMaterializerTest, ContinuousPacketsSameClip)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (clip_create_count_, 1);

  write_and_drain (track_id, units::samples (256), true);
  EXPECT_EQ (clip_create_count_, 1);

  write_and_drain (track_id, units::samples (512), true);
  EXPECT_EQ (clip_create_count_, 1);

  auto * clip = get_last_created_clip ();
  ASSERT_NE (clip, nullptr);
  auto * source = get_audio_source_for_clip (clip);
  ASSERT_NE (source, nullptr);
  EXPECT_EQ (source->get_num_frames (), 768);
  const auto expected_ticks =
    tempo_map_->samples_to_tick (units::samples (768.0));
  EXPECT_DOUBLE_EQ (clip->length ()->ticks (), expected_ticks.asDouble ());
}

TEST_F (RecordingMaterializerTest, DiscontinuityCreatesNewClip)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (clip_create_count_, 1);

  auto * first_clip = get_last_created_clip ();
  ASSERT_NE (first_clip, nullptr);
  auto * first_source = get_audio_source_for_clip (first_clip);
  ASSERT_NE (first_source, nullptr);
  EXPECT_EQ (first_source->get_num_frames (), 256);

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (clip_create_count_, 2);
  EXPECT_EQ (last_start_position_.in (units::samples), 1000);

  auto * second_clip = get_last_created_clip ();
  ASSERT_NE (second_clip, nullptr);
  auto * second_source = get_audio_source_for_clip (second_clip);
  ASSERT_NE (second_source, nullptr);
  EXPECT_EQ (second_source->get_num_frames (), 256);
  const auto expected_ticks =
    tempo_map_->samples_to_tick (units::samples (256.0));
  EXPECT_DOUBLE_EQ (
    second_clip->length ()->ticks (), expected_ticks.asDouble ());
}

TEST_F (RecordingMaterializerTest, TransportStopFinalizesMacro)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (clip_create_count_, 1);

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
  EXPECT_EQ (clip_create_count_, 1);

  write_and_drain (track_b, units::samples (0), true);
  EXPECT_EQ (clip_create_count_, 2);

  EXPECT_FALSE (undo_stack_->canUndo ()) << "Macro should still be open";

  coordinator_->disarm_track (track_a);
  coordinator_->process_pending ();
  EXPECT_FALSE (undo_stack_->canUndo ())
    << "Macro should remain open while track_b is still recording";

  write_and_drain (track_b, units::samples (256), true);
  EXPECT_EQ (clip_create_count_, 2)
    << "Continuous write should extend existing clip";

  write_and_drain (track_b, units::samples (1000), true);
  EXPECT_EQ (clip_create_count_, 3)
    << "Discontinuous write should create new clip";

  coordinator_->disarm_track (track_b);
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized only when all tracks done";
}

TEST_F (RecordingMaterializerTest, NullClipCreatorDoesNotCorruptState)
{
  clip_create_count_ = 0;

  materializer_ = std::make_unique<RecordingMaterializer> (
    *coordinator_, *undo_stack_,
    RecordingMaterializer::ArrangerObjectCreators{
      .audio_clip =
        [] (TrackUuid, units::sample_t, const utils::audio::AudioBuffer &, size_t)
        -> RecordingMaterializer::ClipCreationResult { return std::nullopt; },
      .midi_clip = [] (TrackUuid, units::sample_t, size_t)
        -> RecordingMaterializer::ClipCreationResult { return std::nullopt; },
      .midi_note =
        [] (
          structure::arrangement::MidiClip &, units::sample_t, units::sample_t,
          int, int, int) { },
      .midi_control_event =
        [] (
          structure::arrangement::MidiClip &, units::sample_t,
          structure::arrangement::MidiControlEvent::EventType, int, int, int) { },
    },
    [] () { return controllers::recording::RecordingMode::Takes; });

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (clip_create_count_, 0);

  write_and_drain (track_id, units::samples (256), true);
  EXPECT_EQ (clip_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, DisarmFinalizesMacro)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (clip_create_count_, 1);

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
  EXPECT_EQ (clip_create_count_, 1);

  auto * first_clip = get_last_created_clip ();
  ASSERT_NE (first_clip, nullptr);
  EXPECT_FALSE (first_clip->mute ()->muted ());

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (clip_create_count_, 2);

  EXPECT_TRUE (first_clip->mute ()->muted ())
    << "Previous clip should be muted in TakesMuted mode";

  auto * second_clip = get_last_created_clip ();
  ASSERT_NE (second_clip, nullptr);
  EXPECT_FALSE (second_clip->mute ()->muted ());
}

TEST_F (RecordingMaterializerTest, TakesModeDoesNotMutePrevious)
{
  create_materializer (controllers::recording::RecordingMode::Takes);

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Audio);

  write_and_drain (track_id, units::samples (0), true);
  EXPECT_EQ (clip_create_count_, 1);

  auto * first_clip = get_last_created_clip ();
  ASSERT_NE (first_clip, nullptr);

  write_and_drain (track_id, units::samples (1000), true);
  EXPECT_EQ (clip_create_count_, 2);

  EXPECT_FALSE (first_clip->mute ()->muted ())
    << "Previous clip should NOT be muted in Takes mode";
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

  EXPECT_EQ (clip_create_count_, 4);

  EXPECT_TRUE (
    clip_refs_[0]
      .get_object_as<structure::arrangement::AudioClip> ()
      ->mute ()
      ->muted ());
  EXPECT_TRUE (
    clip_refs_[1]
      .get_object_as<structure::arrangement::AudioClip> ()
      ->mute ()
      ->muted ());
  EXPECT_TRUE (
    clip_refs_[2]
      .get_object_as<structure::arrangement::AudioClip> ()
      ->mute ()
      ->muted ());
  EXPECT_FALSE (
    clip_refs_[3]
      .get_object_as<structure::arrangement::AudioClip> ()
      ->mute ()
      ->muted ())
    << "Latest clip should not be muted";
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

  auto note_on = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    note_on.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, note_on);

  EXPECT_EQ (midi_clip_create_count_, 1) << "Clip created on note-on";

  auto note_off = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    note_off.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (256), true, note_off);

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60);
  EXPECT_EQ (midi_note_creations_[0].velocity, 100);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_clip_create_count_, 1);
  EXPECT_EQ (last_midi_track_id_, track_id);
}

TEST_F (RecordingMaterializerTest, MidiNoteOnOffInSamePacket)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 64, 80, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 64, units::samples (200u));
    events.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 64);
  EXPECT_EQ (midi_note_creations_[0].velocity, 80);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_clip_create_count_, 1);
}

TEST_F (RecordingMaterializerTest, MidiMultipleNotesCreateMultipleNotes)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 64, 90, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 64, units::samples (150u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (256), true, events2);

  ASSERT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60);
  EXPECT_EQ (midi_note_creations_[0].velocity, 100);
  EXPECT_EQ (midi_note_creations_[1].pitch, 64);
  EXPECT_EQ (midi_note_creations_[1].velocity, 90);
  EXPECT_EQ (midi_clip_create_count_, 1)
    << "All notes in same contiguous block share one clip";
}

TEST_F (RecordingMaterializerTest, MidiNoteOnWithoutOff_ForceCompletedAtDisarm)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
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

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (2, 60, 100, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
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

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_control_change (0, 1, 64, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  using EventType = structure::arrangement::MidiControlEvent::EventType;
  EXPECT_EQ (midi_control_event_creations_[0].type, EventType::ControlChange);
  EXPECT_EQ (midi_control_event_creations_[0].channel, 0);
  EXPECT_EQ (midi_control_event_creations_[0].controller, 1);
  EXPECT_EQ (midi_control_event_creations_[0].value, 64);
  EXPECT_EQ (midi_clip_create_count_, 1);
}

TEST_F (RecordingMaterializerTest, MidiPitchBendCreatesControlEvent)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto    events = dsp::MidiEventBuffer::make_reserved ();
  uint8_t pb_lsb = 0x00;
  uint8_t pb_msb = 0x40;
  {
    const auto _ev = dsp::midi_event::make_pitchbend (
      0, (pb_msb << 7) | pb_lsb, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
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

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_channel_pressure (0, 96, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  using EventType = structure::arrangement::MidiControlEvent::EventType;
  EXPECT_EQ (midi_control_event_creations_[0].type, EventType::ChannelPressure);
  EXPECT_EQ (midi_control_event_creations_[0].channel, 0);
  EXPECT_EQ (midi_control_event_creations_[0].controller, 0);
  EXPECT_EQ (midi_control_event_creations_[0].value, 96);
  EXPECT_EQ (midi_clip_create_count_, 1);
}

TEST_F (RecordingMaterializerTest, MidiPolyAftertouchCreatesControlEvent)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev = dsp::midi_event::make_raw (
      std::array<midi_byte_t, 3>{ 0xA0, 60, 80 }, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
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

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev = dsp::midi_event::make_raw (
      std::array<midi_byte_t, 3>{ 0xC0, 42, 0 }, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
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

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), false, events);

  EXPECT_TRUE (midi_note_creations_.empty ());
  EXPECT_EQ (midi_clip_create_count_, 0);
}

TEST_F (RecordingMaterializerTest, MidiDiscontinuityCreatesNewClip)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (midi_clip_create_count_, 1);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 62, 90, units::samples (0u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 62, units::samples (100u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  EXPECT_EQ (midi_clip_create_count_, 2);
  EXPECT_EQ (midi_note_creations_.size (), 2u);
}

TEST_F (RecordingMaterializerTest, MidiTakesMutedModeMutesPrevious)
{
  create_materializer (controllers::recording::RecordingMode::TakesMuted);

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  ASSERT_EQ (midi_clip_create_count_, 1);

  auto * first_clip =
    midi_clip_refs_[0].get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (first_clip, nullptr);
  EXPECT_FALSE (first_clip->mute ()->muted ());

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 62, 90, units::samples (0u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 62, units::samples (100u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  EXPECT_EQ (midi_clip_create_count_, 2);

  EXPECT_TRUE (first_clip->mute ()->muted ())
    << "Previous MIDI clip should be muted in TakesMuted mode";
}

TEST_F (RecordingMaterializerTest, MidiLaneIndexIncrementsOnNewClip)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (last_midi_lane_index_, 0u);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 62, 90, units::samples (0u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 62, units::samples (100u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  EXPECT_EQ (last_midi_lane_index_, 1u);
}

TEST_F (RecordingMaterializerTest, MidiNoteAndCCShareClip)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_control_change (0, 1, 64, units::samples (50u));
    events.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (200u));
    events.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events);

  EXPECT_EQ (midi_clip_create_count_, 1)
    << "Note and CC in same contiguous block share one clip";
  EXPECT_EQ (last_midi_start_position_, units::samples (0))
    << "Clip should be created at note-on position, not CC position";
  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_GE (midi_note_creations_[0].start_position.in (units::samples), 0)
    << "Note start should not be negative relative to clip";
  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  EXPECT_EQ (
    midi_note_creations_[0].clip, midi_control_event_creations_[0].clip);
}

TEST_F (RecordingMaterializerTest, MidiNotePositionsRecordedAsSamples)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto note_on = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    note_on.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, note_on);

  auto note_off = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    note_off.push_back (_ev.time_, _ev.data ());
  }
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

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_on (1, 60, 80, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (1, 60, units::samples (200u));
    events2.push_back (_ev.time_, _ev.data ());
  }
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

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 0x80, 0xFF, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 0x80, units::samples (100u));
    events.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events);

  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 0x00)
    << "High bit of pitch byte should be masked";
  EXPECT_EQ (midi_note_creations_[0].velocity, 0x7F)
    << "High bit of velocity byte should be masked";
}

TEST_F (RecordingMaterializerTest, MidiContiguousPacketsShareSameClip)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_control_change (0, 1, 64, units::samples (150u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (midi_clip_create_count_, 1);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 64, 80, units::samples (0u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 64, units::samples (200u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (256), true, events2);

  EXPECT_EQ (midi_clip_create_count_, 1)
    << "Contiguous MIDI packets should share one clip";
  EXPECT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_control_event_creations_.size (), 1u);
}

TEST_F (RecordingMaterializerTest, MidiEmptyPacketsMaintainContiguity)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  EXPECT_EQ (midi_clip_create_count_, 1);

  auto empty_events = dsp::MidiEventBuffer::make_reserved ();
  write_midi_and_drain (track_id, units::samples (256), true, empty_events);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 62, 90, units::samples (0u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 62, units::samples (100u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (512), true, events2);

  EXPECT_EQ (midi_clip_create_count_, 1)
    << "Empty MIDI packets should not break contiguity";
  EXPECT_EQ (midi_note_creations_.size (), 2u);
}

TEST_F (RecordingMaterializerTest, MidiNotePositionsAreClipRelative)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  ASSERT_EQ (midi_note_creations_.size (), 1u);
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_note_creations_[0].end_position, units::samples (100));

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (10u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 64, 90, units::samples (50u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1000), true, events2);

  auto events3 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (100u));
    events3.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 64, units::samples (200u));
    events3.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1256), true, events3);

  ASSERT_EQ (midi_note_creations_.size (), 3u);
  EXPECT_EQ (midi_note_creations_[1].start_position, units::samples (10))
    << "First note in second clip starts at 1010 relative to clip start (1000)";
  EXPECT_EQ (midi_note_creations_[1].end_position, units::samples (356));
  EXPECT_EQ (midi_note_creations_[2].start_position, units::samples (50))
    << "Second note start (1050) relative to clip start (1000)";
  EXPECT_EQ (midi_note_creations_[2].end_position, units::samples (456))
    << "Second note end (1456) relative to clip start (1000)";
}

TEST_F (RecordingMaterializerTest, MidiCCPositionIsClipRelative)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (50u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  ASSERT_EQ (midi_clip_create_count_, 1);
  ASSERT_EQ (midi_control_event_creations_.size (), 0u);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_control_change (0, 1, 64, units::samples (50u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1000), true, events2);
  ASSERT_EQ (midi_clip_create_count_, 2);
  ASSERT_EQ (midi_control_event_creations_.size (), 1u);
  EXPECT_EQ (midi_control_event_creations_[0].position, units::samples (50))
    << "CC at 1050 relative to clip start (1000)";
}

TEST_F (RecordingMaterializerTest, MidiStaleNotesForceCompletedOnDiscontinuity)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 64, 80, units::samples (0u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 64, units::samples (100u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1000), true, events2);

  auto events3 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev = dsp::midi_event::make_note_off (0, 60, units::samples (0u));
    events3.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (1000 + 256), true, events3);

  ASSERT_EQ (midi_note_creations_.size (), 2u);
  EXPECT_EQ (midi_note_creations_[0].pitch, 60)
    << "Stale note from before discontinuity should be force-completed";
  EXPECT_EQ (midi_note_creations_[0].start_position, units::samples (0));
  EXPECT_EQ (midi_note_creations_[0].end_position, units::samples (256))
    << "Force-completed at last_end_position before discontinuity";
  EXPECT_EQ (midi_note_creations_[1].pitch, 64);
}

TEST_F (RecordingMaterializerTest, MidiClipCreatedOnFirstPacketEvenWithNoEvents)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto empty_events = dsp::MidiEventBuffer::make_reserved ();
  write_midi_and_drain (track_id, units::samples (0), true, empty_events);

  EXPECT_EQ (midi_clip_create_count_, 1)
    << "MIDI clip should be created on first packet, even with no events";
  EXPECT_EQ (last_midi_start_position_, units::samples (0));
  EXPECT_EQ (last_midi_track_id_, track_id);

  auto * clip =
    midi_clip_refs_[0].get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);
  EXPECT_DOUBLE_EQ (
    clip->length ()->ticks (),
    tempo_map_->samples_to_tick (units::samples (256.0)).asDouble ())
    << "Clip end should match packet end after first empty packet";

  auto events = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (256), true, events);

  EXPECT_EQ (midi_clip_create_count_, 1)
    << "Same clip should be reused for contiguous packets";
  EXPECT_DOUBLE_EQ (
    clip->length ()->ticks (),
    tempo_map_->samples_to_tick (units::samples (512.0)).asDouble ())
    << "Clip end should extend to cover second packet";
}

TEST_F (RecordingMaterializerTest, MidiClipExtendsWithEveryContiguousPacket)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);
  ASSERT_EQ (midi_clip_create_count_, 1);

  auto * clip =
    midi_clip_refs_[0].get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);
  EXPECT_DOUBLE_EQ (
    clip->length ()->ticks (),
    tempo_map_->samples_to_tick (units::samples (256.0)).asDouble ());

  auto empty = dsp::MidiEventBuffer::make_reserved ();
  write_midi_and_drain (track_id, units::samples (256), true, empty);
  EXPECT_DOUBLE_EQ (
    clip->length ()->ticks (),
    tempo_map_->samples_to_tick (units::samples (512.0)).asDouble ())
    << "Clip end should extend even for empty packets";

  write_midi_and_drain (track_id, units::samples (512), true, empty);
  EXPECT_DOUBLE_EQ (
    clip->length ()->ticks (),
    tempo_map_->samples_to_tick (units::samples (768.0)).asDouble ())
    << "Clip end should keep extending with contiguous empty packets";

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (50u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (768), true, events2);
  EXPECT_DOUBLE_EQ (
    clip->length ()->ticks (),
    tempo_map_->samples_to_tick (units::samples (1024.0)).asDouble ())
    << "Clip end should extend to cover note-off packet";
  EXPECT_EQ (midi_clip_create_count_, 1) << "Still one clip throughout";
}

TEST_F (
  RecordingMaterializerTest,
  MidiRepeatedNoteOnSamePitchCreatesSeparateNotesFIFO)
{
  create_materializer ();

  auto track_id = TrackUuid (QUuid::createUuid ());
  coordinator_->arm_track (track_id, units::samples (256), SessionType::Midi);

  auto events1 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 60, 80, units::samples (100u));
    events1.push_back (_ev.time_, _ev.data ());
  }
  write_midi_and_drain (track_id, units::samples (0), true, events1);

  auto events2 = dsp::MidiEventBuffer::make_reserved ();
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (50u));
    events2.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev =
      dsp::midi_event::make_note_off (0, 60, units::samples (150u));
    events2.push_back (_ev.time_, _ev.data ());
  }
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
  EXPECT_EQ (midi_clip_create_count_, 1);
}

}
