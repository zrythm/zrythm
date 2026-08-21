// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <atomic>
#include <latch>
#include <ranges>
#include <semaphore>
#include <thread>
#include <vector>

#include "dsp/fork_join_executor.h"

#include <gtest/gtest.h>

namespace zrythm::dsp::graph
{

class ForkJoinExecutorTest : public ::testing::Test
{
protected:
  static constexpr std::uint32_t kMaxTasks = 256;

  struct TaskCounter
  {
    std::array<std::atomic<std::uint32_t>, kMaxTasks> counts_{};
    std::atomic<std::uint32_t>                        total_{ 0 };
  };

  static void counting_task (void * context, std::uint32_t task_index) noexcept
  {
    auto * counter = static_cast<TaskCounter *> (context);
    counter->counts_[task_index].fetch_add (1, std::memory_order_relaxed);
    counter->total_.fetch_add (1, std::memory_order_relaxed);
  }
};

TEST_F (ForkJoinExecutorTest, ZeroTasksReturnTrueImmediately)
{
  ForkJoinExecutor executor;
  TaskCounter      counter;
  EXPECT_TRUE (executor.exec (counting_task, &counter, 0));
  EXPECT_EQ (counter.total_.load (), 0);
}

TEST_F (ForkJoinExecutorTest, SingleTaskRunsInlineOnCallingThread)
{
  ForkJoinExecutor executor;

  std::thread::id calling_thread = std::this_thread::get_id ();
  std::thread::id task_thread;
  const auto      task = [] (void * context, std::uint32_t) noexcept {
    *static_cast<std::thread::id *> (context) = std::this_thread::get_id ();
  };

  EXPECT_TRUE (executor.exec (task, &task_thread, 1));
  EXPECT_EQ (task_thread, calling_thread);

  // The inline path must also be taken with started workers
  executor.start (2);
  task_thread = {};
  EXPECT_TRUE (executor.exec (task, &task_thread, 1));
  EXPECT_EQ (task_thread, calling_thread);
}

TEST_F (ForkJoinExecutorTest, MultipleTasksAreRejectedWhenNotStarted)
{
  ForkJoinExecutor executor;
  TaskCounter      counter;
  EXPECT_FALSE (executor.exec (counting_task, &counter, 2));
  EXPECT_EQ (counter.total_.load (), 0);
}

TEST_F (ForkJoinExecutorTest, AllTasksRunExactlyOnce)
{
  ForkJoinExecutor executor;
  executor.start (2);

  TaskCounter counter;
  EXPECT_TRUE (executor.exec (counting_task, &counter, 100));
  EXPECT_EQ (counter.total_.load (), 100);
  for (const auto i : std::views::iota (0u, 100u))
    {
      EXPECT_EQ (counter.counts_[i].load (), 1) << "task " << i;
    }
}

TEST_F (ForkJoinExecutorTest, MoreTasksThanWorkersStillComplete)
{
  ForkJoinExecutor executor;
  executor.start (1);

  TaskCounter counter;
  EXPECT_TRUE (executor.exec (counting_task, &counter, 64));
  EXPECT_EQ (counter.total_.load (), 64);
}

TEST_F (ForkJoinExecutorTest, SubmitterExecutesWholeJobWithoutWorkers)
{
  ForkJoinExecutor executor;
  executor.start (0);

  std::thread::id calling_thread = std::this_thread::get_id ();
  std::thread::id task_thread;
  const auto      task = [] (void * context, std::uint32_t) noexcept {
    auto * thread_id = static_cast<std::thread::id *> (context);
    *thread_id = std::this_thread::get_id ();
  };

  // With no workers, the submitter drains its own job by helping
  EXPECT_TRUE (executor.exec (task, &task_thread, 8));
  EXPECT_EQ (task_thread, calling_thread);
}

TEST_F (ForkJoinExecutorTest, ConcurrentSubmittersAllComplete)
{
  ForkJoinExecutor executor;
  executor.start (4);

  static constexpr int           kSubmitters = 4;
  static constexpr int           kRoundsPerSubmitter = 100;
  static constexpr std::uint32_t kTasksPerRound = 8;

  std::array<TaskCounter, kSubmitters> counters;
  std::atomic<int>                     failed_rounds{ 0 };

  {
    std::vector<std::jthread> threads;
    for (const auto i : std::views::iota (0, kSubmitters))
      {
        threads.emplace_back ([&executor, &counters, &failed_rounds, i] {
          for (
            [[maybe_unused]] const auto round :
            std::views::iota (0, kRoundsPerSubmitter))
            {
              if (!executor.exec (counting_task, &counters[i], kTasksPerRound))
                {
                  failed_rounds.fetch_add (1, std::memory_order_relaxed);
                }
            }
        });
      }
  }

  EXPECT_EQ (failed_rounds.load (), 0);
  for (const auto &counter : counters)
    {
      EXPECT_EQ (counter.total_.load (), kRoundsPerSubmitter * kTasksPerRound);
    }
}

TEST_F (ForkJoinExecutorTest, NestedSubmissionIsRejected)
{
  ForkJoinExecutor executor;
  executor.start (2);

  struct Context
  {
    ForkJoinExecutor * executor_;
    std::atomic<int>   nested_rejected_{ 0 };
    std::atomic<int>   nested_executed_{ 0 };
    TaskCounter        outer_counter_;
  } context{
    .executor_ = &executor,
    .nested_rejected_ = {},
    .nested_executed_ = {},
    .outer_counter_ = {},
  };

  const auto outer_task = [] (void * ctx, std::uint32_t) noexcept {
    auto *      nested_context = static_cast<Context *> (ctx);
    TaskCounter inner_counter;
    if (nested_context->executor_->exec (counting_task, &inner_counter, 2))
      {
        nested_context->nested_executed_.fetch_add (
          1, std::memory_order_relaxed);
      }
    else
      {
        nested_context->nested_rejected_.fetch_add (
          1, std::memory_order_relaxed);
      }
    nested_context->outer_counter_.total_.fetch_add (
      1, std::memory_order_relaxed);
  };

  EXPECT_TRUE (executor.exec (outer_task, &context, 2));
  EXPECT_EQ (context.outer_counter_.total_.load (), 2);
  EXPECT_EQ (context.nested_executed_.load (), 0);
  EXPECT_EQ (context.nested_rejected_.load (), 2);
}

TEST_F (ForkJoinExecutorTest, StopAndRestart)
{
  ForkJoinExecutor executor;
  executor.start (2);
  executor.stop ();
  EXPECT_FALSE (executor.is_started ());

  TaskCounter counter;
  EXPECT_FALSE (executor.exec (counting_task, &counter, 2));

  executor.start (2);
  EXPECT_TRUE (executor.exec (counting_task, &counter, 8));
  EXPECT_EQ (counter.total_.load (), 8);
}

TEST_F (ForkJoinExecutorTest, DestructorStopsWorkersImplicitly)
{
  // Must not hang: the destructor stops and joins idle workers
  ForkJoinExecutor executor;
  executor.start (2);
}

TEST_F (ForkJoinExecutorTest, NestedSubmissionFromInlineTaskIsRejected)
{
  ForkJoinExecutor executor;
  executor.start (2);

  struct Context
  {
    ForkJoinExecutor * executor_;
    bool               nested_rejected_ = false;
  } context{ .executor_ = &executor };

  // A task running inline (1-task job) is still a pool task: nested exec()
  // from within it is a contract violation and must be rejected
  const auto outer_task = [] (void * ctx, std::uint32_t) noexcept {
    auto *      task_context = static_cast<Context *> (ctx);
    TaskCounter inner_counter;
    task_context->nested_rejected_ =
      !task_context->executor_->exec (counting_task, &inner_counter, 2);
  };

  EXPECT_TRUE (executor.exec (outer_task, &context, 1));
  EXPECT_TRUE (context.nested_rejected_);
}

TEST_F (ForkJoinExecutorTest, AllSlotsInUseRejectNewJobs)
{
  ForkJoinExecutor executor;
  // No workers: each submitter executes its own tasks. The first task of
  // each job blocks until the gate opens, so each submitter thread stays
  // inside exec() and keeps holding its job slot
  executor.start (0);

  struct GateContext
  {
    std::latch                entered_;
    std::counting_semaphore<> gate_{ 0 };
  };
  GateContext gate{ .entered_{ ForkJoinExecutor::kMaxConcurrentJobs } };

  const auto gated_task = [] (void * ctx, std::uint32_t task_index) noexcept {
    auto * context = static_cast<GateContext *> (ctx);
    if (task_index == 0)
      {
        context->entered_.count_down ();
        context->gate_.acquire ();
      }
  };

  std::array<std::atomic<bool>, ForkJoinExecutor::kMaxConcurrentJobs> succeeded{};
  {
    std::vector<std::jthread> submitters;
    for (
      const auto i : std::views::iota (0u, ForkJoinExecutor::kMaxConcurrentJobs))
      {
        submitters.emplace_back ([&executor, &gated_task, &gate, &succeeded, i] {
          succeeded[i].store (
            executor.exec (gated_task, &gate, 2), std::memory_order_relaxed);
        });
      }

    // Wait until every submitter holds a slot, then the next submission has
    // none left and must be rejected without executing anything
    gate.entered_.wait ();

    TaskCounter rejected_counter;
    EXPECT_FALSE (executor.exec (counting_task, &rejected_counter, 2));
    EXPECT_EQ (rejected_counter.total_.load (), 0);

    gate.gate_.release (ForkJoinExecutor::kMaxConcurrentJobs);
  }

  // All gated jobs completed and their slots were recycled
  for (const auto &ok : succeeded)
    EXPECT_TRUE (ok.load ());
  TaskCounter counter;
  EXPECT_TRUE (executor.exec (counting_task, &counter, 2));
  EXPECT_EQ (counter.total_.load (), 2);
}

TEST_F (ForkJoinExecutorTest, RecycledSlotsNeverLoseOrDuplicateTasks)
{
  ForkJoinExecutor executor;
  executor.start (4);

  // Maximum slot-recycling churn: many submitters, many tiny jobs
  static constexpr int           kSubmitters = 8;
  static constexpr int           kJobsPerSubmitter = 500;
  static constexpr std::uint32_t kTasksPerJob = 4;

  std::atomic<std::uint64_t> total_tasks{ 0 };
  const auto                 task = [] (void * ctx, std::uint32_t) noexcept {
    static_cast<std::atomic<std::uint64_t> *> (ctx)->fetch_add (
      1, std::memory_order_relaxed);
  };

  {
    std::vector<std::jthread> threads;
    for ([[maybe_unused]] const auto i : std::views::iota (0, kSubmitters))
      {
        threads.emplace_back ([&executor, &task, &total_tasks] {
          for (
            [[maybe_unused]] const auto job :
            std::views::iota (0, kJobsPerSubmitter))
            {
              if (!executor.exec (task, &total_tasks, kTasksPerJob))
                {
                  // Rejected submissions must be executed by the caller
                  // (exec() contract)
                  for (
                    const auto t :
                    std::views::iota (std::uint32_t{ 0 }, kTasksPerJob))
                    {
                      task (&total_tasks, t);
                    }
                }
            }
        });
      }
  }

  EXPECT_EQ (
    total_tasks.load (),
    static_cast<std::uint64_t> (kSubmitters) * kJobsPerSubmitter * kTasksPerJob);
}

} // namespace zrythm::dsp::graph
