// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/itransport.h"
#include "utils/gtest_wrapper.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>

using namespace testing;

using namespace zrythm::dsp::graph;

class GraphTest : public ::testing::Test
{
protected:
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;
  using MockTransport = zrythm::dsp::graph_test::MockTransport;

  void SetUp () override
  {
    transport_ = std::make_unique<MockTransport> ();
    processable_ = std::make_unique<MockProcessable> ();

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return (u8"test_node"));
  }

  std::unique_ptr<MockTransport>   transport_;
  std::unique_ptr<MockProcessable> processable_;
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
  auto   new_procesable = std::make_unique<MockProcessable> ();
  auto * node2 = graph.add_node_for_processable (*new_procesable, *transport_);

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
  auto   processable2 = std::make_unique<MockProcessable> ();
  auto * node2 = graph.add_node_for_processable (*processable2, *transport_);
  auto   processable3 = std::make_unique<MockProcessable> ();
  auto * node3 = graph.add_node_for_processable (*processable3, *transport_);

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

TEST_F (GraphTest, AddNodeForProcessableDisallowsDuplicates)
{
  Graph graph;

  // Add the same processable twice
  auto * node1 = graph.add_node_for_processable (*processable_, *transport_);
  auto * node2 = graph.add_node_for_processable (*processable_, *transport_);

  // Should return the same node both times
  EXPECT_EQ (node1, node2);

  // Should only create one node
  EXPECT_EQ (graph.get_nodes ().graph_nodes_.size (), 1);
}
