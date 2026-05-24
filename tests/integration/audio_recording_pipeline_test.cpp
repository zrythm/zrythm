// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for the audio recording pipeline.
 *
 * Verifies that audio data flows from the engine through TrackProcessor's
 * recording callback into the RecordingCoordinator's sessions, through
 * the RecordingMaterializer to create undoable audio regions, and that
 * arm/disarm lifecycle works correctly with the engine running.
 */

#include <algorithm>

#include "controllers/audio_recording_session.h"
#include "controllers/recording_coordinator.h"
#include "controllers/recording_materializer.h"
#include "dsp/audio_input_processor.h"
#include "dsp/graph.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/project/project_ui_state.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/track.h"

#include <QSignalSpy>

#include "helpers/project_fixture.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm
{

class AudioRecordingPipelineTest : public test_helpers::ProjectTestFixture
{
protected:
  void SetUp () override
  {
    ProjectTestFixture::SetUp ();

    coordinator_ =
      utils::make_qobject_unique<controllers::RecordingCoordinator> ();

    project_ = create_minimal_project (
      [this] (
        const structure::tracks::Track::Uuid &track_id,
        units::sample_t timeline_position, const dsp::ITransport &transport,
        const dsp::MidiEventVector * midi_events,
        std::optional<structure::tracks::TrackProcessor::ConstStereoPortPair>
          stereo_ports) {
        auto * session = coordinator_->session_for_track (track_id);
        if (session == nullptr)
          return;
        if (stereo_ports.has_value ())
          {
            session->write_samples (
              timeline_position, transport.recording_enabled (),
              stereo_ports->first, stereo_ports->second);
          }
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

  units::sample_u32_t block_length () const
  {
    const auto bl = project_->engine ()->block_length ();
    assert (bl > units::samples (0u));
    return bl;
  }

  void set_provider ()
  {
    project_->set_audio_input_selection_provider (
      [this] (const structure::tracks::Track::Uuid &uuid)
        -> dsp::AudioInputSelection * {
        return ui_state_->find_audio_input_selection (uuid);
      });
  }

  void start_engine_and_wait_for_cycles (size_t min_cycles = 2)
  {
    initial_process_count_ = mock_hw_->process_call_count ();
    project_->engine ()->activate ();
    project_->engine ()->graph_dispatcher ().recalc_graph (false);
    project_->engine ()->set_running (true);
    project_->getTransport ()->setPlayState (
      dsp::ITransport::PlayState::RollRequested);

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

  std::optional<structure::arrangement::ArrangerObjectUuidReference>
  create_recording_region (
    structure::tracks::TrackUuid     track_id,
    units::sample_t                  start_position,
    const utils::audio::AudioBuffer &initial_frames)
  {
    auto &registry = project_->get_registry ();
    auto &tempo_map = project_->tempo_map ();

    auto clip_ref = utils::create_object<dsp::FileAudioSource> (
      registry, initial_frames, utils::audio::BitDepth::BIT_DEPTH_32,
      project_->engine ()->sample_rate (), 120.f, u8"RecordingClip");

    auto source_obj_ref =
      utils::create_object<structure::arrangement::AudioSourceObject> (
        registry, tempo_map, registry, clip_ref);

    auto region_ref = utils::create_object<structure::arrangement::AudioRegion> (
      registry, tempo_map, registry, [] () { return true; });

    auto * region =
      region_ref.get_object_as<structure::arrangement::AudioRegion> ();
    region->set_source (source_obj_ref);

    source_obj_refs_.push_back (std::move (source_obj_ref));
    region_refs_.push_back (region_ref);

    return region_ref;
  }

  void create_materializer ()
  {
    undo_stack_ = zrythm::actions::create_mock_undo_stack ();

    materializer_ = std::make_unique<controllers::RecordingMaterializer> (
      *coordinator_, *undo_stack_,
      [this] (
        structure::tracks::TrackUuid track_id, units::sample_t start_position,
        const utils::audio::AudioBuffer &initial_frames, size_t lane_index)
        -> controllers::RecordingMaterializer::RegionCreationResult {
        auto region_ref_opt =
          create_recording_region (track_id, start_position, initial_frames);
        if (!region_ref_opt.has_value ())
          return std::nullopt;
        return controllers::RecordingMaterializer::CreatedRegion{
          std::move (*region_ref_opt), lane_index
        };
      },
      [] () { return controllers::recording::RecordingMode::Takes; });
  }

  /**
   * @brief Drains pending audio from a session, waiting until at least one
   * packet is available.
   *
   * Repeatedly processes Qt events and drains the session's SPSC fifo until
   * audio data has been received from the recording thread. Returns all
   * accumulated packets.
   */
  std::vector<controllers::RecordingAudioPacket>
  collect_recorded_packets (controllers::AudioRecordingSession * session)
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

  /**
   * @brief Drains the coordinator's pending data repeatedly until a condition
   * is met.
   *
   * Calls process_pending() inside process_events_until_true, giving the
   * materializer a chance to create/expand regions after each drain. The
   * predicate is checked after each drain cycle.
   */
  void drain_until (std::function<bool ()> predicate)
  {
    process_events_until_true ([this, pred = std::move (predicate)] () {
      coordinator_->process_pending ();
      return pred ();
    });
  }

  dsp::FileAudioSource * get_first_clip () const
  {
    if (region_refs_.empty ())
      return nullptr;
    auto * region =
      region_refs_.front ().get_object_as<structure::arrangement::AudioRegion> ();
    auto children = region->get_children_view ();
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

  std::vector<structure::arrangement::ArrangerObjectUuidReference> region_refs_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
    source_obj_refs_;
};

// ============================================================================
// Capture-level tests (no materializer needed)
// ============================================================================

TEST_F (AudioRecordingPipelineTest, ArmTrackWritesToSession)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles ();

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  auto packets = collect_recorded_packets (session);

  stop_engine ();

  EXPECT_TRUE (coordinator_->has_session (track->get_uuid ()));
  EXPECT_FALSE (packets.empty ())
    << "Recording session should have received audio data after processing";
}

TEST_F (AudioRecordingPipelineTest, DisarmStopsWriting)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  coordinator_->arm_track (track->get_uuid (), units::samples (256u));

  start_engine_and_wait_for_cycles ();

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  auto packets_before = collect_recorded_packets (session);
  EXPECT_FALSE (packets_before.empty ())
    << "Session should have recorded audio data before disarming";

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_FALSE (coordinator_->has_session (track->get_uuid ()));
}

TEST_F (AudioRecordingPipelineTest, AudioInputThroughRecordingPipeline)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  coordinator_->arm_track (track->get_uuid (), units::samples (256u));

  start_engine_and_wait_for_cycles ();

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  auto packets = collect_recorded_packets (session);

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

TEST_F (AudioRecordingPipelineTest, BlockLengthIncreaseDuringRecording)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles ();

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  auto pre_packets = collect_recorded_packets (session);
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

  auto * session_after = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session_after, nullptr) << "Session should survive a device change";
  EXPECT_EQ (
    session_after->state (),
    controllers::AudioRecordingSession::State::Capturing)
    << "Session should remain in Capturing state after device change";

  auto post_packets = collect_recorded_packets (session_after);
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

TEST_F (AudioRecordingPipelineTest, BlockLengthShrinkDuringRecording)
{
  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles ();

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  auto pre_packets = collect_recorded_packets (session);
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

  auto * session_after = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session_after, nullptr);
  EXPECT_EQ (
    session_after->state (),
    controllers::AudioRecordingSession::State::Capturing)
    << "Session should remain in Capturing state after block length shrink";

  auto post_packets = collect_recorded_packets (session_after);
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
// Full pipeline tests (materializer → region → undo)
// ============================================================================

TEST_F (AudioRecordingPipelineTest, MaterializerCreatesRegionFromEngineAudio)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (4);

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  ASSERT_FALSE (region_refs_.empty ())
    << "Materializer should have created at least one region";

  auto * clip = get_first_clip ();
  ASSERT_NE (clip, nullptr);
  EXPECT_GT (clip->get_num_frames (), 0)
    << "Recorded clip should contain audio frames";

  auto * region =
    region_refs_.front ().get_object_as<structure::arrangement::AudioRegion> ();
  EXPECT_GT (region->bounds ()->length ()->ticks (), 0.0)
    << "Region length should be positive";
}

TEST_F (AudioRecordingPipelineTest, UndoMacroWrapsRecording)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (4);

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
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

TEST_F (AudioRecordingPipelineTest, ContinuousRecordingExpandsSingleRegion)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (6);

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());

  drain_until ([this] {
    auto * clip = get_first_clip ();
    return clip != nullptr && clip->get_num_frames () > 256;
  });

  ASSERT_FALSE (region_refs_.empty ());
  // Continuous recording should produce exactly one region (no
  // discontinuities)
  EXPECT_EQ (region_refs_.size (), 1u)
    << "Continuous recording should create a single region";

  auto * clip = get_first_clip ();
  ASSERT_NE (clip, nullptr);
  EXPECT_GT (clip->get_num_frames (), 256)
    << "Multiple engine cycles should expand the clip beyond one block";
}

TEST_F (AudioRecordingPipelineTest, MultiTrackRecordingCreatesRegionsForAll)
{
  create_materializer ();

  auto * track_a = add_audio_track ();
  ASSERT_NE (track_a, nullptr);
  auto * track_b = add_audio_track ();
  ASSERT_NE (track_b, nullptr);

  set_audio_input_selection (*track_a, u"Test Device"_s, 0, true);
  set_audio_input_selection (*track_b, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track_a->get_uuid (), block_length ());
  coordinator_->arm_track (track_b->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (4);

  auto * session_a = coordinator_->session_for_track (track_a->get_uuid ());
  ASSERT_NE (session_a, nullptr);
  auto * session_b = coordinator_->session_for_track (track_b->get_uuid ());
  ASSERT_NE (session_b, nullptr);

  process_events_until_true ([session_a] () {
    return session_a->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });
  process_events_until_true ([session_b] () {
    return session_b->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (track_a->get_uuid ());
  coordinator_->disarm_track (track_b->get_uuid ());
  drain_until ([this] { return region_refs_.size () >= 2; });

  EXPECT_GE (region_refs_.size (), 2u)
    << "Both tracks should have created at least one region each";
  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Single macro should wrap both tracks' recording";
  EXPECT_EQ (undo_stack_->count (), 1)
    << "All multi-track recording should be in one undo macro";
}

TEST_F (AudioRecordingPipelineTest, TransportRecordToggleFinalizesMacro)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (4);

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
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
  EXPECT_FALSE (region_refs_.empty ())
    << "At least one region should have been created";
}

TEST_F (AudioRecordingPipelineTest, TransportStopStartResumesRecording)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (4);

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();

  ASSERT_FALSE (region_refs_.empty ()) << "First pass should create regions";
  const auto regions_after_first = region_refs_.size ();

  stop_engine ();
  coordinator_->finalizeAllSessions ();

  EXPECT_TRUE (undo_stack_->canUndo ())
    << "Macro should be finalized after transport stop";

  project_->getTransport ()->setRecordEnabled (true);

  start_engine_and_wait_for_cycles (4);

  auto * session_after = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session_after, nullptr)
    << "Session should be reused after transport restart";

  process_events_until_true ([session_after] () {
    return session_after->state ()
           == controllers::AudioRecordingSession::State::Capturing;
  });

  coordinator_->process_pending ();
  stop_engine ();

  coordinator_->disarm_track (track->get_uuid ());
  coordinator_->process_pending ();

  EXPECT_GT (region_refs_.size (), regions_after_first)
    << "Second pass should create additional regions";
}

TEST_F (AudioRecordingPipelineTest, RecordedAudioDataIntegrity)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (4);

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
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

  ASSERT_FALSE (region_refs_.empty ());

  auto * clip = get_first_clip ();
  ASSERT_NE (clip, nullptr);

  const auto num_frames = clip->get_num_frames ();
  EXPECT_GT (num_frames, 0) << "Clip should contain audio frames";
  EXPECT_GE (num_frames, static_cast<int> (expected_min_frames))
    << "Clip should contain at least 2 engine cycles worth of frames";

  EXPECT_EQ (clip->get_num_channels (), 2)
    << "Stereo recording should produce a 2-channel clip";

  auto * region =
    region_refs_.front ().get_object_as<structure::arrangement::AudioRegion> ();
  const auto region_length_samples = region->bounds ()->length ()->samples ();
  EXPECT_NEAR (region_length_samples, static_cast<double> (num_frames), 1.0)
    << "Region length in samples should match clip frame count";
}

TEST_F (AudioRecordingPipelineTest, UndoMacroCountAndRedoAfterUndo)
{
  create_materializer ();

  auto * track = add_audio_track ();
  ASSERT_NE (track, nullptr);
  set_audio_input_selection (*track, u"Test Device"_s, 0, true);
  set_provider ();

  project_->getTransport ()->setRecordEnabled (true);

  coordinator_->arm_track (track->get_uuid (), block_length ());

  start_engine_and_wait_for_cycles (4);

  auto * session = coordinator_->session_for_track (track->get_uuid ());
  ASSERT_NE (session, nullptr);

  process_events_until_true ([session] () {
    return session->state ()
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

} // namespace zrythm
