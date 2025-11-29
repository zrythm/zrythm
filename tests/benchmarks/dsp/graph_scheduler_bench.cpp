// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_node.h"
#include "dsp/graph_scheduler.h"
#include "dsp/graph_thread.h"
#include "utils/dsp.h"

#include "../tests/unit/dsp/graph_helpers.h"
#include <benchmark/benchmark.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace zrythm::dsp::graph;

class GraphSchedulerBenchmark : public benchmark::Fixture
{
protected:
  using MockProcessable = zrythm::dsp::graph_test::MockProcessable;
  using MockTransport = zrythm::dsp::graph_test::MockTransport;

  void SetUp (benchmark::State &) override
  {
    zrythm::utils::LoggerProvider::logger ().get_logger ()->set_level (
      spdlog::level::off);

    // old processable is still used during release_node_resources() so destruct
    // here before assigning processable_ to a new processable
    scheduler_.reset ();
    transport_ = std::make_unique<MockTransport> ();
    scheduler_ =
      std::make_unique<GraphScheduler> (sample_rate_, max_block_length_);

    ON_CALL (*transport_, get_play_state ())
      .WillByDefault (Return (dsp::ITransport::PlayState::Rolling));

    // Set default expectations to suppress warnings
    EXPECT_CALL (*transport_, get_play_state ())
      .Times (::testing::AnyNumber ())
      .WillRepeatedly (Return (dsp::ITransport::PlayState::Rolling));
    EXPECT_CALL (*transport_, get_playhead_position_in_audio_thread ())
      .Times (::testing::AnyNumber ())
      .WillRepeatedly (Return (units::samples (0)));
    EXPECT_CALL (*transport_, loop_enabled ())
      .Times (::testing::AnyNumber ())
      .WillRepeatedly (Return (false));
    EXPECT_CALL (*transport_, get_loop_range_positions ())
      .Times (::testing::AnyNumber ())
      .WillRepeatedly (
        Return (std::make_pair (units::samples (0), units::samples (960))));
  }

  void TearDown (benchmark::State &state) override
  {
    // needed to reset these explicitly here otherwise the mock expectations
    // won't be checked. see:
    // https://stackoverflow.com/questions/10286514/why-is-googlemock-leaking-my-shared-ptr
    scheduler_.reset ();
    transport_.reset ();
    processables_.clear ();
  }

  // Helper to create a configured processable
  std::unique_ptr<MockProcessable> create_processable ()
  {
    auto proc = std::make_unique<MockProcessable> ();

    ON_CALL (*proc, get_node_name ())
      .WillByDefault (Return (utils::Utf8String (u8"bench_node")));
    ON_CALL (*proc, get_single_playback_latency ()).WillByDefault (Return (0));
    ON_CALL (*proc, prepare_for_processing (_, _, _)).WillByDefault (Return ());
    ON_CALL (*proc, release_resources ()).WillByDefault (Return ());

    // Simulate actual processing work
    ON_CALL (*proc, process_block (_, _)).WillByDefault ([] (auto, auto &) {
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
    EXPECT_CALL (*proc, get_node_name ()).Times (AnyNumber ());
    EXPECT_CALL (*proc, get_single_playback_latency ()).Times (AnyNumber ());
    EXPECT_CALL (*proc, process_block (_, _)).Times (AnyNumber ());
    EXPECT_CALL (*proc, prepare_for_processing (_, _, _)).Times (AnyNumber ());
    EXPECT_CALL (*proc, release_resources ()).Times (AnyNumber ());

    return proc;
  }

  GraphNodeCollection create_linear_chain (size_t num_nodes)
  {
    GraphNodeCollection                     collection;
    std::vector<std::unique_ptr<GraphNode>> nodes;

    for (size_t i = 0; i < num_nodes; i++)
      {
        auto proc = create_processable ();
        nodes.push_back (std::make_unique<GraphNode> (i, *proc));
        if (i > 0)
          {
            nodes[i - 1]->connect_to (*nodes[i]);
          }
        // Keep processable alive
        processables_.push_back (std::move (proc));
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

    auto   root_proc = create_processable ();
    auto   root = std::make_unique<GraphNode> (0, *root_proc);
    auto * root_ptr = root.get ();
    collection.graph_nodes_.push_back (std::move (root));
    processables_.push_back (std::move (root_proc));

    for (size_t b = 0; b < branches; b++)
      {
        GraphNode * prev = root_ptr;
        for (size_t n = 0; n < nodes_per_branch; n++)
          {
            auto proc = create_processable ();
            auto node = std::make_unique<GraphNode> (
              collection.graph_nodes_.size (), *proc);
            prev->connect_to (*node);
            prev = node.get ();
            collection.graph_nodes_.push_back (std::move (node));
            processables_.push_back (std::move (proc));
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
        auto proc = create_processable ();
        collection.graph_nodes_.push_back (
          std::make_unique<GraphNode> (i, *proc));
        processables_.push_back (std::move (proc));
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

  units::sample_rate_t            sample_rate_{ units::sample_rate (48000) };
  size_t                          max_block_length_{ 1024 };
  std::unique_ptr<MockTransport>  transport_;
  std::unique_ptr<GraphScheduler> scheduler_;
  std::vector<std::unique_ptr<MockProcessable>>     processables_;
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

  scheduler_->rechain_from_node_collection (
    create_linear_chain (num_nodes), sample_rate_, max_block_length_);
  scheduler_->start_threads (num_threads);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = block_size;

  for (auto _ : state)
    {
      scheduler_->run_cycle (time_info, 0, *transport_);
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
    create_split_chain (num_branches, nodes_per_branch), sample_rate_,
    max_block_length_);
  scheduler_->start_threads (num_threads);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = block_size;

  for (auto _ : state)
    {
      scheduler_->run_cycle (time_info, 0, *transport_);
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
    create_complex_graph (num_nodes, connectivity), sample_rate_,
    max_block_length_);
  scheduler_->start_threads (num_threads);

  EngineProcessTimeInfo time_info{};
  time_info.nframes_ = block_size;

  for (auto _ : state)
    {
      scheduler_->run_cycle (time_info, 0, *transport_);
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
