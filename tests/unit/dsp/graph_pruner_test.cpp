// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <memory>

#include "dsp/graph_pruner.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>

using namespace testing;

namespace zrythm::dsp::graph
{

class GraphPrunerTest : public ::testing::Test
{
protected:
  // Mock processable for testing
  class MockProcessable : public IProcessable
  {
  public:
    MockProcessable (std::string name) : name_ (std::move (name)) { }

    utils::Utf8String get_node_name () const override
    {
      return utils::Utf8String::from_utf8_encoded_string (name_);
    }

  private:
    std::string name_;
  };

  void SetUp () override
  {
    // Create a simple linear graph: A -> B -> C -> D
    node_a_ = std::make_unique<MockProcessable> ("A");
    node_b_ = std::make_unique<MockProcessable> ("B");
    node_c_ = std::make_unique<MockProcessable> ("C");
    node_d_ = std::make_unique<MockProcessable> ("D");

    graph_node_a_ = graph_.add_node_for_processable (*node_a_);
    graph_node_b_ = graph_.add_node_for_processable (*node_b_);
    graph_node_c_ = graph_.add_node_for_processable (*node_c_);
    graph_node_d_ = graph_.add_node_for_processable (*node_d_);

    // Connect: A -> B -> C -> D
    graph_.connect (*graph_node_a_, *graph_node_b_);
    graph_.connect (*graph_node_b_, *graph_node_c_);
    graph_.connect (*graph_node_c_, *graph_node_d_);
  }

  Graph                            graph_;
  std::unique_ptr<MockProcessable> node_a_;
  std::unique_ptr<MockProcessable> node_b_;
  std::unique_ptr<MockProcessable> node_c_;
  std::unique_ptr<MockProcessable> node_d_;
  GraphNode *                      graph_node_a_;
  GraphNode *                      graph_node_b_;
  GraphNode *                      graph_node_c_;
  GraphNode *                      graph_node_d_;

  /** Number of nodes with no outgoing edges, before finalization. */
  static size_t count_nodes_without_feeds (const Graph &graph)
  {
    return static_cast<size_t> (
      std::ranges::count_if (graph.nodes (), [] (const auto &node) {
        return node->feeds ().empty ();
      }));
  }
};

TEST_F (GraphPrunerTest, PruneToSingleTerminal)
{
  // Verify initial graph has 4 nodes and 1 terminal (D)
  EXPECT_EQ (graph_.nodes ().size (), 4);
  EXPECT_EQ (count_nodes_without_feeds (graph_), 1);

  // Prune to only terminal D (should keep all nodes since D depends on all)
  std::vector<std::reference_wrapper<GraphNode>> terminals;
  terminals.emplace_back (*graph_node_d_);

  bool result = GraphPruner::prune_graph_to_terminals (graph_, terminals);

  EXPECT_TRUE (result);
  EXPECT_EQ (graph_.nodes ().size (), 4);
  EXPECT_TRUE (graph_.is_valid ());

  const auto collection = std::move (graph_).finalize ();
  EXPECT_EQ (collection.terminal_nodes ().size (), 1);
}

TEST_F (GraphPrunerTest, PruneToMiddleNodeAsTerminal)
{
  // Add another branch: B -> E
  auto node_e = std::make_unique<MockProcessable> ("E");
  auto graph_node_e = graph_.add_node_for_processable (*node_e);
  graph_.connect (*graph_node_b_, *graph_node_e);

  // Now we have 2 terminals: D and E
  EXPECT_EQ (graph_.nodes ().size (), 5);
  EXPECT_EQ (count_nodes_without_feeds (graph_), 2);

  // Prune to only terminal E (should remove D and C)
  std::vector<std::reference_wrapper<GraphNode>> terminals;
  terminals.emplace_back (*graph_node_e);

  bool result = GraphPruner::prune_graph_to_terminals (graph_, terminals);

  EXPECT_TRUE (result);
  EXPECT_EQ (graph_.nodes ().size (), 3); // A, B, E
  EXPECT_TRUE (graph_.is_valid ());

  // Verify remaining nodes are the correct ones
  bool found_a = false, found_b = false, found_e = false;
  for (const auto &node : graph_.nodes ())
    {
      const auto name = node->get_processable ().get_node_name ().str ();
      if (name == "A")
        found_a = true;
      if (name == "B")
        found_b = true;
      if (name == "E")
        found_e = true;
    }
  EXPECT_TRUE (found_a);
  EXPECT_TRUE (found_b);
  EXPECT_TRUE (found_e);

  const auto collection = std::move (graph_).finalize ();
  EXPECT_EQ (collection.terminal_nodes ().size (), 1); // E only
}

TEST_F (GraphPrunerTest, PruneWithEmptyTerminals)
{
  // Test with empty terminal list should fail
  std::vector<std::reference_wrapper<GraphNode>> empty_terminals;

  bool result = GraphPruner::prune_graph_to_terminals (graph_, empty_terminals);

  EXPECT_FALSE (result);
  // Graph should remain unchanged
  EXPECT_EQ (graph_.nodes ().size (), 4);
}
}
