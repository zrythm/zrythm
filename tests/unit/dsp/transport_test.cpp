// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "dsp/transport.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class TransportTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100.0);

  void SetUp () override
  {
    // Setup Qt application for signal testing
    scoped_qt_app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    tempo_map_ = std::make_unique<TempoMap> (SAMPLE_RATE);

    // Setup config provider with default values
    config_provider_ = {
      .return_to_cue_on_pause_ = [] () { return false; },
      .metronome_countin_bars_ = [] () { return 0; },
      .recording_preroll_bars_ = [] () { return 0; }
    };

    transport_ = std::make_unique<Transport> (*tempo_map_, config_provider_);
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> scoped_qt_app_;
  std::unique_ptr<TempoMap>                             tempo_map_;
  std::unique_ptr<Transport>                            transport_;
  Transport::ConfigProvider                             config_provider_;
};

// Test initial state
TEST_F (TransportTest, InitialState)
{
  EXPECT_EQ (transport_->getPlayState (), Transport::PlayState::Paused);
  EXPECT_TRUE (transport_->loopEnabled ());
  EXPECT_FALSE (transport_->recordEnabled ());
  EXPECT_FALSE (transport_->punchEnabled ());
  EXPECT_FALSE (transport_->isRolling ());
  EXPECT_TRUE (transport_->isPaused ());

  // Test ITransport interface
  EXPECT_EQ (transport_->get_play_state (), Transport::PlayState::Paused);
  EXPECT_TRUE (transport_->loop_enabled ());
  EXPECT_FALSE (transport_->recording_enabled ());
  EXPECT_FALSE (transport_->punch_enabled ());

  // Test initial positions
  EXPECT_DOUBLE_EQ (
    transport_->get_playhead_position_in_audio_thread ().in (units::samples),
    0.0);

  auto [loop_start, loop_end] = transport_->get_loop_range_positions ();
  EXPECT_GE (loop_end.in (units::samples), loop_start.in (units::samples));

  auto [punch_in, punch_out] = transport_->get_punch_range_positions ();
  EXPECT_GE (punch_out.in (units::samples), punch_in.in (units::samples));
}

// Test QML property setters and getters
TEST_F (TransportTest, QMLProperties)
{
  testing::MockFunction<void ()> mock_handler;

  // Test loop enabled property
  QObject::connect (
    transport_.get (), &Transport::loopEnabledChanged, transport_.get (),
    mock_handler.AsStdFunction ());
  EXPECT_CALL (mock_handler, Call ()).Times (1);
  transport_->setLoopEnabled (false);
  EXPECT_FALSE (transport_->loopEnabled ());

  // Test record enabled property
  QObject::connect (
    transport_.get (), &Transport::recordEnabledChanged, transport_.get (),
    mock_handler.AsStdFunction ());
  EXPECT_CALL (mock_handler, Call ()).Times (1);
  transport_->setRecordEnabled (true);
  EXPECT_TRUE (transport_->recordEnabled ());

  // Test punch enabled property
  QObject::connect (
    transport_.get (), &Transport::punchEnabledChanged, transport_.get (),
    mock_handler.AsStdFunction ());
  EXPECT_CALL (mock_handler, Call ()).Times (1);
  transport_->setPunchEnabled (true);
  EXPECT_TRUE (transport_->punchEnabled ());
}

// Test play state changes
TEST_F (TransportTest, PlayStateChanges)
{
  testing::MockFunction<void (Transport::PlayState)> mock_handler;

  QObject::connect (
    transport_.get (), &Transport::playStateChanged, transport_.get (),
    mock_handler.AsStdFunction ());

  // Test setting play state directly
  EXPECT_CALL (mock_handler, Call (Transport::PlayState::Rolling)).Times (1);
  transport_->setPlayState (Transport::PlayState::Rolling);
  EXPECT_EQ (transport_->getPlayState (), Transport::PlayState::Rolling);
  EXPECT_TRUE (transport_->isRolling ());
  EXPECT_FALSE (transport_->isPaused ());

  EXPECT_CALL (mock_handler, Call (Transport::PlayState::Paused)).Times (1);
  transport_->setPlayState (Transport::PlayState::Paused);
  EXPECT_EQ (transport_->getPlayState (), Transport::PlayState::Paused);
  EXPECT_FALSE (transport_->isRolling ());
  EXPECT_TRUE (transport_->isPaused ());
}

// Test playhead movement
TEST_F (TransportTest, PlayheadMovement)
{
  const double test_ticks = 1920.0; // 2 beats at 120 BPM
  transport_->move_playhead (units::ticks (test_ticks), false);

  // Verify playhead position through adapter
  EXPECT_DOUBLE_EQ (transport_->playhead ()->ticks (), test_ticks);

  // Test that cue point is set when requested
  const double new_ticks = 3840.0; // 4 beats
  transport_->move_playhead (units::ticks (new_ticks), true);
  EXPECT_DOUBLE_EQ (transport_->playhead ()->ticks (), new_ticks);
  EXPECT_DOUBLE_EQ (transport_->cuePosition ()->ticks (), new_ticks);
}

// Test request pause
TEST_F (TransportTest, RequestPause)
{
  // Start rolling first
  transport_->setPlayState (Transport::PlayState::Rolling);
  const double initial_ticks = 3840.0;
  transport_->move_playhead (units::ticks (initial_ticks), true);

  // Request pause
  transport_->requestPause ();
  EXPECT_EQ (
    transport_->get_play_state (), Transport::PlayState::PauseRequested);

  // Check that playhead before pause is stored
  EXPECT_DOUBLE_EQ (
    transport_->playhead_ticks_before_pause ().in (units::ticks), initial_ticks);
}

// Test request roll with recording
TEST_F (TransportTest, RequestRollWithRecording)
{
  // Enable recording
  transport_->setRecordEnabled (true);

  // Move to a position and request roll
  transport_->move_playhead (units::ticks (4800.0), true); // 5 beats
  transport_->requestRoll ();

  EXPECT_EQ (transport_->getPlayState (), Transport::PlayState::RollRequested);
}

// Test punch range position checking
TEST_F (TransportTest, PunchRangePositionCheck)
{
  // Set punch range
  const double punch_in_ticks = 960.0;   // 1 beat
  const double punch_out_ticks = 2880.0; // 3 beats

  transport_->punchInPosition ()->setTicks (punch_in_ticks);
  transport_->punchOutPosition ()->setTicks (punch_out_ticks);

  // Convert to samples for testing
  const auto punch_in_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (punch_in_ticks));
  const auto punch_out_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (punch_out_ticks));

  // Test positions inside range
  EXPECT_TRUE (transport_->position_is_inside_punch_range (punch_in_samples));
  EXPECT_TRUE (transport_->position_is_inside_punch_range (
    units::samples (
      (punch_in_samples.in (units::samples)
       + punch_out_samples.in (units::samples))
      / 2)));

  // Test positions outside range
  EXPECT_FALSE (transport_->position_is_inside_punch_range (
    units::samples (punch_in_samples.in (units::samples) -1)));
  EXPECT_FALSE (transport_->position_is_inside_punch_range (
    units::samples (punch_out_samples.in (units::samples) + 1)));
}

// Test audio thread playhead advancement
TEST_F (TransportTest, AudioThreadPlayheadAdvancement)
{
  const auto initial_pos = transport_->get_playhead_position_in_audio_thread ();
  const auto nframes = units::samples (512);

  transport_->add_to_playhead_in_audio_thread (nframes);

  const auto new_pos = transport_->get_playhead_position_in_audio_thread ();
  EXPECT_DOUBLE_EQ (
    new_pos.in (units::samples),
    initial_pos.in (units::samples) + nframes.in (units::samples));
}
// Test real-time safe play state setting
TEST_F (TransportTest, RealTimeSafePlayStateSetting)
{
  // Set initial state
  transport_->setPlayState (Transport::PlayState::Paused);

  // Use RT-safe setter
  transport_->set_play_state_rt_safe (Transport::PlayState::Rolling);

  // State should be updated immediately
  EXPECT_EQ (transport_->get_play_state (), Transport::PlayState::Rolling);
}

// Test marker navigation
TEST_F (TransportTest, MarkerNavigation)
{
  // Setup some markers
  std::vector<units::precise_tick_t> extra_markers = {
    units::ticks (480.0),  // 0.5 beat
    units::ticks (1440.0), // 1.5 beats
    units::ticks (2400.0)  // 2.5 beats
  };

  // Move to middle position
  transport_->move_playhead (units::ticks (1200.0), true); // 1.25 beats

  // Test previous marker
  transport_->goto_prev_or_next_marker (true, extra_markers);
  EXPECT_DOUBLE_EQ (
    transport_->playhead ()->ticks (), 480.0); // Should go to 0.5 beats

  // Test next marker
  transport_->goto_prev_or_next_marker (false, extra_markers);
  EXPECT_DOUBLE_EQ (
    transport_->playhead ()->ticks (), 1440.0); // Should go to 1.5 beats
}
// Test metronome count-in consumption
TEST_F (TransportTest, MetronomeCountInConsumption)
{
  // Create a new transport with count-in config
  Transport::ConfigProvider countin_config = {
    .return_to_cue_on_pause_ = [] () { return false; },
    .metronome_countin_bars_ = [] () { return 1; },
    .recording_preroll_bars_ = [] () { return 0; }
  };

  auto countin_transport =
    std::make_unique<Transport> (*tempo_map_, countin_config);

  // Request roll to trigger count-in
  countin_transport->requestRoll ();

  // Should have count-in frames remaining
  const auto countin_frames =
    countin_transport->metronome_countin_frames_remaining ();
  EXPECT_GT (countin_frames.in (units::samples), 0);

  // Consume some frames
  const auto consume_frames = units::samples (200);
  countin_transport->consume_metronome_countin_samples (consume_frames);

  // Frames remaining should be reduced
  EXPECT_EQ (
    countin_transport->metronome_countin_frames_remaining ().in (units::samples),
    countin_frames.in (units::samples) -consume_frames.in (units::samples));
}

// Test recording preroll consumption
TEST_F (TransportTest, RecordingPrerollConsumption)
{
  // Create a new transport with preroll config
  Transport::ConfigProvider preroll_config = {
    .return_to_cue_on_pause_ = [] () { return false; },
    .metronome_countin_bars_ = [] () { return 0; },
    .recording_preroll_bars_ = [] () { return 2; }
  };

  auto preroll_transport =
    std::make_unique<Transport> (*tempo_map_, preroll_config);

  // Enable recording and request roll to trigger preroll
  preroll_transport->setRecordEnabled (true);
  preroll_transport->requestRoll ();

  // Should have preroll frames remaining
  const auto preroll_frames =
    preroll_transport->recording_preroll_frames_remaining ();
  EXPECT_GT (preroll_frames.in (units::samples), 0);

  // Consume some frames
  const auto consume_frames = units::samples (500);
  preroll_transport->consume_recording_preroll_samples (consume_frames);

  // Frames remaining should be reduced
  EXPECT_EQ (
    preroll_transport->recording_preroll_frames_remaining ().in (units::samples),
    preroll_frames.in (units::samples) -consume_frames.in (units::samples));
}
// Test serialization/deserialization
TEST_F (TransportTest, Serialization)
{
  // Set various properties
  transport_->setLoopEnabled (false);
  transport_->setRecordEnabled (true);
  transport_->setPunchEnabled (true);
  transport_->move_playhead (units::ticks (1920.0), true);
  transport_->loopStartPosition ()->setTicks (960.0);
  transport_->loopEndPosition ()->setTicks (2880.0);
  transport_->cuePosition ()->setTicks (480.0);

  // Serialize to JSON
  nlohmann::json j;
  j = *transport_;

  // Create new transport and deserialize
  Transport new_transport (*tempo_map_, config_provider_);
  j.get_to (new_transport);

  // Verify positions were restored
  EXPECT_DOUBLE_EQ (new_transport.playhead ()->ticks (), 1920.0);
  EXPECT_DOUBLE_EQ (new_transport.cuePosition ()->ticks (), 480.0);
  EXPECT_DOUBLE_EQ (new_transport.loopStartPosition ()->ticks (), 960.0);
  EXPECT_DOUBLE_EQ (new_transport.loopEndPosition ()->ticks (), 2880.0);
}

// Test property notification timer behavior
TEST_F (TransportTest, PropertyNotificationTimer)
{
  // This test verifies that the property notification timer is working
  // We can't directly test the timer behavior, but we can verify
  // that RT-safe state changes don't immediately emit signals

  testing::MockFunction<void ()> mock_handler;
  QObject::connect (
    transport_.get (), &Transport::playStateChanged, transport_.get (),
    mock_handler.AsStdFunction ());

  // Use RT-safe setter - should not immediately emit signal
  EXPECT_CALL (mock_handler, Call ()).Times (0);
  transport_->set_play_state_rt_safe (Transport::PlayState::Rolling);

  // State should be changed but signal not emitted yet
  EXPECT_EQ (transport_->get_play_state (), Transport::PlayState::Rolling);

  // Note: In a real test environment, we'd need to wait for the timer
  // to fire and then expect the signal. For unit tests, we verify the
  // interface behavior.
}

// Test loop end crossing in audio thread
TEST_F (TransportTest, LoopEndCrossing)
{
  // Setup loop range
  const double loop_start_ticks = 960.0; // 1 beat
  const double loop_end_ticks = 1920.0;  // 2 beats

  transport_->loopStartPosition ()->setTicks (loop_start_ticks);
  transport_->loopEndPosition ()->setTicks (loop_end_ticks);
  transport_->setLoopEnabled (true);

  // Move playhead near loop end
  transport_->move_playhead (units::ticks (1800.0), true);

  // Get loop end position for testing
  const auto loop_end_samples =
    tempo_map_->tick_to_samples_rounded (units::ticks (loop_end_ticks));

  // Test ITransport loop point detection
  const auto frames_to_loop_end = transport_->is_loop_point_met_in_audio_thread (
    units::samples (loop_end_samples.in (units::samples) -100),
    units::samples (200));

  EXPECT_GT (frames_to_loop_end.in (units::samples), 0);
  EXPECT_LT (frames_to_loop_end.in (units::samples), 200);
}

// Test edge cases
TEST_F (TransportTest, EdgeCases)
{
  // Test moving to negative position
  transport_->move_playhead (units::ticks (-100.0), false);
  EXPECT_GE (transport_->playhead ()->ticks (), 0.0);

  // Test very large position
  const auto large_ticks = units::ticks (1000000.0);
  transport_->move_playhead (large_ticks, false);
  EXPECT_DOUBLE_EQ (
    transport_->playhead ()->ticks (), large_ticks.in (units::ticks));
}

// Test recording modes enum
TEST_F (TransportTest, RecordingModesEnum)
{
  // Test that all recording modes are defined
  EXPECT_TRUE (std::is_enum_v<Transport::RecordingMode>);

  // Test enum values exist
  EXPECT_NO_THROW ({
    auto mode1 = Transport::RecordingMode::OverwriteEvents;
    auto mode2 = Transport::RecordingMode::MergeEvents;
    auto mode3 = Transport::RecordingMode::Takes;
    auto mode4 = Transport::RecordingMode::TakesMuted;
    (void) mode1;
    (void) mode2;
    (void) mode3;
    (void) mode4;
  });
}

// Test display enum
TEST_F (TransportTest, DisplayEnum)
{
  // Test that display enum is defined
  EXPECT_TRUE (std::is_enum_v<Transport::Display>);

  // Test enum values exist
  EXPECT_NO_THROW ({
    auto display1 = Transport::Display::BBT;
    auto display2 = Transport::Display::Time;
    (void) display1;
    (void) display2;
  });
}

// Test preroll enum conversion
TEST_F (TransportTest, PrerollEnumConversion)
{
  EXPECT_EQ (preroll_count_bars_enum_to_int (PrerollCountBars::PrerollNone), 0);
  EXPECT_EQ (preroll_count_bars_enum_to_int (PrerollCountBars::PrerollOne), 1);
  EXPECT_EQ (preroll_count_bars_enum_to_int (PrerollCountBars::PrerollTwo), 2);
  EXPECT_EQ (preroll_count_bars_enum_to_int (PrerollCountBars::PrerollFour), 4);
}

// Test TransportSnapshot creation and initial state
TEST_F (TransportTest, TransportSnapshotCreation)
{
  // Create a snapshot with known values
  auto loop_range =
    std::make_pair (units::samples (1000), units::samples (2000));
  auto punch_range =
    std::make_pair (units::samples (1500), units::samples (2500));
  auto playhead_pos = units::samples (1200);
  auto preroll_frames = units::samples (500);
  auto countin_frames = units::samples (300);
  auto play_state = Transport::PlayState::Rolling;
  bool loop_enabled = true;
  bool punch_enabled = false;
  bool recording_enabled = true;

  auto snapshot = Transport::TransportSnapshot{
    loop_range, punch_range,  playhead_pos,  preroll_frames,   countin_frames,
    play_state, loop_enabled, punch_enabled, recording_enabled
  };

  // Test all getters return expected values
  EXPECT_EQ (snapshot.get_loop_range_positions (), loop_range);
  EXPECT_EQ (snapshot.get_punch_range_positions (), punch_range);
  EXPECT_EQ (snapshot.get_playhead_position_in_audio_thread (), playhead_pos);
  EXPECT_EQ (snapshot.recording_preroll_frames_remaining (), preroll_frames);
  EXPECT_EQ (snapshot.metronome_countin_frames_remaining (), countin_frames);
  EXPECT_EQ (snapshot.get_play_state (), play_state);
  EXPECT_EQ (snapshot.loop_enabled (), loop_enabled);
  EXPECT_EQ (snapshot.punch_enabled (), punch_enabled);
  EXPECT_EQ (snapshot.recording_enabled (), recording_enabled);
}

// Test TransportSnapshot mutable operations
TEST_F (TransportTest, TransportSnapshotMutableOperations)
{
  auto snapshot = transport_->get_snapshot ();

  // Test play state modification
  EXPECT_EQ (snapshot.get_play_state (), Transport::PlayState::Paused);
  snapshot.set_play_state (Transport::PlayState::Rolling);
  EXPECT_EQ (snapshot.get_play_state (), Transport::PlayState::Rolling);

  // Test position modification
  auto original_pos = snapshot.get_playhead_position_in_audio_thread ();
  auto new_pos = units::samples (original_pos.in (units::samples) + 1000);
  snapshot.set_position (new_pos);
  EXPECT_EQ (snapshot.get_playhead_position_in_audio_thread (), new_pos);
}

// Test TransportSnapshot frame consumption
TEST_F (TransportTest, TransportSnapshotFrameConsumption)
{
  // Create snapshot with known frame counts
  auto preroll_frames = units::samples (1000);
  auto countin_frames = units::samples (500);

  auto snapshot = Transport::TransportSnapshot{
    std::make_pair (units::samples (0), units::samples (2000)),
    std::make_pair (units::samples (500), units::samples (1500)),
    units::samples (100),
    preroll_frames,
    countin_frames,
    Transport::PlayState::Rolling,
    true,
    false,
    true
  };

  // Test metronome count-in consumption
  auto consume_amount = units::samples (200);
  snapshot.consume_metronome_countin_samples (consume_amount);
  EXPECT_EQ (
    snapshot.metronome_countin_frames_remaining (),
    units::samples (
      countin_frames.in (units::samples) -consume_amount.in (units::samples)));

  // Test recording preroll consumption
  snapshot.consume_recording_preroll_samples (consume_amount);
  EXPECT_EQ (
    snapshot.recording_preroll_frames_remaining (),
    units::samples (
      preroll_frames.in (units::samples) -consume_amount.in (units::samples)));
}

// Test TransportSnapshot edge cases
TEST_F (TransportTest, TransportSnapshotEdgeCases)
{
  // Test with zero values
  auto zero_snapshot = Transport::TransportSnapshot{
    std::make_pair (units::samples (0), units::samples (0)),
    std::make_pair (units::samples (0), units::samples (0)),
    units::samples (0),
    units::samples (0),
    units::samples (0),
    Transport::PlayState::Paused,
    false,
    false,
    false
  };

  EXPECT_EQ (
    zero_snapshot.get_playhead_position_in_audio_thread (), units::samples (0));
  EXPECT_EQ (
    zero_snapshot.recording_preroll_frames_remaining (), units::samples (0));
  EXPECT_EQ (
    zero_snapshot.metronome_countin_frames_remaining (), units::samples (0));

  // Test consuming more frames than available (should not go negative in
  // practice)
  auto snapshot = Transport::TransportSnapshot{
    std::make_pair (units::samples (0), units::samples (2000)),
    std::make_pair (units::samples (500), units::samples (1500)),
    units::samples (100),
    units::samples (100),
    units::samples (50),
    Transport::PlayState::Rolling,
    true,
    false,
    true
  };

  // Consume exact amount available
  snapshot.consume_metronome_countin_samples (units::samples (50));
  EXPECT_EQ (snapshot.metronome_countin_frames_remaining (), units::samples (0));

  snapshot.consume_recording_preroll_samples (units::samples (100));
  EXPECT_EQ (snapshot.recording_preroll_frames_remaining (), units::samples (0));
}

// Test TransportSnapshot with large values
TEST_F (TransportTest, TransportSnapshotLargeValues)
{
  // Test with large sample values (simulating long projects)
  auto large_position = units::samples (
    static_cast<int64_t> (44100.0 * 60.0 * 10.0)); // 10 minutes at 44.1kHz
  auto large_preroll =
    units::samples (static_cast<int64_t> (44100.0 * 4.0)); // 4 seconds preroll

  auto snapshot = Transport::TransportSnapshot{
    std::make_pair (
      units::samples (0),
      units::samples (static_cast<int64_t> (44100.0 * 60.0 * 5.0))), // 5 minute
                                                                     // loop
    std::make_pair (
      units::samples (static_cast<int64_t> (44100.0 * 60.0)),
      units::samples (static_cast<int64_t> (44100.0 * 60.0 * 2.0))), // 1-2
                                                                     // minute
                                                                     // punch
    large_position, large_preroll,
    units::samples (static_cast<int64_t> (44100.0 * 2.0)), // 2 second count-in
    Transport::PlayState::Rolling, true, true, true
  };

  EXPECT_EQ (snapshot.get_playhead_position_in_audio_thread (), large_position);
  EXPECT_EQ (snapshot.recording_preroll_frames_remaining (), large_preroll);

  // Test consumption with large values
  auto consume_frames =
    units::samples (static_cast<int64_t> (44100.0)); // 1 second
  snapshot.consume_recording_preroll_samples (consume_frames);
  EXPECT_EQ (
    snapshot.recording_preroll_frames_remaining (),
    units::samples (
      large_preroll.in (units::samples) -consume_frames.in (units::samples)));
}

// Test TransportSnapshot from actual Transport
TEST_F (TransportTest, TransportSnapshotFromTransport)
{
  // Set up transport with specific state
  transport_->setLoopEnabled (true);
  transport_->setRecordEnabled (true);
  transport_->setPunchEnabled (false);
  transport_->setPlayState (Transport::PlayState::Rolling);
  transport_->move_playhead (units::ticks (4800.0), true); // 5 beats

  // Get snapshot
  auto snapshot = transport_->get_snapshot ();

  // Verify snapshot matches transport state
  EXPECT_EQ (snapshot.loop_enabled (), transport_->loop_enabled ());
  EXPECT_EQ (snapshot.recording_enabled (), transport_->recording_enabled ());
  EXPECT_EQ (snapshot.punch_enabled (), transport_->punch_enabled ());
  EXPECT_EQ (snapshot.get_play_state (), transport_->get_play_state ());
  EXPECT_EQ (
    snapshot.get_playhead_position_in_audio_thread (),
    transport_->get_playhead_position_in_audio_thread ());
  EXPECT_EQ (
    snapshot.get_loop_range_positions (),
    transport_->get_loop_range_positions ());
  EXPECT_EQ (
    snapshot.get_punch_range_positions (),
    transport_->get_punch_range_positions ());
}

// Test QML adapters are accessible
TEST_F (TransportTest, QMLAdapters)
{
  // Test that all QML adapters are accessible and not null
  EXPECT_NE (transport_->playhead (), nullptr);
  EXPECT_NE (transport_->cuePosition (), nullptr);
  EXPECT_NE (transport_->loopStartPosition (), nullptr);
  EXPECT_NE (transport_->loopEndPosition (), nullptr);
  EXPECT_NE (transport_->punchInPosition (), nullptr);
  EXPECT_NE (transport_->punchOutPosition (), nullptr);

  // Test that adapters have the expected interface
  EXPECT_NO_THROW ({
    double ticks = transport_->playhead ()->ticks ();
    transport_->playhead ()->setTicks (ticks + 1.0);
  });

  EXPECT_NO_THROW ({
    double ticks = transport_->cuePosition ()->ticks ();
    transport_->cuePosition ()->setTicks (ticks + 1.0);
  });
}

} // namespace zrythm::dsp
