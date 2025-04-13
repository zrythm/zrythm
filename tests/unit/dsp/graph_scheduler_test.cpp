// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <chrono>
#include <thread>

#include "dsp/graph_scheduler.h"
#include "dsp/graph_thread.h"
#include "utils/gtest_wrapper.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

namespace zrythm::dsp
{

class MockProcessable : public IProcessable
{
public:
  MOCK_METHOD (std::string, get_node_name, (), (const, override));
  MOCK_METHOD (nframes_t, get_single_playback_latency, (), (const, override));
  MOCK_METHOD (void, process_block, (EngineProcessTimeInfo), (override));
  MOCK_METHOD (void, clear_external_buffer, (), (override));
  MOCK_METHOD (
    bool,
    needs_external_buffer_clear_on_early_return,
    (),
    (const, override));
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
    (signed_frame_t, nframes_t),
    (const, override));
};

class GraphSchedulerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    transport_ = std::make_unique<MockTransport> ();
    processable_ = std::make_unique<MockProcessable> ();
    scheduler_ = std::make_unique<GraphScheduler> ();

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return ("test_node"));
    ON_CALL (*processable_, get_single_playback_latency ())
      .WillByDefault (Return (0));
  }

  GraphNodeCollection create_test_collection ()
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

    collection.finalize_nodes ();
    return collection;
  }

  std::unique_ptr<MockTransport>   transport_;
  std::unique_ptr<MockProcessable> processable_;
  std::unique_ptr<GraphScheduler>  scheduler_;
};

TEST_F (GraphSchedulerTest, ThreadStartAndTermination)
{
  scheduler_->start_threads (2);
  EXPECT_FALSE (scheduler_->contains_thread (current_thread_id.get ()));
  scheduler_->terminate_threads ();
}

TEST_F (GraphSchedulerTest, ProcessingCycle)
{
  auto collection = create_test_collection ();

  EXPECT_CALL (*processable_, process_block (_)).Times (3); // Once for each node

  scheduler_->rechain_from_node_collection (std::move (collection));
  scheduler_->start_threads (2);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  scheduler_->run_cycle (time_info, 0);

  scheduler_->terminate_threads ();
}

// FIXME: ON_CALL crashes for some reason
#if 0
TEST_F (GraphSchedulerTest, ExternalBufferClearing)
{
  auto collection = create_test_collection ();

  ON_CALL (*processable_, needs_external_buffer_clear_on_early_return ())
    .WillByDefault (Return (true));
  EXPECT_CALL (*processable_, clear_external_buffer ()).Times (AtLeast (1));

  scheduler_->rechain_from_node_collection (std::move (collection));

  scheduler_->start_threads (2);
  std::this_thread::sleep_for (10ms);

  scheduler_->clear_external_output_buffers ();

  scheduler_->terminate_threads ();
}
#endif

TEST_F (GraphSchedulerTest, MultiThreadedProcessing)
{
  auto collection = create_test_collection ();

  std::atomic<int> process_count{ 0 };
  ON_CALL (*processable_, process_block (_)).WillByDefault ([&] (auto) {
    process_count++;
    std::this_thread::sleep_for (10ms);
  });

  scheduler_->rechain_from_node_collection (std::move (collection));
  scheduler_->start_threads (4);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  scheduler_->run_cycle (time_info, 0);

  EXPECT_EQ (process_count, 3);
  scheduler_->terminate_threads ();
}

TEST_F (GraphSchedulerTest, NodeTriggeringOrder)
{
  auto collection = create_test_collection ();

  std::vector<int> process_order;
  std::mutex       mutex;
  ON_CALL (*processable_, process_block (_)).WillByDefault ([&] (auto) {
    std::lock_guard<std::mutex> lock (mutex);
    process_order.push_back (process_order.size ());
  });

  scheduler_->rechain_from_node_collection (std::move (collection));
  scheduler_->start_threads (1); // Single thread to ensure deterministic order

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  scheduler_->run_cycle (time_info, 0);

  EXPECT_THAT (process_order, ElementsAre (0, 1, 2));
  scheduler_->terminate_threads ();
}

}
