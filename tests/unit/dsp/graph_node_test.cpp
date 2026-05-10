// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/graph_node.h"
#include "dsp/itransport.h"
#include "utils/gtest_wrapper.h"
#include "utils/utf8_string.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>

using namespace testing;

namespace zrythm::dsp::graph
{

class GraphNodeTest : public ::testing::Test
{
protected:
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;
  using MockTransport = zrythm::dsp::graph_test::MockTransport;

  void SetUp () override
  {
    transport_ = std::make_unique<MockTransport> ();
    processable_ = std::make_unique<MockProcessable> ();

    // Default expectations
    ON_CALL (*transport_, get_play_state ())
      .WillByDefault (Return (ITransport::PlayState::Paused));
    ON_CALL (*transport_, get_playhead_position_in_audio_thread ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (*transport_, loop_enabled ()).WillByDefault (Return (false));
    ON_CALL (*transport_, get_loop_range_positions ())
      .WillByDefault (Return (
        std::make_pair (
          units::samples (0),
          units::samples (static_cast<int64_t> (1920.0 * 22.675736961451247)))));

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return (u8"test_node"));
    ON_CALL (*processable_, get_single_playback_latency ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (*processable_, prepare_for_processing (_, _, _))
      .WillByDefault (Return ());
    ON_CALL (*processable_, release_resources ()).WillByDefault (Return ());

    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (48000));
  }

  GraphNode create_test_node () { return { 1, *processable_ }; }

  std::unique_ptr<MockTransport>   transport_;
  std::unique_ptr<MockProcessable> processable_;
  std::unique_ptr<dsp::TempoMap>   tempo_map_;
};

TEST_F (GraphNodeTest, Construction)
{
  EXPECT_CALL (*processable_, get_node_name ()).WillOnce (Return (u8"test_node"));

  auto node = create_test_node ();
  EXPECT_EQ (node.get_processable ().get_node_name (), "test_node");
  EXPECT_TRUE (node.feeds ().empty ());
  EXPECT_FALSE (node.terminal_);
  EXPECT_FALSE (node.initial_);
}

TEST_F (GraphNodeTest, ProcessingBasics)
{
  EXPECT_CALL (*processable_, process_block (_, _, _)).Times (Exactly (1));

  auto node = create_test_node ();
  const auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));
  node.process (time_info, units::samples (0), *transport_, *tempo_map_);
}

TEST_F (GraphNodeTest, LatencyHandling)
{
  EXPECT_CALL (*processable_, get_single_playback_latency ())
    .WillRepeatedly (Return (units::samples (512)));

  auto node = create_test_node ();
  EXPECT_EQ (node.get_single_playback_latency (), units::samples (512));

  node.set_route_playback_latency (units::samples (1024));
  EXPECT_EQ (node.route_playback_latency_, units::samples (1024));
}

TEST_F (GraphNodeTest, NodeConnections)
{
  auto node1 = create_test_node ();
  auto node2 = create_test_node ();

  node1.connect_to (node2);

  EXPECT_EQ (node1.feeds ().size (), 1);
  EXPECT_FALSE (node1.terminal_);
  EXPECT_FALSE (node2.initial_);
  EXPECT_EQ (node2.init_refcount_, 1);
  EXPECT_EQ (node2.refcount_, 1);
}

TEST_F (GraphNodeTest, SkipProcessing)
{
  EXPECT_CALL (*processable_, process_block (_, _, _)).Times (0);

  auto node = create_test_node ();
  node.set_skip_processing (true);

  const auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));
  node.process (time_info, units::samples (0), *transport_, *tempo_map_);
}

TEST_F (GraphNodeTest, ProcessingWithTransport)
{
  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (Return (ITransport::PlayState::Rolling));
  EXPECT_CALL (*processable_, process_block (_, _, _)).Times (1);

  auto node = create_test_node ();
  const auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));
  node.process (time_info, units::samples (0), *transport_, *tempo_map_);
}

TEST_F (GraphNodeTest, LoopPointProcessing)
{
  EXPECT_CALL (*transport_, loop_enabled ()).WillRepeatedly (Return (true));
  EXPECT_CALL (*transport_, get_loop_range_positions ())
    .WillRepeatedly (
      Return (std::make_pair (units::samples (0), units::samples (128))));
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (Return (ITransport::PlayState::Rolling));
  EXPECT_CALL (*processable_, process_block (_, _, _)).Times (2);

  auto node = create_test_node ();
  const auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));
  node.process (time_info, units::samples (0), *transport_, *tempo_map_);
}

// New test for multiple node connections
TEST_F (GraphNodeTest, MultipleNodeConnections)
{
  auto node1 = create_test_node ();
  auto node2 = create_test_node ();
  auto node3 = create_test_node ();

  node1.connect_to (node2);
  node1.connect_to (node3);
  node2.connect_to (node3);

  EXPECT_EQ (node1.feeds ().size (), 2);
  EXPECT_EQ (node2.feeds ().size (), 1);
  EXPECT_EQ (node3.feeds ().size (), 0);
  EXPECT_EQ (node3.init_refcount_, 2);
}

// New test for latency propagation
TEST_F (GraphNodeTest, LatencyPropagation)
{
  auto node1 = create_test_node ();
  auto node2 = create_test_node ();
  auto node3 = create_test_node ();

  node1.connect_to (node2);
  node2.connect_to (node3);

  node3.set_route_playback_latency (units::samples (1000));

  EXPECT_EQ (node3.route_playback_latency_, units::samples (1000));
  EXPECT_EQ (node2.route_playback_latency_, units::samples (1000));
  EXPECT_EQ (node1.route_playback_latency_, units::samples (1000));
}

TEST_F (GraphNodeTest, ProcessingWithLoopAndLatency)
{
  EXPECT_CALL (*transport_, loop_enabled ()).WillRepeatedly (Return (true));
  EXPECT_CALL (*processable_, get_single_playback_latency ())
    .WillRepeatedly (Return (units::samples (128)));
  EXPECT_CALL (*transport_, get_loop_range_positions ())
    .WillRepeatedly (
      Return (std::make_pair (units::samples (0), units::samples (192))));
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (Return (ITransport::PlayState::Rolling));
  EXPECT_CALL (*transport_, get_playhead_position_in_audio_thread ())
    .WillRepeatedly (Return (units::samples (0)));

  auto node = create_test_node ();
  node.set_route_playback_latency (units::samples (256));

  std::vector<ProcessBlockInfo> captured;
  EXPECT_CALL (*processable_, process_block (_, _, _))
    .Times (2)
    .WillRepeatedly (
      Invoke ([&captured] (ProcessBlockInfo info, const auto &, const auto &) {
        captured.push_back (info);
      }));

  const auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));
  node.process (time_info, units::samples (64), *transport_, *tempo_map_);

  ASSERT_EQ (captured.size (), 2u);

  // compensate_latency adjusts transport_position_ by adding the route
  // latency and buffer_offset_.
  //
  // playhead=0, route_latency=256, preroll_remaining=64, buffer_offset=0
  // frames_to_add = 256 - 64 = 192.  With loop [0,192), get_playhead_...
  // wraps: 0 + 192 = 192 >= loop_end, so adjusted = 192 - 192 = 0.
  // transport_position_ = 0 + 0 (buffer_offset) = 0.
  //
  // process_chunks_after_splitting_at_loop_points then sees transport_position
  // 0 within the loop [0,192): the full 192 frames are processable (reaching
  // exactly loop_end), then the remaining 64 wrap to loop_start=0.
  EXPECT_EQ (captured[0].transport_position_, units::samples (0u));
  EXPECT_EQ (captured[0].buffer_offset_, units::samples (0u));
  EXPECT_EQ (captured[0].nframes_, units::samples (192u));

  // Chunk 2: wraps back to loop start, buffer_offset advances by 192
  EXPECT_EQ (captured[1].transport_position_, units::samples (0u));
  EXPECT_EQ (captured[1].buffer_offset_, units::samples (192u));
  EXPECT_EQ (captured[1].nframes_, units::samples (64u));
}

TEST_F (GraphNodeTest, LatencyCompensationWithBufferOffset)
{
  EXPECT_CALL (*transport_, loop_enabled ()).WillRepeatedly (Return (false));
  EXPECT_CALL (*processable_, get_single_playback_latency ())
    .WillRepeatedly (Return (units::samples (128)));
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (Return (ITransport::PlayState::Rolling));
  EXPECT_CALL (*transport_, get_playhead_position_in_audio_thread ())
    .WillRepeatedly (Return (units::samples (1000)));

  auto node = create_test_node ();
  node.set_route_playback_latency (units::samples (256));

  ProcessBlockInfo captured{ ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (0)) };
  EXPECT_CALL (*processable_, process_block (_, _, _))
    .Times (1)
    .WillOnce (
      Invoke ([&captured] (ProcessBlockInfo info, const auto &, const auto &) {
        captured = info;
      }));

  // Simulate an engine split: base playhead=1000, buffer_offset=128,
  // nframes=128
  ProcessBlockInfo time_nfo = {
    .transport_position_ = units::samples (1000u),
    .buffer_offset_ = units::samples (128u),
    .nframes_ = units::samples (128u),
  };

  // remaining_preroll=0, so latency adjustment = route_latency - 0 = 256
  node.process (time_nfo, units::samples (0), *transport_, *tempo_map_);

  // compensate_latency adjusts transport_position by adding the route latency.
  // With playhead=1000, latency=256, buffer_offset=128:
  // adjusted_position = 1000 + 256 = 1256.
  // The doc comment says transport_position_ is "already taking into account
  // the offset", so the expected value should be 1256 + 128 = 1384.
  EXPECT_EQ (captured.transport_position_, units::samples (1384u))
    << "transport_position should include both latency adjustment (256) and "
       "buffer_offset (128): playhead(1000) + latency(256) + offset(128) = "
       "1384";
  EXPECT_EQ (captured.buffer_offset_, units::samples (128u));
  EXPECT_EQ (captured.nframes_, units::samples (128u));
}

TEST_F (GraphNodeTest, NonRollingTransportPositionIncludesBufferOffset)
{
  EXPECT_CALL (*transport_, get_play_state ())
    .WillRepeatedly (Return (ITransport::PlayState::Paused));
  EXPECT_CALL (*transport_, loop_enabled ()).WillRepeatedly (Return (false));

  auto node = create_test_node ();

  ProcessBlockInfo captured{ ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (0)) };
  EXPECT_CALL (*processable_, process_block (_, _, _))
    .Times (1)
    .WillOnce (
      Invoke ([&captured] (ProcessBlockInfo info, const auto &, const auto &) {
        captured = info;
      }));

  // Simulate an engine split during Paused transport:
  // playhead=500, buffer_offset=64, nframes=192
  ProcessBlockInfo time_nfo = {
    .transport_position_ = units::samples (500u),
    .buffer_offset_ = units::samples (64u),
    .nframes_ = units::samples (192u),
  };

  node.process (time_nfo, units::samples (0), *transport_, *tempo_map_);

  // transport_position_ must include buffer_offset_ even when transport is
  // not Rolling, per the documented contract ("already taking into account
  // the offset").
  EXPECT_EQ (captured.transport_position_, units::samples (564u))
    << "transport_position should include buffer_offset for non-Rolling "
       "transport: playhead(500) + offset(64) = 564";
  EXPECT_EQ (captured.buffer_offset_, units::samples (64u));
  EXPECT_EQ (captured.nframes_, units::samples (192u));
}

TEST_F (GraphNodeTest, ComplexGraphTopology)
{
  auto node1 = create_test_node ();
  auto node2 = create_test_node ();
  auto node3 = create_test_node ();
  auto node4 = create_test_node ();

  // Create diamond pattern
  node1.connect_to (node2);
  node1.connect_to (node3);
  node2.connect_to (node4);
  node3.connect_to (node4);

  // Verify graph structure
  EXPECT_EQ (node1.feeds ().size (), 2);
  EXPECT_EQ (node2.feeds ().size (), 1);
  EXPECT_EQ (node3.feeds ().size (), 1);
  EXPECT_EQ (node4.feeds ().size (), 0);

  // Verify reference counts
  EXPECT_EQ (node1.init_refcount_, 0);
  EXPECT_EQ (node2.init_refcount_, 1);
  EXPECT_EQ (node3.init_refcount_, 1);
  EXPECT_EQ (node4.init_refcount_, 2);
}

TEST_F (GraphNodeTest, NodeCollection)
{
  GraphNodeCollection collection;

  auto node1 = std::make_unique<GraphNode> (1, *processable_);
  auto node2 = std::make_unique<GraphNode> (2, *processable_);
  auto node3 = std::make_unique<GraphNode> (3, *processable_);

  node1->connect_to (*node2);
  node2->connect_to (*node3);

  collection.graph_nodes_.push_back (std::move (node1));
  collection.graph_nodes_.push_back (std::move (node2));
  collection.graph_nodes_.push_back (std::move (node3));

  collection.set_initial_and_terminal_nodes ();

  EXPECT_EQ (collection.trigger_nodes_.size (), 1);
  EXPECT_EQ (collection.terminal_nodes_.size (), 1);
}

TEST_F (GraphNodeTest, LatencyPropagationInCollection)
{
  GraphNodeCollection collection;

  EXPECT_CALL (*processable_, get_single_playback_latency ())
    .WillRepeatedly (Return (units::samples (128)));

  auto node1 = std::make_unique<GraphNode> (1, *processable_);
  auto node2 = std::make_unique<GraphNode> (2, *processable_);

  node1->connect_to (*node2);
  collection.graph_nodes_.push_back (std::move (node1));
  collection.graph_nodes_.push_back (std::move (node2));

  collection.finalize_nodes ();
  collection.update_latencies ();

  EXPECT_EQ (collection.get_max_route_playback_latency (), units::samples (128));
}

TEST_F (GraphNodeTest, ProcessableSearch)
{
  GraphNodeCollection collection;

  auto  node = std::make_unique<GraphNode> (1, *processable_);
  auto &node_ref = *node;
  collection.graph_nodes_.push_back (std::move (node));

  auto * found = collection.find_node_for_processable (*processable_);
  EXPECT_EQ (found, &node_ref);

  MockProcessable other_processable;
  auto * not_found = collection.find_node_for_processable (other_processable);
  EXPECT_EQ (not_found, nullptr);
}

} // namespace zrythm::dsp::graph
