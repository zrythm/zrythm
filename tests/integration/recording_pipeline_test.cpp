// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for the recording pipeline (audio + MIDI).
 *
 * Verifies that audio and MIDI data flows from the engine through
 * TrackProcessor's recording callback into the RecordingCoordinator's
 * sessions, through the RecordingMaterializer to create undoable clips,
 * and that arm/disarm lifecycle works correctly with the engine running.
 */

#include <algorithm>

#include "controllers/recording_coordinator.h"
#include "controllers/recording_materializer.h"
#include "controllers/recording_session.h"
#include "dsp/audio_input_processor.h"
#include "dsp/graph.h"
#include "dsp/midi_device_buffer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/project/project_ui_state.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/midi_track.h"
#include "structure/tracks/track.h"

#include <QSignalSpy>

#include "helpers/project_fixture.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm
{

using SessionType = controllers::RecordingCoordinator::SessionType;

class RecordingPipelineTest : public test_helpers::ProjectTestFixture
{
protected:
  inline static const utils::Utf8String kMidiDeviceIdentifier{
    u8"test-midi-device"
  };
  void SetUp () override
  {
    ProjectTestFixture::SetUp ();

    coordinator_ =
      utils::make_qobject_unique<controllers::RecordingCoordinator> ();

    project_ = create_minimal_project (
      [this] (
        const structure::tracks::Track::Uuid &track_id,
        units::sample_t timeline_position, const dsp::ITransport &transport,
        const dsp::MidiEventBuffer * midi_events,
        std::optional<structure::tracks::TrackProcessor::ConstStereoPortPair>
                            stereo_ports,
        units::sample_u32_t nframes) {
        auto session = coordinator_->session_for_track (track_id);
        std::visit (
          utils::overload{
            [] (std::monostate) { },
            [&] (controllers::AudioRecordingSession * s) {
              if (stereo_ports.has_value ())
                {
                  s->write (
                    timeline_position, transport.recording_enabled (),
                    stereo_ports->first, stereo_ports->second);
                }
            },
            [&] (controllers::MidiRecordingSession * s) {
              if (midi_events != nullptr)
                {
                  s->write (
                    timeline_position, transport.recording_enabled (),
                    *midi_events, nframes);
                }
            },
          },
          session);
      });

    project_->add_default_tracks ();

    ui_state_ = utils::make_qobject_unique<structure::project::ProjectUiState> (
      *project_, *app_settings_);

    mock_hw_ = dynamic_cast<test_helpers::ThreadedMockHardwareAudioInterface *> (
      hw_interface_.get ());
    ASSERT_NE (mock_hw_, nullptr);

    auto * coordinator_raw = coordinator_.get ();
    auto * engine_raw = project_->engine ();
    QObject::connect (
      engine_raw, &dsp::AudioEngine::blockLengthChanged, coordinator_raw,
      [coordinator_raw] (int block_length) {
        coordinator_raw->prepare_for_processing (units::samples (block_length));
      });
  }

  void TearDown () override { ProjectTestFixture::TearDown (); }

  // === Audio track helpers ===

  structure::tracks::AudioTrack * add_audio_track ()
  {
    auto track_ref = project_->track_factory_->create_empty_track<
      structure::tracks::AudioTrack> ();
    auto * raw = track_ref.get_object_as<structure::tracks::AudioTrack> ();
    raw->setName (u8"Audio Track");
    raw->get_track_processor ()->set_recording_armed (true);
    project_->tracklist ()->collection ()->add_track (track_ref);
    return raw;
  }

  void set_audio_input_selection (
    const structure::tracks::Track &track,
    const QString                  &device_name,
    int                             first_channel,
    bool                            stereo)
  {
    auto * sel = ui_state_->audioInputSelectionForTrack (&track);
    sel->setDeviceName (device_name);
    sel->setFirstChannel (first_channel);
    sel->setStereo (stereo);
  }

  // === MIDI track helpers ===

  structure::tracks::MidiTrack * add_midi_track ()
  {
    auto track_ref = project_->track_factory_->create_empty_track<
      structure::tracks::MidiTrack> ();
    auto * raw = track_ref.get_object_as<structure::tracks::MidiTrack> ();
    raw->setName (u8"MIDI Track");
    raw->get_track_processor ()->set_recording_armed (true);
    project_->tracklist ()->collection ()->add_track (track_ref);
    return raw;
  }

  void set_midi_input_selection (
    const structure::tracks::Track &track,
    const QString                  &device_identifier,
    int                             midi_channel)
  {
    auto * sel = ui_state_->midiInputSelectionForTrack (&track);
    sel->setDeviceIdentifier (device_identifier);
    sel->setMidiChannel (midi_channel);
  }

  // === Shared helpers ===

  units::sample_u32_t block_length () const
  {
    const auto bl = project_->engine ()->block_length ();
    assert (bl > units::samples (0u));
    return bl;
  }

  void set_audio_provider ()
  {
    project_->set_audio_input_selection_provider (
      [this] (const structure::tracks::Track::Uuid &uuid)
        -> dsp::AudioInputSelection * {
        return ui_state_->find_audio_input_selection (uuid);
      });
  }

  void set_midi_provider ()
  {
    project_->set_midi_input_selection_provider (
      [this] (const structure::tracks::Track::Uuid &uuid)
        -> dsp::MidiInputSelection * {
        return ui_state_->find_midi_input_selection (uuid);
      });
  }

  void start_engine_and_wait_for_cycles (size_t min_cycles = 2)
  {
    project_->engine ()->activate ();
    project_->engine ()->graph_dispatcher ().recalc_graph (false);
    project_->engine ()->set_running (true);
    project_->getTransport ()->setPlayState (
      dsp::ITransport::PlayState::RollRequested);

    initial_process_count_ = mock_hw_->process_call_count ();

    process_events_until_true ([this, min_cycles] () {
      return mock_hw_->process_call_count ()
             >= initial_process_count_ + min_cycles;
    });
  }

  void start_engine_with_midi_device (
    const utils::Utf8String               &device_identifier,
    std::shared_ptr<dsp::MidiDeviceBuffer> buffer,
    size_t                                 min_cycles = 2)
  {
    project_->engine ()->activate ();

    midi_interface_.simulate_device_change (
      {
        { device_identifier, std::move (buffer) }
    });

    project_->engine ()->graph_dispatcher ().recalc_graph (false);
    project_->engine ()->set_running (true);
    project_->getTransport ()->setPlayState (
      dsp::ITransport::PlayState::RollRequested);

    initial_process_count_ = mock_hw_->process_call_count ();

    process_events_until_true ([this, min_cycles] () {
      return mock_hw_->process_call_count ()
             >= initial_process_count_ + min_cycles;
    });
  }

  void stop_engine ()
  {
    project_->engine ()->set_running (false);
    project_->engine ()->deactivate ();
  }

  void create_materializer ()
  {
    undo_stack_ = zrythm::actions::create_mock_undo_stack ();

    materializer_ = std::make_unique<controllers::RecordingMaterializer> (
      *coordinator_, *undo_stack_,
      controllers::RecordingMaterializer::ArrangerObjectCreators{
        .audio_clip =
          [this] (
            structure::tracks::TrackUuid track_id, units::sample_t start_position,
            const utils::audio::AudioBuffer &initial_frames, size_t lane_index)
          -> controllers::RecordingMaterializer::ClipCreationResult {
          auto clip_ref_opt = create_audio_recording_clip (
            track_id, start_position, initial_frames);
          if (!clip_ref_opt.has_value ())
            return std::nullopt;
          return controllers::RecordingMaterializer::CreatedClip{
            std::move (*clip_ref_opt), lane_index
          };
        },
        .midi_clip =
          [this] (
            structure::tracks::TrackUuid track_id,
            units::sample_t start_position, size_t lane_index)
          -> controllers::RecordingMaterializer::ClipCreationResult {
          midi_clip_create_count_++;
          auto clip_ref = utils::create_object<structure::arrangement::MidiClip> (
            project_->get_registry (), *project_->getTempoMap (),
            project_->get_registry ());
          clip_ref.get ()->position ()->setTicks (
            project_->getTempoMap ()
              ->get_tempo_map ()
              .samples_to_tick (
                units::samples (start_position.in (units::samples)))
              .asDouble ());
          midi_clip_refs_.push_back (clip_ref);
          return controllers::RecordingMaterializer::CreatedClip{
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
      [] () { return controllers::recording::RecordingMode::Takes; });
  }

  std::vector<controllers::RecordingAudioPacket>
  collect_audio_packets (controllers::AudioRecordingSession * session)
  {
    std::vector<controllers::RecordingAudioPacket> packets;
    process_events_until_true ([session, &packets] () {
      auto drained = session->drain_pending ();
      packets.insert (
        packets.end (), std::make_move_iterator (drained.begin ()),
        std::make_move_iterator (drained.end ()));
      return !packets.empty ();
    });
    return packets;
  }

  std::vector<controllers::RecordingMidiPacket>
  collect_midi_packets (controllers::MidiRecordingSession * session)
  {
    std::vector<controllers::RecordingMidiPacket> packets;
    process_events_until_true ([session, &packets] () {
      auto drained = session->drain_pending ();
      packets.insert (
        packets.end (), std::make_move_iterator (drained.begin ()),
        std::make_move_iterator (drained.end ()));
      return !packets.empty ();
    });
    return packets;
  }

  void drain_until (std::function<bool ()> predicate)
  {
    process_events_until_true ([this, pred = std::move (predicate)] () {
      coordinator_->process_pending ();
      return pred ();
    });
  }

  dsp::FileAudioSource * get_first_clip () const
  {
    if (audio_clip_refs_.empty ())
      return nullptr;
    auto * clip =
      audio_clip_refs_.front ()
        .get_object_as<structure::arrangement::AudioClip> ();
    auto children = clip->get_children_view ();
    if (children.empty ())
      return nullptr;
    auto * source_obj = children.front ();
    auto   clip_ref = source_obj->audio_source_ref ();
    return clip_ref.get_object_as<dsp::FileAudioSource> ();
  }

  test_helpers::ThreadedMockHardwareAudioInterface * mock_hw_{};
  size_t initial_process_count_{ 0 };

  utils::QObjectUniquePtr<controllers::RecordingCoordinator>  coordinator_;
  std::unique_ptr<structure::project::Project>                project_;
  utils::QObjectUniquePtr<structure::project::ProjectUiState> ui_state_;
  std::unique_ptr<undo::UndoStack>                            undo_stack_;
  std::unique_ptr<controllers::RecordingMaterializer>         materializer_;

  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    audio_clip_refs_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    source_obj_refs_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    midi_clip_refs_;

  int midi_clip_create_count_ = 0;

  struct MidiNoteCreation
  {
    units::sample_t                    start_position;
    units::sample_t                    end_position;
    int                                pitch;
    int                                velocity;
    int                                channel;
    structure::arrangement::MidiClip * clip;
  };
  std::vector<MidiNoteCreation> midi_note_creations_;

  struct MidiControlEventCreation
  {
    units::sample_t                                     position;
    structure::arrangement::MidiControlEvent::EventType type;
    int                                                 channel;
    int                                                 controller;
    int                                                 value;
    structure::arrangement::MidiClip *                  clip;
  };
  std::vector<MidiControlEventCreation> midi_control_event_creations_;

private:
  std::optional<structure::arrangement::ArrangerObjectUuidReference>
  create_audio_recording_clip (
    structure::tracks::TrackUuid     track_id,
    units::sample_t                  start_position,
    const utils::audio::AudioBuffer &initial_frames)
  {
    auto  &registry = project_->get_registry ();
    auto * tempo_map = project_->getTempoMap ();

    auto audio_source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, initial_frames, utils::audio::BitDepth::BIT_DEPTH_32,
      project_->engine ()->sample_rate (), units::bpm (120.0),
      u8"RecordingClip");

    auto source_obj_ref =
      utils::create_object<structure::arrangement::AudioSourceObject> (
        registry, *tempo_map, registry, audio_source_ref);

    auto clip_ref = utils::create_object<structure::arrangement::AudioClip> (
      registry, *tempo_map, registry);

    auto * clip = clip_ref.get_object_as<structure::arrangement::AudioClip> ();
    clip->set_source (source_obj_ref);

    source_obj_refs_.push_back (std::move (source_obj_ref));
    audio_clip_refs_.push_back (clip_ref);

    return clip_ref;
  }
};

// ============================================================================
// Audio capture-level tests
// ============================================================================

TEST_F (RecordingPipelineTest, AudioArmTrackWritesToSession)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles ();

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  auto packets = collect_audio_packets (
    std::get<controllers::AudioRecordingSession *> (session));

  stop_engine ();

  EXPECT_TRUE (coordinator_->has_session (track->get_uuid ()));
  EXPECT_FALSE (packets.empty ())
    << "Recording session should have received audio data after processing";
}

TEST_F (RecordingPipelineTest, AudioDisarmStopsWriting)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  coordinator_->arm_track (
    track->get_uuid (), units::samples (256u), SessionType::Audio);

  start_engine_and_wait_for_cycles ();

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  auto packets_before = collect_audio_packets (
    std::get<controllers::AudioRecordingSession *> (session));
  EXPECT_FALSE (packets_before.empty ())
    << "Session should have recorded audio data before disarming";

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_FALSE (coordinator_->has_session (track->get_uuid ()));
}

TEST_F (RecordingPipelineTest, AudioInputThroughRecordingPipeline)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  coordinator_->arm_track (
    track->get_uuid (), units::samples (256u), SessionType::Audio);

  start_engine_and_wait_for_cycles ();

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  auto packets = collect_audio_packets (
    std::get<controllers::AudioRecordingSession *> (session));

  stop_engine ();

  ASSERT_FALSE (packets.empty ());

  for (const auto &packet : packets)
    {
      EXPECT_EQ (
        packet.l_frames.size (), packet.nframes.in<size_t> (units::samples))
        << "Left channel frame count should match nframes";
      EXPECT_EQ (
        packet.r_frames.size (), packet.nframes.in<size_t> (units::samples))
        << "Right channel frame count should match nframes";
    }

  size_t total_frames = 0;
  for (const auto &packet : packets)
    {
      total_frames += packet.nframes.in<size_t> (units::samples);
    }
  EXPECT_GT (total_frames, 0u) << "Should have recorded some audio frames";
}

TEST_F (RecordingPipelineTest, AudioBlockLengthIncreaseDuringRecording)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles ();

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  auto pre_packets = collect_audio_packets (
    std::get<controllers::AudioRecordingSession *> (session));
  ASSERT_FALSE (pre_packets.empty ());

  dsp::AudioDeviceInfo new_device_info{
    .device_name = u8"Test Device 2",
    .sample_rate = units::sample_rate (48000),
    .block_length = units::samples (512),
    .input_channel_count = units::channels (4),
    .output_channel_count = units::channels (2),
  };
  QSignalSpy block_length_spy (
    project_->engine (), &dsp::AudioEngine::blockLengthChanged);
  const auto count_before_change = mock_hw_->process_call_count ();
  mock_hw_->simulate_device_change (new_device_info);

  ASSERT_EQ (block_length_spy.size (), 1);
  EXPECT_EQ (block_length_spy.takeFirst ().at (0).toInt (), 512);

  process_events_until_true ([this, count_before_change] () {
    return mock_hw_->process_call_count () >= count_before_change + 5;
  });

  auto session_after = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session_after))
    << "Session should survive a device change";
  EXPECT_EQ (
    std::get<controllers::AudioRecordingSession *> (session_after)->state (),
    controllers::AudioRecordingSession::State::Capturing)
    << "Session should remain in Capturing state after device change";

  auto post_packets = collect_audio_packets (
    std::get<controllers::AudioRecordingSession *> (session_after));
  EXPECT_FALSE (post_packets.empty ())
    << "Session should continue recording after device change";

  bool has_new_block_length =
    std::ranges::any_of (post_packets, [] (const auto &packet) {
      return packet.nframes == units::samples (512u);
    });
  EXPECT_TRUE (has_new_block_length)
    << "At least one post-change packet should use the new block length";

  stop_engine ();
}

TEST_F (RecordingPipelineTest, AudioBlockLengthShrinkDuringRecording)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles ();

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  auto pre_packets = collect_audio_packets (
    std::get<controllers::AudioRecordingSession *> (session));
  ASSERT_FALSE (pre_packets.empty ());

  dsp::AudioDeviceInfo new_device_info{
    .device_name = u8"Test Device Small",
    .sample_rate = units::sample_rate (48000),
    .block_length = units::samples (64),
    .input_channel_count = units::channels (2),
    .output_channel_count = units::channels (2),
  };
  QSignalSpy block_length_spy (
    project_->engine (), &dsp::AudioEngine::blockLengthChanged);
  const auto count_before_change = mock_hw_->process_call_count ();
  mock_hw_->simulate_device_change (new_device_info);

  ASSERT_EQ (block_length_spy.size (), 1);
  EXPECT_EQ (block_length_spy.takeFirst ().at (0).toInt (), 64);

  process_events_until_true ([this, count_before_change] () {
    return mock_hw_->process_call_count () >= count_before_change + 5;
  });

  auto session_after = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session_after));
  EXPECT_EQ (
    std::get<controllers::AudioRecordingSession *> (session_after)->state (),
    controllers::AudioRecordingSession::State::Capturing)
    << "Session should remain in Capturing state after block length shrink";

  auto post_packets = collect_audio_packets (
    std::get<controllers::AudioRecordingSession *> (session_after));
  EXPECT_FALSE (post_packets.empty ())
    << "Session should continue recording after block length shrink";

  bool has_shrunk_block_length =
    std::ranges::any_of (post_packets, [] (const auto &packet) {
      return packet.nframes == units::samples (64u);
    });
  EXPECT_TRUE (has_shrunk_block_length)
    << "At least one post-shrink packet should use the new block length";

  stop_engine ();
}

// ============================================================================
// Audio full pipeline tests (materializer → clip → undo)
// ============================================================================

TEST_F (RecordingPipelineTest, AudioMaterializerCreatesClipFromEngineAudio)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  ASSERT_FALSE (audio_clip_refs_.empty ())
    << "Materializer should have created at least one clip";

  auto * clip = get_first_clip ();
  ASSERT_NE (clip, nullptr);
  EXPECT_GT (clip->get_num_frames (), 0)
    << "Recorded clip should contain audio frames";

  auto * audio_clip =
    audio_clip_refs_.front ().get_object_as<structure::arrangement::AudioClip> ();
  EXPECT_GT (audio_clip->length ()->ticks (), 0.0)
    << "Clip length should be positive";
}

TEST_F (RecordingPipelineTest, AudioUndoMacroWrapsRecording)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Recording should be undoable via the undo stack macro";
  EXPECT_EQ (undo_stack_->count (), 1)
    << "Undo stack should contain a single recording macro";

  ASSERT_TRUE (undo_stack_->canUndo ());
  undo_stack_->undo ();

  EXPECT_TRUE (undo_stack_->canRedo ())
    << "Undoing the recording should allow redo";
}

TEST_F (RecordingPipelineTest, AudioContinuousRecordingExpandsSingleClip)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (6);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());

  drain_until ([this] {
    auto * clip = get_first_clip ();
    return clip != nullptr && clip->get_num_frames () > 256;
  });

  ASSERT_FALSE (audio_clip_refs_.empty ());
  EXPECT_EQ (audio_clip_refs_.size (), 1u)
    << "Continuous recording should create a single clip";

  auto * clip = get_first_clip ();
  ASSERT_NE (clip, nullptr);
  EXPECT_GT (clip->get_num_frames (), 256)
    << "Multiple engine cycles should expand the clip beyond one block";
}

TEST_F (RecordingPipelineTest, AudioMultiTrackRecordingCreatesClipsForAll)
{
  create_materializer ();

  auto * track_a = add_audio_track ();
  ASSERT_NE (track_a, nullptr);
  auto * track_b = add_audio_track ();
  ASSERT_NE (track_b, nullptr);

  set_audio_input_selection (*track_a, u"Test Device"_s, 0, true);
  set_audio_input_selection (*track_b, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track_a->get_uuid (), block_length (), SessionType::Audio);
  coordinator_->arm_track (
    track_b->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (4);

  auto session_a = coordinator_->session_for_track (track_a->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session_a));
  auto session_b = coordinator_->session_for_track (track_b->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session_b));

  process_events_until_true ([session_a] () {
    return std::get<controllers::AudioRecordingSession *> (session_a)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });
  process_events_until_true ([session_b] () {
    return std::get<controllers::AudioRecordingSession *> (session_b)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (track_a->get_uuid ());
  coordinator_->disarm_track (track_b->get_uuid ());
  drain_until ([this] { return audio_clip_refs_.size () >= 2; });

  EXPECT_GE (audio_clip_refs_.size (), 2u)
    << "Both tracks should have created at least one clip each";
  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Single macro should wrap both tracks' recording";
  EXPECT_EQ (undo_stack_->count (), 1)
    << "All multi-track recording should be in one undo macro";
}

TEST_F (RecordingPipelineTest, AudioTransportRecordToggleFinalizesMacro)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  project_->getTransport ()->setRecordEnabled (false);

  start_engine_and_wait_for_cycles (2);

  coordinator_->process_pending ();

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized after transport record toggle";
  EXPECT_FALSE (audio_clip_refs_.empty ())
    << "At least one clip should have been created";
}

TEST_F (RecordingPipelineTest, AudioTransportStopStartResumesRecording)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  ASSERT_FALSE (audio_clip_refs_.empty ()) << "First pass should create clips";
  const auto clips_after_first = audio_clip_refs_.size ();

  stop_engine ();
  coordinator_->finalizeAllSessions ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized after transport stop";

  project_->getTransport ()->setRecordEnabled (true);

  start_engine_and_wait_for_cycles (4);

  auto session_after = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session_after))
    << "Session should be reused after transport restart";

  process_events_until_true ([session_after] () {
    return std::get<controllers::AudioRecordingSession *> (session_after)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_GT (audio_clip_refs_.size (), clips_after_first)
    << "Second pass should create additional clips";
}

TEST_F (RecordingPipelineTest, AudioRecordedAudioDataIntegrity)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());

  const auto expected_min_frames =
    static_cast<size_t> (project_->engine ()->blockLength ()) * 2;
  drain_until ([this, expected_min_frames] {
    auto * clip = get_first_clip ();
    return clip != nullptr
           && clip->get_num_frames () >= static_cast<int> (expected_min_frames);
  });

  ASSERT_FALSE (audio_clip_refs_.empty ());

  auto * clip = get_first_clip ();
  ASSERT_NE (clip, nullptr);

  const auto num_frames = clip->get_num_frames ();
  EXPECT_GT (num_frames, 0) << "Clip should contain audio frames";
  EXPECT_GE (num_frames, static_cast<int> (expected_min_frames))
    << "Clip should contain at least 2 engine cycles worth of frames";

  EXPECT_EQ (clip->get_num_channels (), 2)
    << "Stereo recording should produce a 2-channel clip";

  auto * audio_clip =
    audio_clip_refs_.front ().get_object_as<structure::arrangement::AudioClip> ();
  const auto clip_length_samples =
    audio_clip->get_tempo_map ()
      .tick_to_samples_rounded (
        timeline_end_ticks (*audio_clip) - audio_clip->position ()->asTick ())
      .in (units::samples);
  EXPECT_NEAR (clip_length_samples, static_cast<double> (num_frames), 1.0)
    << "Clip length in samples should match clip frame count";
}

TEST_F (RecordingPipelineTest, AudioUndoMacroCountAndRedoAfterUndo)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_audio_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Audio);

  start_engine_and_wait_for_cycles (4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::AudioRecordingSession *> (session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  ASSERT_TRUE (undo_stack_->canUndo ());
  EXPECT_EQ (undo_stack_->count (), 1)
    << "Undo stack should contain exactly one recording macro";

  undo_stack_->undo ();

  EXPECT_TRUE (undo_stack_->canRedo ())
    << "After undo, redo should be available";
  EXPECT_FALSE (undo_stack_->canUndo ())
    << "After undoing the only macro, nothing should be undoable";

  undo_stack_->redo ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "After redo, undo should be available again";
  EXPECT_FALSE (undo_stack_->canRedo ())
    << "After redoing the only macro, nothing should be redoable";
}

// ============================================================================
// MIDI capture-level tests
// ============================================================================

TEST_F (RecordingPipelineTest, MidiArmTrackWritesToSession)
{
  auto * track = add_midi_track ();
  ASSERT_NE (track, nullptr);
  set_midi_input_selection (*track, kMidiDeviceIdentifier, 0);
  set_midi_provider ();

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Midi);

  auto midi_buffer = std::make_shared<dsp::MidiDeviceBuffer> ();
  midi_buffer->push (juce::MidiMessage::noteOn (1, 60, 0.8f));

  start_engine_with_midi_device (kMidiDeviceIdentifier, midi_buffer);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::MidiRecordingSession *> (session));

  auto packets = collect_midi_packets (
    std::get<controllers::MidiRecordingSession *> (session));

  stop_engine ();

  EXPECT_TRUE (coordinator_->has_session (track->get_uuid ()));
  EXPECT_FALSE (packets.empty ())
    << "MIDI recording session should have received packets after processing";
}

TEST_F (RecordingPipelineTest, MidiDisarmStopsWriting)
{
  auto * track = add_midi_track ();
  ASSERT_NE (track, nullptr);
  set_midi_input_selection (*track, kMidiDeviceIdentifier, 0);
  set_midi_provider ();

  coordinator_->arm_track (
    track->get_uuid (), units::samples (256u), SessionType::Midi);

  auto midi_buffer = std::make_shared<dsp::MidiDeviceBuffer> ();
  midi_buffer->push (juce::MidiMessage::noteOn (1, 60, 0.8f));

  start_engine_with_midi_device (kMidiDeviceIdentifier, midi_buffer);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::MidiRecordingSession *> (session));

  auto packets_before = collect_midi_packets (
    std::get<controllers::MidiRecordingSession *> (session));
  EXPECT_FALSE (packets_before.empty ())
    << "Session should have recorded MIDI data before disarming";

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_FALSE (coordinator_->has_session (track->get_uuid ()));
}

// ============================================================================
// MIDI full pipeline tests (materializer → clip → notes → undo)
// ============================================================================

TEST_F (RecordingPipelineTest, MidiMaterializerCreatesClipWithNotes)
{
  create_materializer ();

  auto * track = add_midi_track ();
  ASSERT_NE (track, nullptr);
  set_midi_input_selection (*track, kMidiDeviceIdentifier, 0);
  set_midi_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Midi);

  auto midi_buffer = std::make_shared<dsp::MidiDeviceBuffer> ();
  midi_buffer->push (juce::MidiMessage::noteOn (1, 60, 0.8f));
  midi_buffer->push (juce::MidiMessage::noteOff (1, 60, 0.0f));

  start_engine_with_midi_device (kMidiDeviceIdentifier, midi_buffer, 4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::MidiRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::MidiRecordingSession *> (session)->state ()
           == controllers::MidiRecordingSession::State::Capturing;
  });

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_GE (midi_clip_create_count_, 1)
    << "Materializer should have created at least one MIDI clip";
}

TEST_F (RecordingPipelineTest, MidiContinuousRecordingExpandsClip)
{
  create_materializer ();

  auto * track = add_midi_track ();
  ASSERT_NE (track, nullptr);
  set_midi_input_selection (*track, kMidiDeviceIdentifier, 0);
  set_midi_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Midi);

  auto midi_buffer = std::make_shared<dsp::MidiDeviceBuffer> ();

  start_engine_with_midi_device (kMidiDeviceIdentifier, midi_buffer, 6);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::MidiRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::MidiRecordingSession *> (session)->state ()
           == controllers::MidiRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());

  drain_until ([this] { return midi_clip_create_count_ >= 1; });

  ASSERT_EQ (midi_clip_refs_.size (), 1u)
    << "Continuous recording should create a single MIDI clip";

  auto * clip =
    midi_clip_refs_.front ().get_object_as<structure::arrangement::MidiClip> ();
  ASSERT_NE (clip, nullptr);
  EXPECT_GT (clip->length ()->ticks (), 0.0)
    << "Clip length should grow with continuous recording";
}

TEST_F (RecordingPipelineTest, MidiNoteOnWithoutOffForceCompletedOnDisarm)
{
  create_materializer ();

  auto * track = add_midi_track ();
  ASSERT_NE (track, nullptr);
  set_midi_input_selection (*track, kMidiDeviceIdentifier, 0);
  set_midi_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Midi);

  auto midi_buffer = std::make_shared<dsp::MidiDeviceBuffer> ();
  midi_buffer->push (juce::MidiMessage::noteOn (1, 60, 0.8f));

  start_engine_with_midi_device (kMidiDeviceIdentifier, midi_buffer, 4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::MidiRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::MidiRecordingSession *> (session)->state ()
           == controllers::MidiRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Disarming should finalize the recording macro even with stuck notes";
  EXPECT_GE (midi_clip_create_count_, 1) << "MIDI clip should have been created";
}

TEST_F (RecordingPipelineTest, MidiUndoMacroCountAndRedoAfterUndo)
{
  create_materializer ();

  auto * track = add_midi_track ();
  ASSERT_NE (track, nullptr);
  set_midi_input_selection (*track, kMidiDeviceIdentifier, 0);
  set_midi_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    track->get_uuid (), block_length (), SessionType::Midi);

  auto midi_buffer = std::make_shared<dsp::MidiDeviceBuffer> ();
  midi_buffer->push (juce::MidiMessage::noteOn (1, 60, 0.8f));
  midi_buffer->push (juce::MidiMessage::noteOff (1, 60, 0.0f));

  start_engine_with_midi_device (kMidiDeviceIdentifier, midi_buffer, 4);

  auto session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::MidiRecordingSession *> (session));

  process_events_until_true ([session] () {
    return std::get<controllers::MidiRecordingSession *> (session)->state ()
           == controllers::MidiRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  ASSERT_TRUE (undo_stack_->canUndo ());
  EXPECT_EQ (undo_stack_->count (), 1)
    << "Undo stack should contain exactly one MIDI recording macro";

  undo_stack_->undo ();

  EXPECT_TRUE (undo_stack_->canRedo ())
    << "After undo, redo should be available";

  undo_stack_->redo ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "After redo, undo should be available again";
}

// ============================================================================
// Mixed audio + MIDI tests
// ============================================================================

TEST_F (RecordingPipelineTest, MixedAudioAndMidiRecording)
{
  create_materializer ();

  auto * audio_track = add_audio_track ();
  ASSERT_NE (audio_track, nullptr);
  auto * midi_track = add_midi_track ();
  ASSERT_NE (midi_track, nullptr);

  set_audio_input_selection (*audio_track, u"Test Device"_s, 0, true);
  set_audio_provider ();
  set_midi_input_selection (*midi_track, kMidiDeviceIdentifier, 0);
  set_midi_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (
    audio_track->get_uuid (), block_length (), SessionType::Audio);
  coordinator_->arm_track (
    midi_track->get_uuid (), block_length (), SessionType::Midi);

  auto midi_buffer = std::make_shared<dsp::MidiDeviceBuffer> ();
  midi_buffer->push (juce::MidiMessage::noteOn (1, 64, 0.7f));
  midi_buffer->push (juce::MidiMessage::noteOff (1, 64, 0.0f));

  start_engine_with_midi_device (kMidiDeviceIdentifier, midi_buffer, 4);

  auto audio_session =
    coordinator_->session_for_track (audio_track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::AudioRecordingSession *> (audio_session));

  auto midi_session = coordinator_->session_for_track (midi_track->get_uuid ());
  ASSERT_TRUE (
    std::holds_alternative<controllers::MidiRecordingSession *> (midi_session));

  process_events_until_true ([audio_session] () {
    return std::get<controllers::AudioRecordingSession *> (audio_session)->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });
  process_events_until_true ([midi_session] () {
    return std::get<controllers::MidiRecordingSession *> (midi_session)->state ()
           == controllers::MidiRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (audio_track->get_uuid ());
  coordinator_->disarm_track (midi_track->get_uuid ());
  drain_until ([this] {
    return !audio_clip_refs_.empty () && midi_clip_create_count_ >= 1;
  });

  EXPECT_FALSE (audio_clip_refs_.empty ())
    << "Audio track should have created a clip";
  EXPECT_GE (midi_clip_create_count_, 1)
    << "MIDI track should have created a clip";
  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Single macro should wrap both audio and MIDI recording";
  EXPECT_EQ (undo_stack_->count (), 1)
    << "Mixed recording should be in one undo macro";
}

} // namespace zrythm
