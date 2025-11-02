// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <chrono>
#include <thread>

#include "dsp/graph_scheduler.h"
#include "dsp/graph_thread.h"
#include "utils/gtest_wrapper.h"

#include "./graph_helpers.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

namespace zrythm::dsp::graph
{
class GraphSchedulerTest : public ::testing::Test
{
protected:
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;
  using MockTransport = zrythm::dsp::graph_test::MockTransport;

  void SetUp () override
  {
    transport_ = std::make_unique<MockTransport> ();
    processable_ = std::make_unique<MockProcessable> ();
    scheduler_ = std::make_unique<GraphScheduler> (sample_rate_, block_length_);

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return (u8"test_node"));
    ON_CALL (*processable_, get_single_playback_latency ())
      .WillByDefault (Return (0));
    ON_CALL (*processable_, prepare_for_processing (_, _))
      .WillByDefault (Return ());
    ON_CALL (*processable_, release_resources ()).WillByDefault (Return ());
  }

  GraphNodeCollection create_test_collection ()
  {
    GraphNodeCollection collection;
    auto                node1 = std::make_unique<GraphNode> (1, *processable_);
    auto                node2 = std::make_unique<GraphNode> (2, *processable_);
    auto                node3 = std::make_unique<GraphNode> (3, *processable_);

    node1->connect_to (*node2);
    node2->connect_to (*node3);

    collection.graph_nodes_.push_back (std::move (node1));
    collection.graph_nodes_.push_back (std::move (node2));
    collection.graph_nodes_.push_back (std::move (node3));

    collection.finalize_nodes ();
    return collection;
  }

  int                              sample_rate_{ 48000 };
  int                              block_length_{ 256 };
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

  EXPECT_CALL (*processable_, process_block (_, _)).Times (3); // Once for each
                                                               // node

  scheduler_->rechain_from_node_collection (
    std::move (collection), sample_rate_, block_length_);
  scheduler_->start_threads (2);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  scheduler_->run_cycle (time_info, 0, *transport_);

  scheduler_->terminate_threads ();
}

TEST_F (GraphSchedulerTest, MultiThreadedProcessing)
{
  auto collection = create_test_collection ();

  std::atomic<int> process_count{ 0 };
  ON_CALL (*processable_, process_block (_, _)).WillByDefault ([&] (auto, auto &) {
    process_count++;
    std::this_thread::sleep_for (10ms);
  });

  scheduler_->rechain_from_node_collection (
    std::move (collection), sample_rate_, block_length_);
  scheduler_->start_threads (4);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  scheduler_->run_cycle (time_info, 0, *transport_);

  EXPECT_EQ (process_count, 3);
  scheduler_->terminate_threads ();
}

TEST_F (GraphSchedulerTest, NodeTriggeringOrder)
{
  auto collection = create_test_collection ();

  std::vector<int> process_order;
  std::mutex       mutex;
  ON_CALL (*processable_, process_block (_, _)).WillByDefault ([&] (auto, auto &) {
    std::lock_guard<std::mutex> lock (mutex);
    process_order.push_back (process_order.size ());
  });

  scheduler_->rechain_from_node_collection (
    std::move (collection), sample_rate_, block_length_);
  scheduler_->start_threads (1); // Single thread to ensure deterministic order

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = 256;
  scheduler_->run_cycle (time_info, 0, *transport_);

  EXPECT_THAT (process_order, ElementsAre (0, 1, 2));
  scheduler_->terminate_threads ();
}

TEST_F (GraphSchedulerTest, ResourceManagement)
{
  auto collection = create_test_collection ();

  // Expect prepare to be called for each node
  EXPECT_CALL (*processable_, prepare_for_processing (48000, 256)).Times (3);

  // Expect release to be called when scheduler is destroyed
  EXPECT_CALL (*processable_, release_resources ()).Times (3);

  scheduler_->rechain_from_node_collection (
    std::move (collection), sample_rate_, block_length_);
  scheduler_->start_threads (2);

  // Trigger resource release
  scheduler_->terminate_threads ();
}

TEST_F (GraphSchedulerTest, RechainWithLargerBufferSize)
{
  auto collection = create_test_collection ();

  // First chain with initial parameters
  EXPECT_CALL (*processable_, prepare_for_processing (48000, 256)).Times (3);
  scheduler_->rechain_from_node_collection (
    std::move (collection), sample_rate_, block_length_);

  // Create new collection for rechaining
  auto new_collection = create_test_collection ();

  // Rechain with larger buffer size and different sample rate
  const int new_sample_rate = 96000;
  const int new_block_length = 512;

  // Expect prepare to be called with new parameters
  EXPECT_CALL (
    *processable_, prepare_for_processing (new_sample_rate, new_block_length))
    .Times (3);

  scheduler_->rechain_from_node_collection (
    std::move (new_collection), new_sample_rate, new_block_length);

  // Run a processing cycle to ensure everything works
  scheduler_->start_threads (2);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = new_block_length;
  scheduler_->run_cycle (time_info, 0, *transport_);

  scheduler_->terminate_threads ();
}
}
