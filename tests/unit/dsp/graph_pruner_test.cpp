// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_pruner.h"

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
    graph_node_a_->connect_to (*graph_node_b_);
    graph_node_b_->connect_to (*graph_node_c_);
    graph_node_c_->connect_to (*graph_node_d_);

    graph_.finalize_nodes ();
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
};

TEST_F (GraphPrunerTest, PruneToSingleTerminal)
{
  // Verify initial graph has 4 nodes and 1 terminal (D)
  EXPECT_EQ (graph_.get_nodes ().graph_nodes_.size (), 4);
  EXPECT_EQ (graph_.get_nodes ().terminal_nodes_.size (), 1);

  // Prune to only terminal D (should keep all nodes since D depends on all)
  std::vector<std::reference_wrapper<GraphNode>> terminals;
  terminals.emplace_back (*graph_node_d_);

  bool result = GraphPruner::prune_graph_to_terminals (graph_, terminals);

  EXPECT_TRUE (result);
  EXPECT_EQ (graph_.get_nodes ().graph_nodes_.size (), 4);
  EXPECT_EQ (graph_.get_nodes ().terminal_nodes_.size (), 1);
  EXPECT_TRUE (graph_.is_valid ());
}

TEST_F (GraphPrunerTest, PruneToMiddleNodeAsTerminal)
{
  // Add another branch: B -> E
  auto node_e = std::make_unique<MockProcessable> ("E");
  auto graph_node_e = graph_.add_node_for_processable (*node_e);
  graph_node_b_->connect_to (*graph_node_e);
  graph_.finalize_nodes ();

  // Now we have 2 terminals: D and E
  EXPECT_EQ (graph_.get_nodes ().graph_nodes_.size (), 5);
  EXPECT_EQ (graph_.get_nodes ().terminal_nodes_.size (), 2);

  // Prune to only terminal E (should remove D and C)
  std::vector<std::reference_wrapper<GraphNode>> terminals;
  terminals.emplace_back (*graph_node_e);

  bool result = GraphPruner::prune_graph_to_terminals (graph_, terminals);

  EXPECT_TRUE (result);
  EXPECT_EQ (graph_.get_nodes ().graph_nodes_.size (), 3);    // A, B, E
  EXPECT_EQ (graph_.get_nodes ().terminal_nodes_.size (), 1); // E only
  EXPECT_TRUE (graph_.is_valid ());

  // Verify remaining nodes are the correct ones
  bool found_a = false, found_b = false, found_e = false;
  for (const auto &node : graph_.get_nodes ().graph_nodes_)
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
}

TEST_F (GraphPrunerTest, PruneWithEmptyTerminals)
{
  // Test with empty terminal list should fail
  std::vector<std::reference_wrapper<GraphNode>> empty_terminals;

  bool result = GraphPruner::prune_graph_to_terminals (graph_, empty_terminals);

  EXPECT_FALSE (result);
  // Graph should remain unchanged
  EXPECT_EQ (graph_.get_nodes ().graph_nodes_.size (), 4);
}
}
