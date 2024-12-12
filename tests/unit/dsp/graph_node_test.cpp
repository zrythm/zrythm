#include "dsp/graph_node.h"
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

class GraphNodeTest : public ::testing::Test
{
protected:
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
        std::make_pair (Position{}, Position{ 1920.0, 22.675736961451247 })));

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return ("test_node"));
    ON_CALL (*processable_, get_single_playback_latency ())
      .WillByDefault (Return (0));
  }

  GraphNode create_test_node () { return { 1, *transport_, *processable_ }; }

  std::unique_ptr<MockTransport>   transport_;
  std::unique_ptr<MockProcessable> processable_;
};

TEST_F (GraphNodeTest, Construction)
{
  EXPECT_CALL (*processable_, get_node_name ()).WillOnce (Return ("test_node"));

  auto node = create_test_node ();
  EXPECT_EQ (node.get_processable ().get_node_name (), "test_node");
  EXPECT_TRUE (node.childnodes_.empty ());
  EXPECT_FALSE (node.terminal_);
  EXPECT_FALSE (node.initial_);
}

TEST_F (GraphNodeTest, ProcessingBasics)
{
  EXPECT_CALL (*processable_, process_block (_)).Times (Exactly (1));

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
  EXPECT_CALL (*processable_, process_block (_)).Times (0);

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
  EXPECT_CALL (*processable_, process_block (_)).Times (1);

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
  EXPECT_CALL (*processable_, process_block (_)).Times (2);

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

} // namespace zrythm::dsp
