// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_builder.h"
#include "dsp/itransport.h"
#include "utils/gtest_wrapper.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>

using namespace testing;

using namespace zrythm::dsp::graph;

// Simple test implementation of IGraphBuilder
class TestGraphBuilder : public IGraphBuilder
{
public:
  using MockTransport = zrythm::dsp::graph_test::MockTransport;
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;

  TestGraphBuilder (
    MockTransport                              &transport,
    std::span<std::unique_ptr<MockProcessable>> processables)
      : transport_ (transport), processables_ (processables)
  {
  }

protected:
  void build_graph_impl (Graph &graph) override
  {
    // Create a simple chain of 3 nodes
    auto * node1 =
      graph.add_node_for_processable (*processables_[0], transport_);
    auto * node2 =
      graph.add_node_for_processable (*processables_[1], transport_);
    auto * node3 =
      graph.add_node_for_processable (*processables_[2], transport_);

    node1->connect_to (*node2);
    node2->connect_to (*node3);
  }

private:
  MockTransport                              &transport_;
  std::span<std::unique_ptr<MockProcessable>> processables_;
};

class GraphBuilderTest : public ::testing::Test
{
protected:
  using MockTransport = zrythm::dsp::graph_test::MockTransport;
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;

  void SetUp () override
  {
    transport_ = std::make_unique<MockTransport> ();

    // Create 3 test processables
    for (const auto _ : std::views::iota (0, 3))
      {
        processables_.emplace_back (std::make_unique<MockProcessable> ());
        ON_CALL (*processables_.back (), get_node_name ())
          .WillByDefault (Return (u8"test_node"));
      }
  }

  std::unique_ptr<MockTransport>                transport_;
  std::vector<std::unique_ptr<MockProcessable>> processables_;
};

TEST_F (GraphBuilderTest, BuildsValidGraph)
{
  TestGraphBuilder builder (*transport_, std::span (processables_));
  Graph            graph;

  builder.build_graph (graph);

  EXPECT_TRUE (graph.is_valid ());
  EXPECT_EQ (graph.get_nodes ().graph_nodes_.size (), 3);
  EXPECT_EQ (graph.get_nodes ().trigger_nodes_.size (), 1);
  EXPECT_EQ (graph.get_nodes ().terminal_nodes_.size (), 1);
}

TEST_F (GraphBuilderTest, NodesProperlyConnected)
{
  TestGraphBuilder builder (*transport_, std::span (processables_));
  Graph            graph;

  builder.build_graph (graph);

  const auto &nodes = graph.get_nodes ().graph_nodes_;
  EXPECT_EQ (nodes.at (0)->childnodes_.size (), 1);
  EXPECT_EQ (nodes.at (1)->childnodes_.size (), 1);
  EXPECT_EQ (nodes.at (2)->childnodes_.size (), 0);
}

TEST_F (GraphBuilderTest, LatenciesUpdated)
{
  for (const auto &processable : processables_)
    {
      EXPECT_CALL (*processable, get_single_playback_latency ())
        .WillRepeatedly (Return (128));
    }

  TestGraphBuilder builder (*transport_, std::span (processables_));
  Graph            graph;

  builder.build_graph (graph);
  graph.get_nodes ().update_latencies ();

  EXPECT_EQ (graph.get_nodes ().get_max_route_playback_latency (), 128);
}
