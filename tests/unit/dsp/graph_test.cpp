// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/graph.h"
#include "dsp/itransport.h"
#include "utils/utf8_string.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace zrythm::dsp::graph
{

class GraphTest : public ::testing::Test
{
protected:
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;

  void SetUp () override
  {
    processable_ = std::make_unique<MockProcessable> ();

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return (u8"test_node"));
  }

  std::unique_ptr<MockProcessable> processable_;
};

TEST_F (GraphTest, BasicNodeAddition)
{
  Graph  graph;
  auto * node = graph.add_node_for_processable (*processable_);

  EXPECT_NE (node, nullptr);
  EXPECT_EQ (graph.nodes ().size (), 1);
}

TEST_F (GraphTest, InitialProcessorNode)
{
  Graph graph;

  auto * initial_node = graph.add_initial_processor ();

  EXPECT_NE (initial_node, nullptr);
  EXPECT_EQ (graph.nodes ().size (), 1);
}

TEST_F (GraphTest, ValidGraphCheck)
{
  Graph graph;

  auto * node1 = graph.add_node_for_processable (*processable_);
  auto   new_procesable = std::make_unique<MockProcessable> ();
  auto * node2 = graph.add_node_for_processable (*new_procesable);

  graph.connect (*node1, *node2);

  EXPECT_TRUE (graph.is_valid ());
}

TEST_F (GraphTest, FinalizeConsumesTheGraph)
{
  Graph graph;
  graph.add_node_for_processable (*processable_);

  auto nodes = std::move (graph).finalize ();
  EXPECT_EQ (nodes.nodes ().size (), 1);
  EXPECT_EQ (graph.nodes ().size (), 0);
}

TEST_F (GraphTest, CompleteGraphSetup)
{
  Graph graph;

  auto * node1 = graph.add_node_for_processable (*processable_);
  auto   processable2 = std::make_unique<MockProcessable> ();
  auto * node2 = graph.add_node_for_processable (*processable2);
  auto   processable3 = std::make_unique<MockProcessable> ();
  auto * node3 = graph.add_node_for_processable (*processable3);

  graph.connect (*node1, *node2);
  graph.connect (*node2, *node3);

  const auto nodes = std::move (graph).finalize ();
  EXPECT_EQ (nodes.trigger_nodes ().size (), 1);
  EXPECT_EQ (nodes.terminal_nodes ().size (), 1);
}

TEST_F (GraphTest, CyclicGraphDetection)
{
  Graph graph;

  auto processable2 = std::make_unique<MockProcessable> ();
  ON_CALL (*processable2, get_node_name ()).WillByDefault (Return (u8"node2"));

  auto * node1 = graph.add_node_for_processable (*processable_);
  auto * node2 = graph.add_node_for_processable (*processable2);

  graph.connect (*node1, *node2);
  graph.connect (*node2, *node1);

  EXPECT_FALSE (graph.is_valid ());
}

TEST_F (GraphTest, AddNodeForProcessableDisallowsDuplicates)
{
  Graph graph;

  // Add the same processable twice
  auto * node1 = graph.add_node_for_processable (*processable_);
  auto * node2 = graph.add_node_for_processable (*processable_);

  // Should return the same node both times
  EXPECT_EQ (node1, node2);

  // Should only create one node
  EXPECT_EQ (graph.nodes ().size (), 1);
}

TEST_F (GraphTest, DiamondGraphIsValid)
{
  // source → left → sink
  // source → right → sink
  Graph graph;

  auto p_left = std::make_unique<MockProcessable> ();
  auto p_right = std::make_unique<MockProcessable> ();
  auto p_sink = std::make_unique<MockProcessable> ();
  ON_CALL (*p_left, get_node_name ()).WillByDefault (Return (u8"left"));
  ON_CALL (*p_right, get_node_name ()).WillByDefault (Return (u8"right"));
  ON_CALL (*p_sink, get_node_name ()).WillByDefault (Return (u8"sink"));

  auto * source = graph.add_node_for_processable (*processable_);
  auto * left = graph.add_node_for_processable (*p_left);
  auto * right = graph.add_node_for_processable (*p_right);
  auto * sink = graph.add_node_for_processable (*p_sink);

  graph.connect (*source, *left);
  graph.connect (*source, *right);
  graph.connect (*left, *sink);
  graph.connect (*right, *sink);

  EXPECT_TRUE (graph.is_valid ());
}

TEST_F (GraphTest, IsValidIdempotent)
{
  Graph graph;

  auto p2 = std::make_unique<MockProcessable> ();
  auto p3 = std::make_unique<MockProcessable> ();
  ON_CALL (*p2, get_node_name ()).WillByDefault (Return (u8"b"));
  ON_CALL (*p3, get_node_name ()).WillByDefault (Return (u8"c"));

  auto * n1 = graph.add_node_for_processable (*processable_);
  auto * n2 = graph.add_node_for_processable (*p2);
  auto * n3 = graph.add_node_for_processable (*p3);

  graph.connect (*n1, *n2);
  graph.connect (*n2, *n3);

  EXPECT_TRUE (graph.is_valid ());
  EXPECT_TRUE (graph.is_valid ());
}

TEST_F (GraphTest, DiamondGraphWithCycleIsInvalid)
{
  Graph graph;

  auto p_left = std::make_unique<MockProcessable> ();
  auto p_right = std::make_unique<MockProcessable> ();
  auto p_sink = std::make_unique<MockProcessable> ();
  ON_CALL (*p_left, get_node_name ()).WillByDefault (Return (u8"left"));
  ON_CALL (*p_right, get_node_name ()).WillByDefault (Return (u8"right"));
  ON_CALL (*p_sink, get_node_name ()).WillByDefault (Return (u8"sink"));

  auto * source = graph.add_node_for_processable (*processable_);
  auto * left = graph.add_node_for_processable (*p_left);
  auto * right = graph.add_node_for_processable (*p_right);
  auto * sink = graph.add_node_for_processable (*p_sink);

  graph.connect (*source, *left);
  graph.connect (*source, *right);
  graph.connect (*left, *sink);
  graph.connect (*right, *sink);
  graph.connect (*sink, *left);

  EXPECT_FALSE (graph.is_valid ());
}

TEST_F (GraphTest, WideFanOutIsValid)
{
  // source feeds 5 independent chains, all converging on sink
  Graph graph;

  std::array<std::unique_ptr<MockProcessable>, 5> mids;
  auto p_sink = std::make_unique<MockProcessable> ();
  ON_CALL (*p_sink, get_node_name ()).WillByDefault (Return (u8"sink"));

  auto * source = graph.add_node_for_processable (*processable_);
  auto * sink = graph.add_node_for_processable (*p_sink);

  for (size_t i = 0; i < mids.size (); ++i)
    {
      mids[i] = std::make_unique<MockProcessable> ();
      ON_CALL (*mids[i], get_node_name ())
        .WillByDefault (Return (
          utils::Utf8String::from_utf8_encoded_string (fmt::format ("mid{}", i))));
      auto * mid = graph.add_node_for_processable (*mids[i]);
      graph.connect (*source, *mid);
      graph.connect (*mid, *sink);
    }

  EXPECT_TRUE (graph.is_valid ());
}

// A candidate edge must validate without finalizing the graph first
TEST_F (GraphTest, IsValidAfterConnectWithoutFinalize)
{
  Graph graph;

  auto   processable2 = std::make_unique<MockProcessable> ();
  auto * src = graph.add_node_for_processable (*processable_);
  auto * dest = graph.add_node_for_processable (*processable2);

  // dest was previously unconnected
  graph.connect (*src, *dest);

  EXPECT_TRUE (graph.is_valid ());
}

TEST_F (GraphTest, DetectsCycleAfterConnectWithoutFinalize)
{
  Graph graph;

  auto   processable2 = std::make_unique<MockProcessable> ();
  auto * node1 = graph.add_node_for_processable (*processable_);
  auto * node2 = graph.add_node_for_processable (*processable2);

  graph.connect (*node1, *node2);
  graph.connect (*node2, *node1);

  EXPECT_FALSE (graph.is_valid ());
}

// Erasing a node severs its edges into surviving nodes completely:
// survivors' dependency counts and derived node sets reflect the removal
TEST_F (GraphTest, ErasedNodeEdgesAreSeveredIntoSurvivors)
{
  Graph graph;

  auto   processable2 = std::make_unique<MockProcessable> ();
  auto   processable3 = std::make_unique<MockProcessable> ();
  auto * a = graph.add_node_for_processable (*processable_);
  auto * b = graph.add_node_for_processable (*processable2);
  auto * c = graph.add_node_for_processable (*processable3);

  graph.connect (*a, *b);
  graph.connect (*b, *c);
  ASSERT_TRUE (graph.is_valid ());

  graph.erase_nodes_if ([b] (const auto &node) { return node.get () == b; });

  EXPECT_TRUE (graph.is_valid ());
  const auto nodes = std::move (graph).finalize ();
  EXPECT_EQ (nodes.trigger_nodes ().size (), 2); // a and c
}

// With several nodes sharing one processable, lookups return the first
// node added; after it is erased, they return the next remaining node
TEST_F (GraphTest, FindNodeForProcessableReturnsFirstNode)
{
  Graph graph;

  auto * node1 = graph.add_node (std::make_unique<GraphNode> (1, *processable_));
  auto * node2 = graph.add_node (std::make_unique<GraphNode> (2, *processable_));
  EXPECT_EQ (graph.find_node_for_processable (*processable_), node1);

  graph.erase_nodes_if ([node1] (const auto &node) {
    return node.get () == node1;
  });
  EXPECT_EQ (graph.find_node_for_processable (*processable_), node2);
}
}
