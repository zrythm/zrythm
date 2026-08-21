// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>
#include <ranges>
#include <thread>

#include "dsp/fork_join_executor.h"
#include "utils/exceptions.h"
#include "utils/logger.h"
#include "utils/raii_utils.h"
#include "utils/threads.h"
#include "utils/tracy.h"

#include <fmt/format.h>

namespace zrythm::dsp::graph
{

using zrythm::utils::exceptions::ZrythmException;

namespace
{
#ifdef __APPLE__
constexpr auto THREAD_STACK_SIZE = 0x80000; // 512kB (same as graph threads)
#else
constexpr auto THREAD_STACK_SIZE = 0x20000; // 128kB (same as graph threads)
#endif

/** Set while a thread is executing a pool task (worker or helping
 * submitter), to reject nested exec() submissions. */
thread_local bool is_executing_pool_task = false;
} // namespace

class ForkJoinExecutor::Worker final : public juce::Thread
{
public:
  Worker (
    int                                 id,
    ForkJoinExecutor                   &executor,
    std::optional<juce::AudioWorkgroup> workgroup)
      : juce::Thread (
          fmt::format ("ForkJoinWorker{}", id),
          utils::rt_worker_stack_size (THREAD_STACK_SIZE)),
        id_ (id), executor_ (executor), workgroup_ (std::move (workgroup))
  {
  }

private:
  void run () override
  {
    {
      juce::WorkgroupToken workgroup_token;
      if (workgroup_.has_value ())
        {
          workgroup_->join (workgroup_token);
        }

      // Group hint 0: no Tracy thread group (pool workers are not graph
      // threads)
      zrythm::utils::set_thread_name (
        fmt::format ("ForkJoin worker {}", id_), 0);

      executor_.worker_thread_loop ();
    }

    // Release handshake for stop(): waitForThreadToExit only sleep-polls
    // (detached pthreads), which establishes no happens-before edge between
    // everything this thread did and the destruction of this Worker
    finished_.store (true, std::memory_order_release);
  }

public:
  std::atomic<bool> finished_{ false };

  /** Set by the owner thread once the thread is confirmed running, so stop()
   * knows there is a thread to join (see stop()). Owner thread only. */
  bool thread_started_ = false;

private:
  int                                 id_;
  ForkJoinExecutor                   &executor_;
  std::optional<juce::AudioWorkgroup> workgroup_;
};

ForkJoinExecutor::ForkJoinExecutor ()
{
  for (auto &slot : job_slots_)
    {
      const bool pushed [[maybe_unused]] = free_slots_.try_push (&slot);
      assert (pushed);
    }
}

ForkJoinExecutor::~ForkJoinExecutor ()
{
  stop ();
}

void
ForkJoinExecutor::run_tasks_from_job (JobSlot &slot) noexcept
{
  for (;;)
    {
      // Acquire: pairs with the release store that resets this counter on
      // job submission, publishing the job's fields to claimers
      const auto task_index =
        slot.next_task_index_.fetch_add (1, std::memory_order_acquire);
      if (task_index >= slot.num_tasks_)
        {
          // No tasks left to claim: remaining ones are running elsewhere
          break;
        }

      {
        ScopedBool in_task{ is_executing_pool_task };
        slot.task_function_ (slot.context_, task_index);
      }

      // Signal completion exactly once per job: the submitter waits on the
      // semaphore exactly once, so the slot's semaphore stays balanced
      if (
        slot.completed_tasks_.fetch_add (1, std::memory_order_acq_rel) + 1
        == slot.num_tasks_)
        {
          slot.completion_sem_.signal ();
        }
    }
}

void
ForkJoinExecutor::drain_work_queue () noexcept
{
  WorkItem item{};
  while (work_queue_.try_pop (item))
    {
      JobSlot &slot = *item.slot_;

      // Join the job before touching it: the CAS expected value embeds the
      // generation we were queued under, so we never count ourselves into a
      // recycled slot (and therefore never have to undo a join)
      bool joined = false;
      auto state = slot.state_.load (std::memory_order_acquire);
      while (generation_of (state) == item.generation_)
        {
          if (
            slot.state_.compare_exchange_weak (
              state, state + 1, std::memory_order_acq_rel,
              std::memory_order_acquire))
            {
              joined = true;
              break;
            }
        }
      if (!joined)
        {
          // Stale entry left over from a completed job (or from before its
          // slot was recycled): nothing to do
          continue;
        }

      // Other participants can claim this job's tasks too: re-queue the
      // entry and wake one more worker before claiming tasks ourselves.
      // Re-queueing is best-effort; on failure we simply run the tasks alone
      if (
        slot.next_task_index_.load (std::memory_order_relaxed) < slot.num_tasks_)
        {
          // Waking a worker is only useful if the entry was re-queued
          if (work_queue_.try_push (item))
            {
              work_available_sem_.signal ();
            }
        }
      run_tasks_from_job (slot);
      slot.state_.fetch_sub (1, std::memory_order_release);
    }
}

void
ForkJoinExecutor::worker_thread_loop () noexcept
{
  for (;;)
    {
      drain_work_queue ();

      if (stop_requested_.load (std::memory_order_acquire)) [[unlikely]]
        {
          return;
        }

      // Fall asleep until work arrives or stop() wakes us (stop() signals
      // once per worker); a spurious wakeup only costs an empty queue drain
      work_available_sem_.wait ();

      if (stop_requested_.load (std::memory_order_acquire)) [[unlikely]]
        {
          return;
        }
    }
}

bool
ForkJoinExecutor::
  exec (TaskFunction task, void * context, std::uint32_t num_tasks) noexcept
{
  assert (task != nullptr);

  if (is_executing_pool_task) [[unlikely]]
    {
      // Nested submission from within a task (contract violation, e.g. the
      // CLAP thread-pool spec forbids request_exec from pool threads)
      z_warning ("fork-join exec() called from within a pool task - rejecting");
      return false;
    }

  if (num_tasks == 0)
    return true;

  if (num_tasks == 1)
    {
      // Same contract as tasks claimed from a queued job: nested exec() is
      // rejected
      ScopedBool in_task{ is_executing_pool_task };
      task (context, 0);
      return true;
    }

  if (!started_.load (std::memory_order_acquire))
    {
      // No workers to execute the tasks: the caller must run them serially
      return false;
    }

  JobSlot * slot = nullptr;
  if (!free_slots_.try_pop (slot))
    {
      // All job slots are in use: the caller must run the tasks serially
      return false;
    }

  // We own the slot exclusively (from the free list). Initialize the job,
  // then publish the new generation together with our own participation in a
  // single release store
  const auto generation =
    generation_of (slot->state_.load (std::memory_order_relaxed)) + 1;
  slot->completed_tasks_.store (0, std::memory_order_relaxed);
  slot->task_function_ = task;
  slot->context_ = context;
  slot->num_tasks_ = num_tasks;
  slot->next_task_index_.store (0, std::memory_order_relaxed);
  slot->state_.store (
    (static_cast<std::uint64_t> (generation) << 32) | 1u,
    std::memory_order_release);

  if (!work_queue_.try_push ({ slot, generation }))
    {
      return_slot_to_free_list (*slot);
      return false;
    }
  work_available_sem_.signal ();

  // Help drain the queue (this covers our own job: whoever pops its entry
  // claims its tasks cooperatively), then wait for the completion signal.
  // The semaphore is signaled exactly once per job and waited on exactly
  // once here, so it stays balanced when the slot is recycled
  drain_work_queue ();
  slot->completion_sem_.wait ();

  // Close the job to new participants (bump the generation, keeping the
  // participant count), then wait until late helpers have left the claim
  // loop before recycling the slot, so no thread can observe a
  // half-initialized next job
  slot->state_.fetch_add (std::uint64_t{ 1 } << 32, std::memory_order_acq_rel);
  while (participants_of (slot->state_.load (std::memory_order_acquire)) != 1)
    {
      std::this_thread::yield ();
    }

  return_slot_to_free_list (*slot);
  return true;
}

void
ForkJoinExecutor::return_slot_to_free_list (JobSlot &slot) noexcept
{
  while (!free_slots_.try_push (&slot))
    {
      std::this_thread::yield ();
    }
}

void
ForkJoinExecutor::start (
  const int                                    num_workers,
  std::optional<juce::Thread::RealtimeOptions> realtime_options,
  std::optional<juce::AudioWorkgroup>          workgroup)
{
  if (is_started ())
    return;

  assert (num_workers >= 0);
  stop_requested_.store (false, std::memory_order_release);

  // Initialize JUCE's current-thread holder from this thread before any
  // worker starts: its lazy static initialization (getCurrentThreadHolder()
  // in juce_Thread.cpp) is guarded by a SpinLock that TSan does not
  // recognize, so concurrent first-use by two workers gets flagged (and is
  // formally a race on the holder's construction)
  (void) juce::Thread::getCurrentThread ();

  try
    {
      for (const auto i : std::views::iota (0, num_workers))
        {
          auto &worker = workers_.emplace_back (
            std::make_unique<Worker> (i, *this, workgroup));

          const auto start_normal_thread = [&worker] {
            if (!worker->startThread ())
              {
                throw ZrythmException ("startThread failed");
              }
          };

          if (realtime_options.has_value ())
            {
              if (!worker->startRealtimeThread (realtime_options.value ()))
                {
                  z_warning (
                    "failed to start realtime thread, trying normal thread");
                  start_normal_thread ();
                }
            }
          else
            {
              start_normal_thread ();
            }

          // Wait for the thread to be running before starting the next one:
          // works around a JUCE race on first thread construction (same as
          // the graph scheduler)
          while (!worker->isThreadRunning ())
            {
              std::this_thread::yield ();
            }
          worker->thread_started_ = true;
        }
    }
  catch (const std::exception &e)
    {
      stop ();
      throw ZrythmException (
        "failed to start fork-join worker: " + std::string (e.what ()));
    }

  started_.store (true, std::memory_order_release);

  z_debug ("fork-join executor started with {} workers", num_workers);
}

void
ForkJoinExecutor::stop ()
{
  if (!is_started () && workers_.empty ())
    return;

  stop_requested_.store (true, std::memory_order_release);
  for (const auto _ : std::views::iota (0, static_cast<int> (workers_.size ())))
    {
      work_available_sem_.signal ();
    }

  for (auto &worker : workers_)
    {
      if (!worker->thread_started_)
        {
          // Thread creation failed during start(): there is no thread to
          // join, and finished_ will never be set
          continue;
        }
      worker->waitForThreadToExit (-1);
      // waitForThreadToExit only sleep-polls (no happens-before edge), so
      // synchronize-with the worker's release store before destroying it
      while (!worker->finished_.load (std::memory_order_acquire))
        std::this_thread::yield ();
    }
  workers_.clear ();

  started_.store (false, std::memory_order_release);
}

} // namespace zrythm::dsp::graph
