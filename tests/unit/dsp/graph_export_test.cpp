// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/graph_export.h"
#include "utils/gtest_wrapper.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace zrythm::dsp::graph;
using namespace zrythm::dsp::graph_test;

class GraphExportTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    transport_ = std::make_unique<MockTransport> ();
    processable1_ = std::make_unique<MockProcessable> ();
    processable2_ = std::make_unique<MockProcessable> ();

    ON_CALL (*processable1_, get_node_name ()).WillByDefault (Return (u8"node1"));
    ON_CALL (*processable2_, get_node_name ()).WillByDefault (Return (u8"node2"));
  }

  std::unique_ptr<MockTransport>   transport_;
  std::unique_ptr<MockProcessable> processable1_;
  std::unique_ptr<MockProcessable> processable2_;
};

TEST_F (GraphExportTest, EmptyGraphExport)
{
  Graph graph;
  auto  dot = GraphExport::export_to_dot (graph, false);

  EXPECT_THAT (dot.view (), HasSubstr ("digraph G"));
  EXPECT_THAT (dot.view (), HasSubstr ("}"));
  EXPECT_THAT (dot.view (), Not (HasSubstr ("->")));
}

TEST_F (GraphExportTest, SingleNodeExport)
{
  Graph graph;
  graph.add_node_for_processable (*processable1_, *transport_);
  graph.finalize_nodes ();

  auto dot = GraphExport::export_to_dot (graph, false);

  EXPECT_THAT (dot.view (), HasSubstr ("node1"));
  EXPECT_THAT (dot.view (), Not (HasSubstr ("->")));
}

TEST_F (GraphExportTest, ConnectedNodesExport)
{
  Graph  graph;
  auto * node1 = graph.add_node_for_processable (*processable1_, *transport_);
  auto * node2 = graph.add_node_for_processable (*processable2_, *transport_);
  node1->connect_to (*node2);
  graph.finalize_nodes ();

  auto dot = GraphExport::export_to_dot (graph, false);

  EXPECT_THAT (dot.view (), HasSubstr ("node1"));
  EXPECT_THAT (dot.view (), HasSubstr ("node2"));
  EXPECT_THAT (dot.view (), HasSubstr ("->"));
  EXPECT_THAT (
    dot.view (),
    HasSubstr (
      std::to_string (node1->get_id ()) + " -> "
      + std::to_string (node2->get_id ())));
}

TEST_F (GraphExportTest, ClassNameInclusion)
{
  Graph graph;
  graph.add_node_for_processable (*processable1_, *transport_);
  graph.finalize_nodes ();

  auto dot = GraphExport::export_to_dot (graph, true);

  // Should contain class name pattern
  EXPECT_THAT (dot.view (), HasSubstr ("\\n("));
}
