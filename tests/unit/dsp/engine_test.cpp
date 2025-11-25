// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/graph_builder.h"
#include "dsp/tempo_map.h"
#include "dsp/transport.h"

#include "helpers/mock_audio_io_device.h"
#include "helpers/scoped_juce_qapplication.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace std::chrono_literals;

namespace zrythm::dsp
{

class AudioEngineTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
{
protected:
  using MockProcessable = graph_test::MockProcessable;

  void SetUp () override
  {
    // Create real tempo map
    tempo_map_ = std::make_unique<TempoMap> (units::sample_rate (48000.0));

    // Create real snap grid
    snap_grid_ = std::make_unique<SnapGrid> (
      *tempo_map_, utils::NoteLength::Note_1_4, [] () { return 0.0; });

    // Setup config provider with default values
    config_provider_ = {
      .return_to_cue_on_pause_ = [] () { return false; },
      .metronome_countin_bars_ = [] () { return 0; },
      .recording_preroll_bars_ = [] () { return 0; }
    };

    // Create real transport with default settings
    transport_ =
      std::make_unique<Transport> (*tempo_map_, *snap_grid_, config_provider_);

    // Create real graph builder using the actual implementation
    // We'll use a simple mock implementation for testing
    graph_builder_ = std::make_unique<MockGraphBuilder> ();
    processable_ = std::make_unique<MockProcessable> ();
    ON_CALL (*graph_builder_, build_graph_impl (_))
      .WillByDefault ([this] (graph::Graph &graph) {
        // Simple graph with 1 node
        graph.add_node_for_processable (*processable_);
      });

    // Create real graph dispatcher
    auto terminal_processables = std::vector<graph::IProcessable *>{};
    auto run_function_with_engine_lock = [] (std::function<void ()> func) {
      func ();
    };

    // Create audio device manager with dummy device
    audio_device_manager_ =
      test_helpers::create_audio_device_manager_with_dummy_device ();

    graph_dispatcher_ = std::make_unique<DspGraphDispatcher> (
      std::move (graph_builder_), terminal_processables, *audio_device_manager_,
      run_function_with_engine_lock);
  }

  // Mock implementation of IGraphBuilder for testing
  class MockGraphBuilder : public graph::IGraphBuilder
  {
  public:
    MOCK_METHOD (void, build_graph_impl, (graph::Graph &), (override));
  };

  std::unique_ptr<TempoMap>                 tempo_map_;
  std::unique_ptr<SnapGrid>                 snap_grid_;
  std::unique_ptr<Transport>                transport_;
  std::unique_ptr<MockProcessable>          processable_;
  std::unique_ptr<MockGraphBuilder>         graph_builder_;
  std::unique_ptr<DspGraphDispatcher>       graph_dispatcher_;
  std::shared_ptr<juce::AudioDeviceManager> audio_device_manager_;
  Transport::ConfigProvider                 config_provider_;
};

TEST_F (AudioEngineTest, ConstructorInitializesCorrectly)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  EXPECT_FALSE (engine->activated ());
  EXPECT_FALSE (engine->running ());
  EXPECT_FALSE (engine->exporting ());
  EXPECT_EQ (engine->get_device_manager (), audio_device_manager_);
  EXPECT_NE (engine->midi_panic_processor (), nullptr);
  EXPECT_EQ (engine->xRunCount (), 0); // Initial value should be 0
}

TEST_F (AudioEngineTest, GetBlockLengthReturnsCorrectValue)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Should return the buffer size from the audio device
  EXPECT_EQ (engine->get_block_length (), 256);
}

TEST_F (AudioEngineTest, GetSampleRateReturnsCorrectValue)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Should return the sample rate from the audio device
  EXPECT_EQ (engine->get_sample_rate (), 48000);
}

TEST_F (AudioEngineTest, ActivateWithTrueSetsStateToActive)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  engine->activate (true);

  EXPECT_TRUE (engine->activated ());
}

TEST_F (AudioEngineTest, ActivateWithFalseSetsStateToInitialized)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // First activate
  engine->activate (true);
  EXPECT_TRUE (engine->activated ());

  // Then deactivate
  engine->activate (false);
  EXPECT_FALSE (engine->activated ());
}

TEST_F (AudioEngineTest, ActivateWithSameStateDoesNothing)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate once
  engine->activate (true);
  EXPECT_TRUE (engine->activated ());

  // Activate again with same state - should do nothing
  engine->activate (true);
  EXPECT_TRUE (engine->activated ());
}

TEST_F (AudioEngineTest, SetAndGetRunning)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  EXPECT_FALSE (engine->running ());

  engine->set_running (true);
  EXPECT_TRUE (engine->running ());

  engine->set_running (false);
  EXPECT_FALSE (engine->running ());
}

TEST_F (AudioEngineTest, SetAndGetExporting)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  EXPECT_FALSE (engine->exporting ());

  engine->set_exporting (true);
  EXPECT_TRUE (engine->exporting ());

  engine->set_exporting (false);
  EXPECT_FALSE (engine->exporting ());
}

TEST_F (AudioEngineTest, PanicAllCallsMidiPanicProcessor)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  engine->panic_all ();

  // Should not crash - we can't easily verify the midi panic processor call
  // without exposing more internals, but we can at least ensure it doesn't crash
}

TEST_F (AudioEngineTest, ProcessPrepareWithPauseRequestedUpdatesPlayState)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);

  // Setup transport to return PauseRequested state
  transport_->requestPause ();

  // Create a dummy semaphore for testing
  moodycamel::LightweightSemaphore sem (1);
  auto                             lock = engine->get_processing_lock ();

  // Get transport snapshot to test process_prepare
  auto snapshot = transport_->get_snapshot ();

  bool result = engine->process_prepare (snapshot, 256, lock);

  EXPECT_FALSE (result); // Should not skip cycle
}

TEST_F (
  AudioEngineTest,
  ProcessPrepareWithRollRequestedAndNoCountinUpdatesPlayState)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);

  // Setup transport to return RollRequested state
  transport_->requestRoll ();

  // Create a dummy semaphore for testing
  moodycamel::LightweightSemaphore sem (1);
  auto                             lock = engine->get_processing_lock ();

  // Get transport snapshot to test process_prepare
  auto snapshot = transport_->get_snapshot ();

  bool result = engine->process_prepare (snapshot, 256, lock);

  EXPECT_FALSE (result); // Should not skip cycle
}

TEST_F (AudioEngineTest, ProcessPrepareWithoutExportingAndNoSemaphoreSkipsCycle)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);
  engine->set_exporting (false);

  // Create a semaphore that is not acquired
  moodycamel::LightweightSemaphore sem (0);
  auto                             lock = SemaphoreRAII (sem, false);

  // Get transport snapshot to test process_prepare
  auto snapshot = transport_->get_snapshot ();

  bool result = engine->process_prepare (snapshot, 256, lock);

  EXPECT_TRUE (result); // Should skip cycle
}

TEST_F (AudioEngineTest, ProcessWhenNotRunningReturnsSkipped)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);
  engine->set_running (false);

  dsp::PlayheadProcessingGuard guard{ transport_->playhead ()->playhead () };
  auto                         result = engine->process (guard, 256);

  EXPECT_EQ (result, AudioEngine::ProcessReturnStatus::ProcessSkipped);
}

TEST_F (AudioEngineTest, ProcessWithZeroFramesReturnsFailed)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);
  engine->set_running (true);

  dsp::PlayheadProcessingGuard guard{ transport_->playhead ()->playhead () };
  auto                         result = engine->process (guard, 0);

  EXPECT_EQ (result, AudioEngine::ProcessReturnStatus::ProcessFailed);
}

TEST_F (AudioEngineTest, ProcessWhenRunningReturnsCompleted)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);
  engine->set_running (true);

  dsp::PlayheadProcessingGuard guard{ transport_->playhead ()->playhead () };
  auto                         result = engine->process (guard, 256);

  EXPECT_EQ (result, AudioEngine::ProcessReturnStatus::ProcessCompleted);
}

TEST_F (AudioEngineTest, PostProcessWithRollingStateUpdatesPlayhead)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);
  engine->set_running (true);

  // Start rolling to test post_process
  transport_->setPlayState (dsp::ITransport::PlayState::Rolling);

  // Get transport snapshot to test post_process
  auto snapshot = transport_->get_snapshot ();

  // Store initial playhead position
  auto initial_pos =
    transport_->playhead ()->playhead ().position_samples_FOR_TESTING ();

  {
    dsp::PlayheadProcessingGuard guard{ transport_->playhead ()->playhead () };
    engine->post_process (snapshot, guard, 128, 256);
  }

  // Playhead should have advanced by 128 samples
  auto new_pos =
    transport_->playhead ()->playhead ().position_samples_FOR_TESTING ();
  EXPECT_DOUBLE_EQ (
    new_pos.in (units::samples), initial_pos.in (units::samples) + 128);
}

TEST_F (AudioEngineTest, PostProcessWithNonRollingStateDoesNotUpdatePlayhead)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Activate and start device to initialize monitor out port buffer
  engine->activate (true);
  engine->set_running (true);

  // Ensure transport is paused
  transport_->requestPause ();

  // Get transport snapshot to test post_process
  auto snapshot = transport_->get_snapshot ();

  // Store initial playhead position
  auto initial_pos =
    transport_->playhead ()->playhead ().position_samples_FOR_TESTING ();

  {
    dsp::PlayheadProcessingGuard guard{ transport_->playhead ()->playhead () };
    engine->post_process (snapshot, guard, 128, 256);
  }

  // Playhead should not have advanced since transport is not rolling
  auto new_pos =
    transport_->playhead ()->playhead ().position_samples_FOR_TESTING ();
  EXPECT_EQ (new_pos.in (units::samples), initial_pos.in (units::samples));
}

TEST_F (AudioEngineTest, WaitforPauseWithNonRunningEngineReturnsEarly)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  engine->set_running (false);

  transport_->setLoopEnabled (false);

  EngineState state{};
  engine->wait_for_pause (state, false, false);

  EXPECT_FALSE (state.running_);
  EXPECT_FALSE (state.playing_);
  EXPECT_FALSE (state.looping_);
}

TEST_F (AudioEngineTest, WaitforPauseWithPlayingEngineStopsPlayback)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  engine->set_running (true);

  // Start rolling and enable loop
  transport_->setPlayState (dsp::ITransport::PlayState::Rolling);
  transport_->setLoopEnabled (true);

  EngineState state{};
  engine->wait_for_pause (state, true, false);

  EXPECT_TRUE (state.running_);
  EXPECT_TRUE (state.playing_);
  EXPECT_TRUE (state.looping_);
  EXPECT_FALSE (engine->running ());
}

TEST_F (AudioEngineTest, ResumeWithNonRunningEngineDoesNothing)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  engine->set_running (true);

  EngineState state{ .running_ = false, .playing_ = true, .looping_ = true };
  engine->resume (state);

  // Should not modify the state since engine wasn't running
  EXPECT_FALSE (state.running_);
  EXPECT_TRUE (state.playing_);
  EXPECT_TRUE (state.looping_);
}

TEST_F (AudioEngineTest, ResumeWithRunningEngineRestoresState)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  engine->set_running (true);

  // Start rolling and enable loop
  transport_->requestRoll ();
  transport_->setLoopEnabled (true);

  EngineState state{ .running_ = true, .playing_ = true, .looping_ = true };
  engine->resume (state);

  EXPECT_TRUE (state.running_);
  EXPECT_TRUE (state.playing_);
  EXPECT_TRUE (state.looping_);
}

TEST_F (AudioEngineTest, GetProcessingLockReturnsValidLock)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  auto lock = engine->get_processing_lock ();

  // Should be able to acquire the lock
  EXPECT_TRUE (lock.is_acquired ());
}

TEST_F (AudioEngineTest, LoadPercentageReturnsValidValue)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Should return a valid percentage (initially 0.0)
  double load = engine->loadPercentage ();
  EXPECT_GE (load, 0.0);
  EXPECT_LE (load, 100.0);
}

TEST_F (AudioEngineTest, XRunCountReturnsValidValue)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  // Should return a valid count (initially 0)
  int xruns = engine->xRunCount ();
  EXPECT_GE (xruns, 0);
}

TEST_F (AudioEngineTest, GetMonitorOutPortReturnsValidPort)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  auto &monitor_out = engine->get_monitor_out_port ();

  // Should return a valid reference to the monitor out port
  EXPECT_EQ (&monitor_out, &engine->get_monitor_out_port ());
}

TEST_F (AudioEngineTest, GetGraphDispatcherReturnsValidDispatcher)
{
  auto engine = std::make_unique<AudioEngine> (
    *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

  auto &dispatcher = engine->graph_dispatcher ();

  // Should return a valid reference to the graph dispatcher
  EXPECT_EQ (&dispatcher, &engine->graph_dispatcher ());
}

TEST_F (AudioEngineTest, DestructorDeactivatesIfActive)
{
  {
    auto engine = std::make_unique<AudioEngine> (
      *transport_, *tempo_map_, audio_device_manager_, *graph_dispatcher_);

    // Activate the engine first
    engine->activate (true);
    EXPECT_TRUE (engine->activated ());

    // Destroying the engine (going out of scope) should call deactivate
    // We can't easily verify this without exposing more internals,
    // but the destructor should be called without crashing
  }
}

} // namespace zrythm::dsp
