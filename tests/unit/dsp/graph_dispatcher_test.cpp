// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_builder.h"
#include "dsp/graph_dispatcher.h"
#include "dsp/juce_hardware_audio_interface.h"

#include "helpers/mock_audio_io_device.h"
#include "helpers/scoped_juce_qapplication.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace std::chrono_literals;

namespace zrythm::dsp
{

// Mock implementation of IGraphBuilder for testing
class MockGraphBuilder : public graph::IGraphBuilder
{
public:
  MOCK_METHOD (void, build_graph_impl, (graph::Graph &), (override));
};

class DspGraphDispatcherTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
{
protected:
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;
  using MockTransport = zrythm::dsp::graph_test::MockTransport;

  void SetUp () override
  {
    // Create mock processables
    for (const auto _ : std::views::iota (0, 3))
      {
        processables_.emplace_back (std::make_unique<MockProcessable> ());
        ON_CALL (*processables_.back (), get_node_name ())
          .WillByDefault (Return (u8"test_node"));
        ON_CALL (*processables_.back (), get_single_playback_latency ())
          .WillByDefault (Return (0));
        ON_CALL (
          *processables_.back (),
          prepare_for_processing (
            An<const graph::GraphNode *> (), An<units::sample_rate_t> (), _))
          .WillByDefault (Return ());
        ON_CALL (*processables_.back (), release_resources ())
          .WillByDefault (Return ());
      }

    // Create mock transport
    transport_ = std::make_unique<MockTransport> ();
    ON_CALL (*transport_, get_play_state ())
      .WillByDefault (Return (ITransport::PlayState::Paused));
    ON_CALL (*transport_, get_playhead_position_in_audio_thread ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (*transport_, loop_enabled ()).WillByDefault (Return (false));
    ON_CALL (*transport_, punch_enabled ()).WillByDefault (Return (false));
    ON_CALL (*transport_, recording_enabled ()).WillByDefault (Return (false));
    ON_CALL (*transport_, recording_preroll_frames_remaining ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (*transport_, metronome_countin_frames_remaining ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (*transport_, get_loop_range_positions ())
      .WillByDefault (
        Return (std::make_pair (units::samples (0), units::samples (48000))));
    ON_CALL (*transport_, get_punch_range_positions ())
      .WillByDefault (
        Return (std::make_pair (units::samples (0), units::samples (48000))));

    // Create mock graph builder
    mock_graph_builder_ = std::make_unique<MockGraphBuilder> ();

    // Setup terminal processables (last processable in chain)
    terminal_processables_.push_back (processables_.back ().get ());

    // Setup mock run function with engine lock
    run_function_with_engine_lock_ = [] (std::function<void ()> func) {
      func ();
    };

    // Create audio device manager with dummy device
    audio_device_manager_ =
      test_helpers::create_audio_device_manager_with_dummy_device ();

    // Create hardware audio interface wrapper
    hw_interface_ =
      dsp::JuceHardwareAudioInterface::create (audio_device_manager_);

    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (48000));
  }

  void create_dispatcher ()
  {
    dispatcher_ = std::make_unique<DspGraphDispatcher> (
      std::move (mock_graph_builder_), terminal_processables_, *hw_interface_,
      run_function_with_engine_lock_,
      [] (std::function<void ()> func) { func (); });
  }

  std::vector<std::unique_ptr<MockProcessable>> processables_;
  std::unique_ptr<MockTransport>                transport_;
  std::unique_ptr<MockGraphBuilder>             mock_graph_builder_;
  std::vector<graph::IProcessable *>            terminal_processables_;
  DspGraphDispatcher::RunFunctionWithEngineLock run_function_with_engine_lock_;
  std::shared_ptr<juce::AudioDeviceManager>     audio_device_manager_;
  std::unique_ptr<dsp::IHardwareAudioInterface> hw_interface_;
  std::unique_ptr<DspGraphDispatcher>           dispatcher_;
  std::unique_ptr<dsp::TempoMap>                tempo_map_;
};

TEST_F (DspGraphDispatcherTest, ConstructorInitializesCorrectly)
{
  create_dispatcher ();
  EXPECT_NE (dispatcher_, nullptr);
  EXPECT_FALSE (dispatcher_->is_processing_kickoff_thread ());
  EXPECT_FALSE (dispatcher_->is_processing_thread ());
}

TEST_F (
  DspGraphDispatcherTest,
  GetMaxRoutePlaybackLatencyReturnsZeroWhenNoScheduler)
{
  create_dispatcher ();
  EXPECT_EQ (dispatcher_->get_max_route_playback_latency (), 0);
}

TEST_F (DspGraphDispatcherTest, RecalcGraphWithSoftFalse)
{
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (1)
    .WillOnce ([this] (graph::Graph &graph) {
      // Create a simple chain: node1 -> node2 -> node3
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  create_dispatcher ();

  // Expect prepare_for_processing to be called for each node
  EXPECT_CALL (
    *processables_[0],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);
  EXPECT_CALL (
    *processables_[1],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);
  EXPECT_CALL (
    *processables_[2],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);

  // Adjust playback latency for a node
  ON_CALL (*processables_[0], get_single_playback_latency ())
    .WillByDefault (Return (128));

  dispatcher_->recalc_graph (false);

  // Should have non-zero latency after graph is built
  EXPECT_GT (dispatcher_->get_max_route_playback_latency (), 0);
}

TEST_F (DspGraphDispatcherTest, RecalcGraphWithSoftTrue)
{
  // First do a full recalculation
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (1)
    .WillOnce ([this] (graph::Graph &graph) {
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  create_dispatcher ();
  dispatcher_->recalc_graph (false);

  // Now do a soft recalculation (should not call build_graph_impl)
  dispatcher_->recalc_graph (true);
}

TEST_F (DspGraphDispatcherTest, StartCycleInRealtimeContext)
{
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (1)
    .WillOnce ([this] (graph::Graph &graph) {
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  create_dispatcher ();
  dispatcher_->recalc_graph (false);

  // Expect process_block to be called for each node
  EXPECT_CALL (*processables_[0], process_block (_, _, _)).Times (1);
  EXPECT_CALL (*processables_[1], process_block (_, _, _)).Times (1);
  EXPECT_CALL (*processables_[2], process_block (_, _, _)).Times (1);

  EngineProcessTimeInfo time_info{};
  time_info.g_start_frame_ = 0;
  time_info.g_start_frame_w_offset_ = 0;
  time_info.local_offset_ = 0;
  time_info.nframes_ = 256;

  dispatcher_->start_cycle (*transport_, time_info, 0, true, *tempo_map_);

  // Should be marked as processing kickoff thread
  EXPECT_TRUE (dispatcher_->is_processing_kickoff_thread ());
}

TEST_F (DspGraphDispatcherTest, StartCycleInNonRealtimeContext)
{
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (1)
    .WillOnce ([this] (graph::Graph &graph) {
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  create_dispatcher ();
  dispatcher_->recalc_graph (false);

  // Expect process_block to be called for each node
  EXPECT_CALL (*processables_[0], process_block (_, _, _)).Times (1);
  EXPECT_CALL (*processables_[1], process_block (_, _, _)).Times (1);
  EXPECT_CALL (*processables_[2], process_block (_, _, _)).Times (1);

  EngineProcessTimeInfo time_info{};
  time_info.g_start_frame_ = 0;
  time_info.g_start_frame_w_offset_ = 0;
  time_info.local_offset_ = 0;
  time_info.nframes_ = 256;

  dispatcher_->start_cycle (*transport_, time_info, 0, false, *tempo_map_);

  // Should NOT be marked as processing kickoff thread
  EXPECT_FALSE (dispatcher_->is_processing_kickoff_thread ());
}

TEST_F (DspGraphDispatcherTest, StartCycleWithLatencyPreroll)
{
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (1)
    .WillOnce ([this] (graph::Graph &graph) {
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  // Set up latency expectations
  ON_CALL (*processables_[0], get_single_playback_latency ())
    .WillByDefault (Return (128));
  ON_CALL (*processables_[1], get_single_playback_latency ())
    .WillByDefault (Return (64));
  ON_CALL (*processables_[2], get_single_playback_latency ())
    .WillByDefault (Return (32));

  create_dispatcher ();
  dispatcher_->recalc_graph (false);

  EXPECT_CALL (*processables_[0], process_block (_, _, _)).Times (1);
  EXPECT_CALL (*processables_[1], process_block (_, _, _)).Times (1);
  // Latency preroll means this will not be processed in this cycle
  // TODO: I haven't done the calculations yet to see if this is correct
  EXPECT_CALL (*processables_[2], process_block (_, _, _)).Times (0);

  EngineProcessTimeInfo time_info{};
  time_info.g_start_frame_ = 0;
  time_info.g_start_frame_w_offset_ = 0;
  time_info.local_offset_ = 0;
  time_info.nframes_ = 256;

  const nframes_t remaining_latency_preroll = 64;
  dispatcher_->start_cycle (
    *transport_, time_info, remaining_latency_preroll, true, *tempo_map_);
}

TEST_F (DspGraphDispatcherTest, CurrentTriggerNodesAccess)
{
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (1)
    .WillOnce ([this] (graph::Graph &graph) {
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  create_dispatcher ();
  dispatcher_->recalc_graph (false);

  // Should be able to access current trigger nodes
  auto &trigger_nodes = dispatcher_->current_trigger_nodes ();
  EXPECT_GT (trigger_nodes.size (), 0);
}

TEST_F (DspGraphDispatcherTest, ProcessingThreadDetection)
{
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (1)
    .WillOnce ([this] (graph::Graph &graph) {
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  create_dispatcher ();
  dispatcher_->recalc_graph (false);

  // Initially should not be a processing thread
  EXPECT_FALSE (dispatcher_->is_processing_thread ());

  EXPECT_CALL (*processables_[0], process_block (_, _, _)).Times (1);
  EXPECT_CALL (*processables_[1], process_block (_, _, _)).Times (1);
  EXPECT_CALL (*processables_[2], process_block (_, _, _)).Times (1);

  EngineProcessTimeInfo time_info{};
  time_info.g_start_frame_ = 0;
  time_info.g_start_frame_w_offset_ = 0;
  time_info.local_offset_ = 0;
  time_info.nframes_ = 256;

  // After starting a cycle, main thread might be detected as processing thread
  // depending on implementation, but this test ensures the method doesn't crash
  dispatcher_->start_cycle (*transport_, time_info, 0, true, *tempo_map_);

  // Just ensure the method can be called without issues
  [[maybe_unused]] bool is_processing = dispatcher_->is_processing_thread ();
}

TEST_F (DspGraphDispatcherTest, MultipleRecalcGraphCalls)
{
  EXPECT_CALL (*mock_graph_builder_, build_graph_impl (_))
    .Times (2)
    .WillRepeatedly ([this] (graph::Graph &graph) {
      auto * node1 = graph.add_node_for_processable (*processables_[0]);
      auto * node2 = graph.add_node_for_processable (*processables_[1]);
      auto * node3 = graph.add_node_for_processable (*processables_[2]);

      node1->connect_to (*node2);
      node2->connect_to (*node3);
    });

  create_dispatcher ();

  // First recalculation
  EXPECT_CALL (
    *processables_[0],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);
  EXPECT_CALL (
    *processables_[1],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);
  EXPECT_CALL (
    *processables_[2],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);

  dispatcher_->recalc_graph (false);

  // Second recalculation (should release old resources and prepare new ones)
  EXPECT_CALL (*processables_[0], release_resources ()).Times (1);
  EXPECT_CALL (*processables_[1], release_resources ()).Times (1);
  EXPECT_CALL (*processables_[2], release_resources ()).Times (1);

  EXPECT_CALL (
    *processables_[0],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);
  EXPECT_CALL (
    *processables_[1],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);
  EXPECT_CALL (
    *processables_[2],
    prepare_for_processing (_, units::sample_rate (48000), 256))
    .Times (1);

  dispatcher_->recalc_graph (false);

  // These will be called again on destruction
  EXPECT_CALL (*processables_[0], release_resources ()).Times (1);
  EXPECT_CALL (*processables_[1], release_resources ()).Times (1);
  EXPECT_CALL (*processables_[2], release_resources ()).Times (1);
}

TEST_F (DspGraphDispatcherTest, PreprocessAtStartOfCycleNoThrow)
{
  create_dispatcher ();

  EngineProcessTimeInfo time_info{};
  time_info.g_start_frame_ = 0;
  time_info.g_start_frame_w_offset_ = 0;
  time_info.local_offset_ = 0;
  time_info.nframes_ = 256;

  // This should not throw even without a scheduler
  EXPECT_NO_THROW ({
    // Access private method through a test-specific approach
    // Since preprocess_at_start_of_cycle is private, we test it indirectly
    // through start_cycle which calls it
    dispatcher_->start_cycle (*transport_, time_info, 0, false, *tempo_map_);
  });
}

}
