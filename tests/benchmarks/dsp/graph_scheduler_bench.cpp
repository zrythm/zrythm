// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_node.h"
#include "dsp/graph_scheduler.h"
#include "dsp/graph_thread.h"
#include "utils/dsp.h"

#include <benchmark/benchmark.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace zrythm::dsp::graph;

class MockProcessable : public IProcessable
{
public:
  MOCK_METHOD (utils::Utf8String, get_node_name, (), (const, override));
  MOCK_METHOD (nframes_t, get_single_playback_latency, (), (const, override));
  MOCK_METHOD (void, process_block, (EngineProcessTimeInfo), (override));
};

class MockTransport : public dsp::ITransport
{
public:
  MOCK_METHOD (
    void,
    position_add_frames,
    (dsp::Position &, signed_frame_t),
    (const, override));
  MOCK_METHOD (
    (std::pair<dsp::Position, dsp::Position>),
    get_loop_range_positions,
    (),
    (const, override));
  MOCK_METHOD (PlayState, get_play_state, (), (const, override));
  MOCK_METHOD (dsp::Position, get_playhead_position, (), (const, override));
  MOCK_METHOD (bool, get_loop_enabled, (), (const, override));
  MOCK_METHOD (
    nframes_t,
    is_loop_point_met,
    (signed_frame_t, nframes_t),
    (const, override));
};

class GraphSchedulerBenchmark : public benchmark::Fixture
{
protected:
  void SetUp (const benchmark::State &) override
  {
    zrythm::utils::LoggerProvider::logger ().get_logger ()->set_level (
      spdlog::level::off);

    transport_ = std::make_unique<MockTransport> ();
    processable_ = std::make_unique<MockProcessable> ();
    scheduler_ = std::make_unique<GraphScheduler> ();

    ON_CALL (*processable_, get_node_name ())
      .WillByDefault (Return (utils::Utf8String (u8"bench_node")));
    ON_CALL (*processable_, get_single_playback_latency ())
      .WillByDefault (Return (0));
    ON_CALL (*transport_, get_play_state ())
      .WillByDefault (Return (dsp::ITransport::PlayState::Rolling));

    // Simulate actual processing work
    ON_CALL (*processable_, process_block (_)).WillByDefault ([] (auto) {
      // Simulate typical DSP operations on a small buffer
      constexpr size_t buffer_size = 64;
      float            buffer[buffer_size];

      // Fill with test signal
      zrythm::utils::float_ranges::fill (buffer, 0.5f, buffer_size);

      // Apply common DSP operations
      zrythm::utils::float_ranges::mul_k2 (buffer, 0.8f, buffer_size);
      zrythm::utils::float_ranges::clip (buffer, -1.0f, 1.0f, buffer_size);

      // Get peak for metering
      float peak = zrythm::utils::float_ranges::abs_max (buffer, buffer_size);
      benchmark::DoNotOptimize (peak);
    });

    // silence GMock warnings
    EXPECT_CALL (*processable_, get_node_name ()).Times (AnyNumber ());
    EXPECT_CALL (*processable_, get_single_playback_latency ())
      .Times (AnyNumber ());
    EXPECT_CALL (*processable_, process_block (_)).Times (AnyNumber ());
    EXPECT_CALL (*transport_, get_play_state ()).Times (AnyNumber ());
    EXPECT_CALL (*transport_, get_playhead_position ()).Times (AnyNumber ());
    EXPECT_CALL (*transport_, position_add_frames (_, _)).Times (AnyNumber ());
    EXPECT_CALL (*transport_, is_loop_point_met (_, _)).Times (AnyNumber ());
  }

  GraphNodeCollection create_linear_chain (size_t num_nodes)
  {
    GraphNodeCollection                     collection;
    std::vector<std::unique_ptr<GraphNode>> nodes;

    for (size_t i = 0; i < num_nodes; i++)
      {
        nodes.push_back (
          std::make_unique<GraphNode> (i, *transport_, *processable_));
        if (i > 0)
          {
            nodes[i - 1]->connect_to (*nodes[i]);
          }
      }

    for (auto &node : nodes)
      {
        collection.graph_nodes_.push_back (std::move (node));
      }
    collection.finalize_nodes ();
    return collection;
  }

  GraphNodeCollection
  create_split_chain (size_t branches, size_t nodes_per_branch)
  {
    GraphNodeCollection collection;

    auto   root = std::make_unique<GraphNode> (0, *transport_, *processable_);
    auto * root_ptr = root.get ();
    collection.graph_nodes_.push_back (std::move (root));

    for (size_t b = 0; b < branches; b++)
      {
        GraphNode * prev = root_ptr;
        for (size_t n = 0; n < nodes_per_branch; n++)
          {
            auto node = std::make_unique<GraphNode> (
              collection.graph_nodes_.size (), *transport_, *processable_);
            prev->connect_to (*node);
            prev = node.get ();
            collection.graph_nodes_.push_back (std::move (node));
          }
      }

    collection.finalize_nodes ();
    return collection;
  }

  GraphNodeCollection
  create_complex_graph (size_t num_nodes, float connectivity_factor)
  {
    GraphNodeCollection collection;

    // Create nodes
    for (size_t i = 0; i < num_nodes; i++)
      {
        collection.graph_nodes_.push_back (
          std::make_unique<GraphNode> (i, *transport_, *processable_));
      }

    // Create layered connections to ensure DAG
    auto layer_size =
      static_cast<size_t> (std::sqrt (static_cast<double> (num_nodes)));
    for (size_t i = 0; i < num_nodes; i++)
      {
        size_t current_layer = i / layer_size;
        size_t max_forward_connections = static_cast<size_t> (
          static_cast<float> (std::min (size_t (5), (num_nodes - i - 1)))
          * connectivity_factor);

        for (size_t j = 0; j < max_forward_connections; j++)
          {
            size_t next_layer_start = (current_layer + 1) * layer_size;
            size_t next_layer_end =
              std::min (next_layer_start + layer_size, num_nodes);
            if (next_layer_start >= num_nodes)
              break;

            size_t target =
              next_layer_start + (rand () % (next_layer_end - next_layer_start));
            collection.graph_nodes_[i]->connect_to (
              *collection.graph_nodes_[target]);
          }
      }

    collection.finalize_nodes ();
    return collection;
  }

  std::unique_ptr<MockTransport>                    transport_;
  std::unique_ptr<MockProcessable>                  processable_;
  std::unique_ptr<GraphScheduler>                   scheduler_;
  static std::shared_ptr<zrythm::utils::TestLogger> logger_;
};

std::shared_ptr<zrythm::utils::TestLogger> GraphSchedulerBenchmark::logger_ =
  std::make_shared<zrythm::utils::TestLogger> ();

BENCHMARK_DEFINE_F (GraphSchedulerBenchmark, LinearChain)
(benchmark::State &state)
{
  const auto num_nodes = state.range (0);
  const auto block_size = state.range (1);
  const auto num_threads = state.range (2);

  scheduler_->rechain_from_node_collection (create_linear_chain (num_nodes));
  scheduler_->start_threads (num_threads);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = block_size;

  for (auto _ : state)
    {
      scheduler_->run_cycle (time_info, 0);
    }

  scheduler_->terminate_threads ();
  state.SetComplexityN (num_nodes);
}

BENCHMARK_DEFINE_F (GraphSchedulerBenchmark, SplitChain)
(benchmark::State &state)
{
  const auto num_branches = state.range (0);
  const auto nodes_per_branch = state.range (1);
  const auto block_size = state.range (2);
  const auto num_threads = state.range (3);

  scheduler_->rechain_from_node_collection (
    create_split_chain (num_branches, nodes_per_branch));
  scheduler_->start_threads (num_threads);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = block_size;

  for (auto _ : state)
    {
      scheduler_->run_cycle (time_info, 0);
    }

  scheduler_->terminate_threads ();
  state.SetComplexityN (num_branches * nodes_per_branch);
}

BENCHMARK_DEFINE_F (GraphSchedulerBenchmark, ComplexGraph)
(benchmark::State &state)
{
  const auto  num_nodes = state.range (0);
  const auto  block_size = state.range (1);
  const auto  num_threads = state.range (2);
  const float connectivity = state.range (3) / 100.0f;

  scheduler_->rechain_from_node_collection (
    create_complex_graph (num_nodes, connectivity));
  scheduler_->start_threads (num_threads);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = block_size;

  for (auto _ : state)
    {
      scheduler_->run_cycle (time_info, 0);
    }

  scheduler_->terminate_threads ();

  state.SetComplexityN (num_nodes);
  // state.counters["Nodes/Thread"] = benchmark::Counter (
  // num_nodes, benchmark::Counter::kAvgThreads, num_threads);
  state.counters["Nodes/Thread"] = double (num_nodes) / double (num_threads);
}

// Register linear chain benchmarks
BENCHMARK_REGISTER_F (GraphSchedulerBenchmark, LinearChain)
  // Format: {num_nodes, block_size, num_threads}
  ->Args ({ 500, 256, 4 })
  ->Args ({ 1000, 256, 4 })
  ->Args ({ 2000, 256, 4 })
  // Thread scaling
  ->Args ({ 1000, 256, 2 })
  ->Args ({ 1000, 256, 6 })
  ->Args ({ 1000, 256, 14 })
  // Block sizes
  ->Args ({ 1000, 64, 4 })
  ->Args ({ 1000, 1024, 4 })
  ->Complexity ();

// Register split chain benchmarks
BENCHMARK_REGISTER_F (GraphSchedulerBenchmark, SplitChain)
  // Format: {num_branches, nodes_per_branch, block_size, num_threads}
  ->Args ({ 5, 100, 256, 4 })  // 500 nodes
  ->Args ({ 10, 100, 256, 4 }) // 1k nodes
  ->Args ({ 20, 100, 256, 4 }) // 2k nodes
  // Thread scaling
  ->Args ({ 10, 100, 256, 2 })
  ->Args ({ 10, 100, 256, 6 })
  ->Args ({ 10, 100, 256, 14 })
  // Block sizes
  ->Args ({ 10, 100, 64, 4 })
  ->Args ({ 10, 100, 1024, 4 })
  // Branch variations
  ->Args ({ 5, 200, 256, 4 }) // Fewer longer branches
  ->Args ({ 40, 25, 256, 4 }) // More shorter branches
  ->Complexity ();

// Register complex graph benchmarks
BENCHMARK_REGISTER_F (GraphSchedulerBenchmark, ComplexGraph)
  // Format: {num_nodes, block_size, num_threads, connectivity_percentage}

  // Test thread scaling
  ->Args ({ 1000, 256, 2, 5 })
  ->Args ({ 1000, 256, 4, 5 })
  ->Args ({ 1000, 256, 6, 5 })
  ->Args ({ 1000, 256, 14, 5 })

  // Test different graph sizes
  ->Args ({ 500, 256, 6, 10 })
  ->Args ({ 1000, 256, 6, 5 })
  ->Args ({ 2000, 256, 6, 2 })

  // Test block sizes
  ->Args ({ 1000, 64, 6, 5 })
  ->Args ({ 1000, 256, 6, 5 })
  ->Args ({ 1000, 1024, 6, 5 })

  // Test connectivity density
  ->Args ({ 1000, 256, 6, 1 })
  ->Args ({ 1000, 256, 6, 5 })
  ->Args ({ 1000, 256, 6, 10 })

  // Extreme cases
  ->Args ({ 2000, 1024, 14, 1 }) // Large but sparse
  ->Args ({ 500, 64, 2, 20 })    // Small but dense
  ->Complexity ();

BENCHMARK_MAIN ();
