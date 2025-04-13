// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_builder.h"
#include "dsp/itransport.h"
#include "utils/gtest_wrapper.h"

#include <gmock/gmock.h>

using namespace testing;

namespace zrythm::dsp
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

// Simple test implementation of IGraphBuilder
class TestGraphBuilder : public IGraphBuilder
{
public:
  TestGraphBuilder (MockTransport &transport, MockProcessable &processable)
      : transport_ (transport), processable_ (processable)
  {
  }

protected:
  void build_graph_impl (Graph &graph) override
  {
    // Create a simple chain of 3 nodes
    auto * node1 = graph.add_node_for_processable (processable_, transport_);
    auto * node2 = graph.add_node_for_processable (processable_, transport_);
    auto * node3 = graph.add_node_for_processable (processable_, transport_);

    node1->connect_to (*node2);
    node2->connect_to (*node3);
  }

private:
  MockTransport   &transport_;
  MockProcessable &processable_;
};

class GraphBuilderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    transport_ = std::make_unique<MockTransport> ();
    processable_ = std::make_unique<MockProcessable> ();

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return ("test_node"));
  }

  std::unique_ptr<MockTransport>   transport_;
  std::unique_ptr<MockProcessable> processable_;
};

TEST_F (GraphBuilderTest, BuildsValidGraph)
{
  TestGraphBuilder builder (*transport_, *processable_);
  Graph            graph;

  builder.build_graph (graph);

  EXPECT_TRUE (graph.is_valid ());
  EXPECT_EQ (graph.get_nodes ().graph_nodes_.size (), 3);
  EXPECT_EQ (graph.get_nodes ().trigger_nodes_.size (), 1);
  EXPECT_EQ (graph.get_nodes ().terminal_nodes_.size (), 1);
}

TEST_F (GraphBuilderTest, NodesProperlyConnected)
{
  TestGraphBuilder builder (*transport_, *processable_);
  Graph            graph;

  builder.build_graph (graph);

  const auto &nodes = graph.get_nodes ().graph_nodes_;
  EXPECT_EQ (nodes[0]->childnodes_.size (), 1);
  EXPECT_EQ (nodes[1]->childnodes_.size (), 1);
  EXPECT_EQ (nodes[2]->childnodes_.size (), 0);
}

TEST_F (GraphBuilderTest, LatenciesUpdated)
{
  EXPECT_CALL (*processable_, get_single_playback_latency ())
    .WillRepeatedly (Return (128));

  TestGraphBuilder builder (*transport_, *processable_);
  Graph            graph;

  builder.build_graph (graph);

  EXPECT_EQ (graph.get_nodes ().get_max_route_playback_latency (), 128);
}

}
