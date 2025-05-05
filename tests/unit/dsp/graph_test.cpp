// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/itransport.h"
#include "utils/gtest_wrapper.h"

#include <gmock/gmock.h>

using namespace testing;

using namespace zrythm::dsp;

namespace graph_test
{
class MockProcessable : public IProcessable
{
public:
  MOCK_METHOD (std::string, get_node_name, (), (const, override));
  MOCK_METHOD (nframes_t, get_single_playback_latency, (), (const, override));
  MOCK_METHOD (void, process_block, (EngineProcessTimeInfo), (override));
};

class MockTransport : public ITransport
{
public:
  MOCK_METHOD (
    void,
    position_add_frames,
    (Position &, signed_frame_t),
    (const, override));
  MOCK_METHOD (
    (std::pair<Position, Position>),
    get_loop_range_positions,
    (),
    (const, override));
  MOCK_METHOD (PlayState, get_play_state, (), (const, override));
  MOCK_METHOD (Position, get_playhead_position, (), (const, override));
  MOCK_METHOD (bool, get_loop_enabled, (), (const, override));
  MOCK_METHOD (
    nframes_t,
    is_loop_point_met,
    (signed_frame_t g_start_frames, nframes_t nframes),
    (const, override));
};
}; // namespace graph_test

class GraphTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    transport_ = std::make_unique<graph_test::MockTransport> ();
    processable_ = std::make_unique<graph_test::MockProcessable> ();

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return ("test_node"));
  }

  std::unique_ptr<graph_test::MockTransport>   transport_;
  std::unique_ptr<graph_test::MockProcessable> processable_;
};

TEST_F (GraphTest, BasicNodeAddition)
{
  Graph  graph;
  auto * node = graph.add_node_for_processable (*processable_, *transport_);

  EXPECT_NE (node, nullptr);
  EXPECT_EQ (graph.get_nodes ().graph_nodes_.size (), 1);
}

TEST_F (GraphTest, InitialProcessorNode)
{
  Graph  graph;
  auto * initial_node = graph.add_initial_processor (*transport_);

  EXPECT_NE (initial_node, nullptr);
  EXPECT_NE (graph.get_nodes ().initial_processor_, nullptr);
}

TEST_F (GraphTest, ValidGraphCheck)
{
  Graph graph;

  auto * node1 = graph.add_node_for_processable (*processable_, *transport_);
  auto * node2 = graph.add_node_for_processable (*processable_, *transport_);

  node1->connect_to (*node2);

  graph.finalize_nodes ();

  EXPECT_TRUE (graph.is_valid ());
}

TEST_F (GraphTest, StealNodes)
{
  Graph graph;
  graph.add_node_for_processable (*processable_, *transport_);

  auto nodes = graph.steal_nodes ();
  EXPECT_EQ (nodes.graph_nodes_.size (), 1);
  EXPECT_EQ (graph.get_nodes ().graph_nodes_.size (), 0);
}

TEST_F (GraphTest, CompleteGraphSetup)
{
  Graph graph;

  auto * node1 = graph.add_node_for_processable (*processable_, *transport_);
  auto * node2 = graph.add_node_for_processable (*processable_, *transport_);
  auto * node3 = graph.add_node_for_processable (*processable_, *transport_);

  node1->connect_to (*node2);
  node2->connect_to (*node3);

  graph.finalize_nodes ();

  auto &nodes = graph.get_nodes ();
  EXPECT_EQ (nodes.trigger_nodes_.size (), 1);
  EXPECT_EQ (nodes.terminal_nodes_.size (), 1);
}

TEST_F (GraphTest, CyclicGraphDetection)
{
  Graph graph;

  auto * node1 = graph.add_node_for_processable (*processable_, *transport_);
  auto * node2 = graph.add_node_for_processable (*processable_, *transport_);

  node1->connect_to (*node2);
  node2->connect_to (*node1);

  graph.finalize_nodes ();

  EXPECT_FALSE (graph.is_valid ());
}
