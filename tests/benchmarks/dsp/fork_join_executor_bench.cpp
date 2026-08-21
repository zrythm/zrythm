// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdint>

#include "dsp/fork_join_executor.h"

#include <benchmark/benchmark.h>

namespace zrythm::dsp::graph
{

/**
 * @brief Per-job overhead and work scaling of the ForkJoinExecutor itself.
 *
 * The worker count is taken from the first range argument. Note that the
 * submitting thread also executes tasks (helping), so effective parallelism
 * is workers + 1. Passing 0 workers measures the helping-only path, which
 * approximates a serial execution plus submission overhead.
 */
class ForkJoinExecutorBenchmark : public benchmark::Fixture
{
public:
  void SetUp (benchmark::State &state) override
  {
    executor_.start (static_cast<int> (state.range (0)));
  }

  void TearDown (benchmark::State &) override { executor_.stop (); }

protected:
  ForkJoinExecutor executor_;
};

/** Task that does nothing, for measuring pure submission overhead. */
static void
noop_task (void *, std::uint32_t)
{
}

/**
 * @brief Burns CPU with a dependent floating-point chain (cannot be
 * vectorized or optimized out). One iteration costs roughly 1-2 ns.
 *
 * noinline: keeps the caller from vectorizing across multiple independent
 * burn chains (the serial baseline calls this 32 times in a row), so that
 * baseline and executor runs perform identical per-task work.
 */
#if defined(_MSC_VER)
[[msvc::noinline]]
#else
[[gnu::noinline]]
#endif
static float
burn_cpu (std::uint64_t iterations, float seed)
{
  float acc = seed;
  for (std::uint64_t i = 0; i < iterations; ++i)
    {
      acc = acc * 1.000001f + 0.5f;
    }
  return acc;
}

struct BurnContext
{
  std::uint64_t iterations_per_task;
};

static void
burn_task (void * context, std::uint32_t task_index)
{
  const auto &ctx = *static_cast<const BurnContext *> (context);
  const float result =
    burn_cpu (ctx.iterations_per_task, static_cast<float> (task_index) + 1.f);
  benchmark::DoNotOptimize (result);
}

/** Pure submission cost: wake + queue + join per job, near-zero work. */
BENCHMARK_DEFINE_F (ForkJoinExecutorBenchmark, OverheadPerJob)
(benchmark::State &state)
{
  const auto num_tasks = static_cast<std::uint32_t> (state.range (1));
  for (auto _ : state)
    {
      const bool ok = executor_.exec (&noop_task, nullptr, num_tasks);
      benchmark::DoNotOptimize (ok);
    }
}

// Total work per job; tuned so a serial execution takes ~1 ms
static constexpr std::uint64_t kScalingJobTotalIterations = 1'070'000;
static constexpr std::uint32_t kScalingJobNumTasks = 32;

/** Fixed total work split into tasks: expect ~work/(workers+1) + overhead. */
BENCHMARK_DEFINE_F (ForkJoinExecutorBenchmark, Scaling)
(benchmark::State &state)
{
  BurnContext ctx{ kScalingJobTotalIterations / kScalingJobNumTasks };
  for (auto _ : state)
    {
      const bool ok = executor_.exec (&burn_task, &ctx, kScalingJobNumTasks);
      benchmark::DoNotOptimize (ok);
    }
}

/** The same total work as Scaling, executed as a plain serial loop. */
static void
BM_ForkJoinSerialBaseline (benchmark::State &state)
{
  // Loop-carried seed: the work must differ between iterations or the
  // compiler hoists the constant computation out of the timed loop. Each
  // call also needs a distinct seed or identical calls get CSE'd into one.
  float seed = 1.f;
  for (auto _ : state)
    {
      float acc = 0.f;
      for (std::uint32_t t = 0; t < kScalingJobNumTasks; ++t)
        {
          acc += burn_cpu (
            kScalingJobTotalIterations / kScalingJobNumTasks,
            seed + static_cast<float> (t));
        }
      benchmark::DoNotOptimize (acc);
      seed = acc;
    }
}

// Format: {num_workers, num_tasks}
BENCHMARK_REGISTER_F (ForkJoinExecutorBenchmark, OverheadPerJob)
  ->ArgsProduct (
    {
      { 0, 2, 4, 8, 16 },
      { 2, 32 }
});

// Format: {num_workers}
BENCHMARK_REGISTER_F (ForkJoinExecutorBenchmark, Scaling)
  ->Args ({ 0 })
  ->Args ({ 2 })
  ->Args ({ 4 })
  ->Args ({ 8 })
  ->Args ({ 16 })
  ->Unit (benchmark::kMillisecond);

BENCHMARK (BM_ForkJoinSerialBaseline)->Unit (benchmark::kMillisecond);

} // namespace zrythm::dsp::graph
