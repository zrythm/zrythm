#include "dsp/graph_node.h"
#include "dsp/itransport.h"
#include "utils/gtest_wrapper.h"

#include <gmock/gmock.h>

using namespace testing;

namespace zrythm::dsp
{

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

    // Default expectations
    ON_CALL (*transport_, get_play_state ())
      .WillByDefault (Return (ITransport::PlayState::Paused));
    ON_CALL (*transport_, get_playhead_position ())
      .WillByDefault (Return (Position{}));
    ON_CALL (*transport_, get_loop_enabled ()).WillByDefault (Return (false));
    ON_CALL (*transport_, get_loop_range_positions ())
      .WillByDefault (Return (
        std::make_pair (Position{}, Position{ 1920.0, 22.675736961451247 })));
  }

  GraphNode create_test_node ()
  {
    return {
      1, [] () { return "test_node"; }, *transport_,
      [] (EngineProcessTimeInfo) { /* no-op */ },
      [] () -> nframes_t { return 0; }
    };
  }

  std::unique_ptr<MockTransport> transport_;
};

TEST_F (GraphNodeTest, Construction)
{
  auto node = create_test_node ();
  EXPECT_EQ (node.get_name (), "test_node");
  EXPECT_EQ (node.get_single_playback_latency (), 0);
  EXPECT_TRUE (node.childnodes_.empty ());
  EXPECT_FALSE (node.terminal_);
  EXPECT_FALSE (node.initial_);
}

TEST_F (GraphNodeTest, ProcessingBasics)
{
  bool      processed = false;
  GraphNode node (
    1, [] () { return "test_node"; }, *transport_,
    [&processed] (EngineProcessTimeInfo) { processed = true; });

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);

  EXPECT_TRUE (processed);
}

TEST_F (GraphNodeTest, LatencyHandling)
{
  GraphNode node (
    1, [] () { return "test_node"; }, *transport_,
    [] (EngineProcessTimeInfo) {}, [] () -> nframes_t { return 512; });

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
  bool      processed = false;
  GraphNode node (
    1, [] () { return "test_node"; }, *transport_,
    [&processed] (EngineProcessTimeInfo) { processed = true; });

  node.set_skip_processing (true);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);

  EXPECT_FALSE (processed);
}

TEST_F (GraphNodeTest, ProcessingWithTransport)
{
  bool      processed = false;
  GraphNode node (
    1, [] () { return "test_node"; }, *transport_,
    [&processed] (EngineProcessTimeInfo) { processed = true; });

  EXPECT_CALL (*transport_, get_play_state ())
    .WillOnce (Return (ITransport::PlayState::Rolling));
  EXPECT_CALL (*transport_, get_playhead_position ())
    .WillOnce (Return (Position{}));
  EXPECT_CALL (*transport_, position_add_frames (_, _)).Times (1);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);

  EXPECT_TRUE (processed);
}

TEST_F (GraphNodeTest, LoopPointProcessing)
{
  bool      processed = false;
  GraphNode node (
    1, [] () { return "test_node"; }, *transport_,
    [&processed] (EngineProcessTimeInfo) { processed = true; });

  EXPECT_CALL (*transport_, get_loop_enabled ()).WillRepeatedly (Return (true));
  EXPECT_CALL (*transport_, is_loop_point_met (_, _))
    .WillOnce (Return (128))
    .WillOnce (Return (0));

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  node.process (time_info, 0);

  EXPECT_TRUE (processed);
}

} // namespace zrythm::dsp
