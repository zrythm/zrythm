// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_node.h"
#include "dsp/itransport.h"
#include "utils/gtest_wrapper.h"

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
    ON_CALL (*transport_, get_playhead_position ())
      .WillByDefault (Return (Position{}));
    ON_CALL (*transport_, get_loop_enabled ()).WillByDefault (Return (false));
    ON_CALL (*transport_, get_loop_range_positions ())
      .WillByDefault (Return (
        std::make_pair (
          Position{}, Position{ 1920.0, FramesPerTick{ 22.675736961451247 } })));

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return (u8"test_node"));
    ON_CALL (*processable_, get_single_playback_latency ())
      .WillByDefault (Return (0));
    ON_CALL (*processable_, prepare_for_processing (_, _))
      .WillByDefault (Return ());
    ON_CALL (*processable_, release_resources ()).WillByDefault (Return ());
  }

  GraphNode create_test_node () { return { 1, *transport_, *processable_ }; }

  std::unique_ptr<MockTransport>   transport_;
  std::unique_ptr<MockProcessable> processable_;
};

TEST_F (GraphNodeTest, Construction)
{
  EXPECT_CALL (*processable_, get_node_name ()).WillOnce (Return (u8"test_node"));

  auto node = create_test_node ();
  EXPECT_EQ (node.get_processable ().get_node_name (), "test_node");
  EXPECT_TRUE (node.childnodes_.empty ());
  EXPECT_FALSE (node.terminal_);
  EXPECT_FALSE (node.initial_);
}

TEST_F (GraphNodeTest, ProcessingBasics)
{
  EXPECT_CALL (*processable_, process_block (_, _)).Times (Exactly (1));

  auto                  node = create_test_node ();
  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);
}

TEST_F (GraphNodeTest, LatencyHandling)
{
  EXPECT_CALL (*processable_, get_single_playback_latency ())
    .WillRepeatedly (Return (512));

  auto node = create_test_node ();
  EXPECT_EQ (node.get_single_playback_latency (), 512);

  node.set_route_playback_latency (1024);
  EXPECT_EQ (node.route_playback_latency_, 1024);
}

TEST_F (GraphNodeTest, NodeConnections)
{
  auto node1 = create_test_node ();
  auto node2 = create_test_node ();

  node1.connect_to (node2);

  EXPECT_EQ (node1.childnodes_.size (), 1);
  EXPECT_FALSE (node1.terminal_);
  EXPECT_FALSE (node2.initial_);
  EXPECT_EQ (node2.init_refcount_, 1);
  EXPECT_EQ (node2.refcount_, 1);
}

TEST_F (GraphNodeTest, SkipProcessing)
{
  EXPECT_CALL (*processable_, process_block (_, _)).Times (0);

  auto node = create_test_node ();
  node.set_skip_processing (true);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);
}

TEST_F (GraphNodeTest, ProcessingWithTransport)
{
  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (Return (ITransport::PlayState::Rolling));
  EXPECT_CALL (*transport_, get_playhead_position ())
    .WillOnce (Return (Position{}));
  EXPECT_CALL (*transport_, position_add_frames (_, _)).Times (1);
  EXPECT_CALL (*processable_, process_block (_, _)).Times (1);

  auto                  node = create_test_node ();
  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);
}

TEST_F (GraphNodeTest, LoopPointProcessing)
{
  EXPECT_CALL (*transport_, get_loop_enabled ()).WillRepeatedly (Return (true));
  EXPECT_CALL (*transport_, is_loop_point_met (_, _))
    .WillOnce (Return (128))
    .WillOnce (Return (0));
  EXPECT_CALL (*processable_, process_block (_, _)).Times (2);

  auto                  node = create_test_node ();
  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);
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

  EXPECT_EQ (node1.childnodes_.size (), 2);
  EXPECT_EQ (node2.childnodes_.size (), 1);
  EXPECT_EQ (node3.childnodes_.size (), 0);
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

  node3.set_route_playback_latency (1000);

  EXPECT_EQ (node3.route_playback_latency_, 1000);
  EXPECT_EQ (node2.route_playback_latency_, 1000);
  EXPECT_EQ (node1.route_playback_latency_, 1000);
}

TEST_F (GraphNodeTest, ProcessingWithLoopAndLatency)
{
  EXPECT_CALL (*transport_, get_loop_enabled ()).WillRepeatedly (Return (true));
  EXPECT_CALL (*processable_, get_single_playback_latency ())
    .WillRepeatedly (Return (128));
  EXPECT_CALL (*transport_, is_loop_point_met (_, _))
    .WillOnce (Return (64))
    .WillOnce (Return (0));
  EXPECT_CALL (*processable_, process_block (_, _)).Times (2);

  auto node = create_test_node ();
  node.set_route_playback_latency (256);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 64);
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
  EXPECT_EQ (node1.childnodes_.size (), 2);
  EXPECT_EQ (node2.childnodes_.size (), 1);
  EXPECT_EQ (node3.childnodes_.size (), 1);
  EXPECT_EQ (node4.childnodes_.size (), 0);

  // Verify reference counts
  EXPECT_EQ (node1.init_refcount_, 0);
  EXPECT_EQ (node2.init_refcount_, 1);
  EXPECT_EQ (node3.init_refcount_, 1);
  EXPECT_EQ (node4.init_refcount_, 2);
}

TEST_F (GraphNodeTest, NodeCollection)
{
  GraphNodeCollection collection;

  auto node1 = std::make_unique<GraphNode> (1, *transport_, *processable_);
  auto node2 = std::make_unique<GraphNode> (2, *transport_, *processable_);
  auto node3 = std::make_unique<GraphNode> (3, *transport_, *processable_);

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
    .WillRepeatedly (Return (128));

  auto node1 = std::make_unique<GraphNode> (1, *transport_, *processable_);
  auto node2 = std::make_unique<GraphNode> (2, *transport_, *processable_);

  node1->connect_to (*node2);
  collection.graph_nodes_.push_back (std::move (node1));
  collection.graph_nodes_.push_back (std::move (node2));

  collection.finalize_nodes ();

  EXPECT_EQ (collection.get_max_route_playback_latency (), 128);
}

TEST_F (GraphNodeTest, ProcessableSearch)
{
  GraphNodeCollection collection;

  auto  node = std::make_unique<GraphNode> (1, *transport_, *processable_);
  auto &node_ref = *node;
  collection.graph_nodes_.push_back (std::move (node));

  auto * found = collection.find_node_for_processable (*processable_);
  EXPECT_EQ (found, &node_ref);

  MockProcessable other_processable;
  auto * not_found = collection.find_node_for_processable (other_processable);
  EXPECT_EQ (not_found, nullptr);
}

TEST_F (GraphNodeTest, ProcessBlockReceivesInputs)
{
  // Create two separate mocks for processables
  auto mock_processable1 = std::make_unique<MockProcessable> ();
  auto mock_processable2 = std::make_unique<MockProcessable> ();

  // Set up default expectations for the new mocks
  ON_CALL (*mock_processable1, get_node_name ())
    .WillByDefault (Return (u8"node1"));
  ON_CALL (*mock_processable2, get_node_name ())
    .WillByDefault (Return (u8"node2"));
  ON_CALL (*mock_processable1, get_single_playback_latency ())
    .WillByDefault (Return (0));
  ON_CALL (*mock_processable2, get_single_playback_latency ())
    .WillByDefault (Return (0));

  // Use the same transport for both (from fixture)
  GraphNode node1 (1, *transport_, *mock_processable1);
  GraphNode node2 (2, *transport_, *mock_processable2);

  node1.connect_to (node2);

  // We don't expect node1 to be processed in this test
  EXPECT_CALL (*mock_processable1, process_block (_, _)).Times (0);

  // Verify node2 receives node1 as input
  EXPECT_CALL (*mock_processable2, process_block (_, _))
    .WillOnce ([&] (auto, auto inputs) {
      ASSERT_EQ (inputs.size (), 1);
      EXPECT_EQ (inputs[0], mock_processable1.get ());
    });

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node2.process (time_info, 0);
}

TEST_F (GraphNodeTest, ProcessBlockReceivesMultipleInputs)
{
  auto mock_processable1 = std::make_unique<MockProcessable> ();
  auto mock_processable2 = std::make_unique<MockProcessable> ();
  auto mock_processable3 = std::make_unique<MockProcessable> ();

  ON_CALL (*mock_processable1, get_node_name ())
    .WillByDefault (Return (u8"node1"));
  ON_CALL (*mock_processable2, get_node_name ())
    .WillByDefault (Return (u8"node2"));
  ON_CALL (*mock_processable3, get_node_name ())
    .WillByDefault (Return (u8"node3"));
  ON_CALL (*mock_processable1, get_single_playback_latency ())
    .WillByDefault (Return (0));
  ON_CALL (*mock_processable2, get_single_playback_latency ())
    .WillByDefault (Return (0));
  ON_CALL (*mock_processable3, get_single_playback_latency ())
    .WillByDefault (Return (0));

  GraphNode node1 (1, *transport_, *mock_processable1);
  GraphNode node2 (2, *transport_, *mock_processable2);
  GraphNode node3 (3, *transport_, *mock_processable3);

  node1.connect_to (node3);
  node2.connect_to (node3);

  // Only node3 is processed
  EXPECT_CALL (*mock_processable1, process_block (_, _)).Times (0);
  EXPECT_CALL (*mock_processable2, process_block (_, _)).Times (0);

  EXPECT_CALL (*mock_processable3, process_block (_, _))
    .WillOnce ([&] (auto, auto inputs) {
      ASSERT_EQ (inputs.size (), 2);
      // The inputs should be the two parents: node1 and node2
      // We don't know the order, so check both
      EXPECT_TRUE (
        (inputs[0] == mock_processable1.get () && inputs[1] == mock_processable2.get ())
        || (inputs[0] == mock_processable2.get () && inputs[1] == mock_processable1.get ()));
    });

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node3.process (time_info, 0);
}

TEST_F (GraphNodeTest, ProcessBlockReceivesNoInputs)
{
  auto mock_processable = std::make_unique<MockProcessable> ();

  ON_CALL (*mock_processable, get_node_name ()).WillByDefault (Return (u8"node"));
  ON_CALL (*mock_processable, get_single_playback_latency ())
    .WillByDefault (Return (0));

  GraphNode node (1, *transport_, *mock_processable);

  EXPECT_CALL (*mock_processable, process_block (_, _))
    .WillOnce ([&] (auto, auto inputs) { EXPECT_EQ (inputs.size (), 0); });

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);
}

class GraphNodeProcessBlockInputsTest
    : public GraphNodeTest,
      public testing::WithParamInterface<int>
{
};

TEST_P (GraphNodeProcessBlockInputsTest, VerifyInputs)
{
  const int input_count = GetParam ();

  // Create mock processables for parent nodes
  std::vector<std::unique_ptr<MockProcessable>> mock_parents;
  for (int i = 0; i < input_count; i++)
    {
      auto mp = std::make_unique<MockProcessable> ();
      ON_CALL (*mp, get_node_name ())
        .WillByDefault (Return (
          utils::Utf8String::from_utf8_encoded_string (
            "parent_node_" + std::to_string (i))));
      ON_CALL (*mp, get_single_playback_latency ()).WillByDefault (Return (0));
      mock_parents.push_back (std::move (mp));
    }

  // Create target node
  auto target_mock = std::make_unique<MockProcessable> ();
  ON_CALL (*target_mock, get_node_name ())
    .WillByDefault (Return (u8"target_node"));
  ON_CALL (*target_mock, get_single_playback_latency ())
    .WillByDefault (Return (0));
  GraphNode target_node (input_count + 1, *transport_, *target_mock);

  // Create parent nodes and connect to target
  for (int i = 0; i < input_count; i++)
    {
      GraphNode parent_node (i + 1, *transport_, *mock_parents[i]);
      parent_node.connect_to (target_node);
    }

  // Set expectation - verify inputs match parents
  EXPECT_CALL (*target_mock, process_block (_, _))
    .WillOnce ([&] (auto, auto inputs) {
      EXPECT_EQ (inputs.size (), input_count);

      // Create set of expected pointers
      std::unordered_set<const IProcessable *> expected;
      for (const auto &mp : mock_parents)
        expected.insert (mp.get ());

      // Verify all inputs are present
      for (const auto * input : inputs)
        EXPECT_TRUE (expected.contains (input));
    });

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  target_node.process (time_info, 0);
}

INSTANTIATE_TEST_SUITE_P (
  InputCases,
  GraphNodeProcessBlockInputsTest,
  testing::Values (0, 1, 2, 128));
} // namespace zrythm::dsp::graph
