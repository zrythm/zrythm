// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <moodycamel/lightweightsemaphore.h>
#include <rigtorp/MPMCQueue.h>

namespace zrythm::dsp::graph
{

/**
 * @brief Realtime worker pool for fork-join parallel processing within a
 * single graph node.
 *
 * The executor runs "jobs" submitted from processing threads: each job is a
 * set of tasks identified by index (0 to num_tasks - 1) sharing one task
 * function and context. The submitting thread blocks until every task of its
 * job has completed, and helps execute queued tasks (its own and other
 * jobs') while it waits, so a submission always completes even when all
 * workers are busy.
 *
 * Lifecycle: the owner thread calls start()/stop(); exec() is the realtime
 * entry point and may be called concurrently from multiple processing
 * threads.
 *
 * @note exec() blocks the calling thread by design (fork-join). Callers in
 * [[clang::nonblocking]] contexts must wrap the call site in a
 * __rtsan::ScopedDisabler (same pattern as the plugin process wrappers).
 */
class ForkJoinExecutor
{
public:
  /**
   * @brief Task callback invoked once per task index of a job.
   *
   * Called from worker threads and from submitting threads while helping.
   * Must be realtime-safe: no allocation, no blocking, no calls back into
   * exec() (rejected as a contract violation), and it must return for the
   * job to complete.
   */
  using TaskFunction = void (*) (void * context, std::uint32_t task_index);

  /** Maximum number of jobs that may be outstanding (submitters blocked in
   * exec()) at the same time. */
  static constexpr std::uint32_t kMaxConcurrentJobs = 256;

public:
  ForkJoinExecutor ();
  ~ForkJoinExecutor ();
  ForkJoinExecutor (const ForkJoinExecutor &) = delete;
  ForkJoinExecutor &operator= (const ForkJoinExecutor &) = delete;
  ForkJoinExecutor (ForkJoinExecutor &&) = delete;
  ForkJoinExecutor &operator= (ForkJoinExecutor &&) = delete;

  /**
   * @brief Starts the worker threads. Owner thread only.
   *
   * Idempotent: does nothing if already started.
   *
   * @param num_workers Number of worker threads. May be 0, in which case
   * submitters execute their jobs entirely by themselves (helping only).
   * @param realtime_options Realtime options for the workers, or nullopt for
   * normal-priority threads (e.g. tests).
   * @param workgroup Optional audio workgroup for the workers to join
   * (macOS).
   * @throw ZrythmException if a thread fails to start.
   */
  void start (
    int num_workers,
    std::optional<juce::Thread::RealtimeOptions> realtime_options = std::nullopt,
    std::optional<juce::AudioWorkgroup> workgroup = std::nullopt);

  /**
   * @brief Stops and joins all worker threads. Owner thread only.
   *
   * Workers finish their currently claimed tasks before exiting and
   * submitters always complete their own jobs by self-draining, so a job in
   * flight during stop() still completes. exec() after stop() returns false.
   */
  void stop ();

  bool is_started () const noexcept
  {
    return started_.load (std::memory_order_acquire);
  }

  /**
   * @brief Executes @p num_tasks tasks (indices 0 to @p num_tasks - 1) and
   * blocks until all of them complete.
   *
   * May be called concurrently from multiple processing threads; each caller
   * gets its own job. Requests for 0 or 1 tasks run inline on the calling
   * thread.
   *
   * @return true if all tasks were executed. Returns false without executing
   * anything when the request is rejected — called from within a task
   * (contract violation), executor not started, all job slots in use, or the
   * job's work entry could not be queued — in which case the caller must run
   * the tasks itself (serial fallback).
   */
  bool
  exec (TaskFunction task, void * context, std::uint32_t num_tasks) noexcept;

private:
  /** A single outstanding job. Slots are preallocated and recycled through
   * the free list, so exec() never allocates. Over-aligned so the hot
   * atomics of concurrent jobs don't share cache lines. */
  struct alignas (std::hardware_destructive_interference_size) JobSlot
  {
    /* Written by the submitter before publishing the job and never while
     * another thread participates in the slot's current job (see state_),
     * so participants can read them without further synchronization. */
    TaskFunction  task_function_ = nullptr;
    void *        context_ = nullptr;
    std::uint32_t num_tasks_ = 0;

    /** High 32 bits: generation, bumped on each reuse (queued work items
     * carry the generation they were queued under, so stale entries are
     * discarded). Low 32 bits: threads currently participating in the job,
     * including the submitter. The submitter recycles the slot only when it
     * is the last remaining participant. */
    std::atomic<std::uint64_t> state_{ 0 };

    /** Next task index to hand out. */
    std::atomic<std::uint32_t> next_task_index_{ 0 };

    /** Number of tasks that finished executing. */
    std::atomic<std::uint32_t> completed_tasks_{ 0 };

    /** Signaled exactly once when completed_tasks_ reaches num_tasks_. The
     * submitter waits on it exactly once per job, so the semaphore is always
     * balanced when the slot is free. */
    moodycamel::LightweightSemaphore completion_sem_{ 0 };
  };

  /** A queued unit of work: a job slot plus the generation it was queued
   * under (see JobSlot::state_). */
  struct WorkItem
  {
    JobSlot *     slot_;
    std::uint32_t generation_;
  };

  static constexpr std::uint32_t generation_of (std::uint64_t state)
  {
    return static_cast<std::uint32_t> (state >> 32);
  }
  static constexpr std::uint32_t participants_of (std::uint64_t state)
  {
    return static_cast<std::uint32_t> (state);
  }

  /** Signals (release) when the worker's run() has returned, so that stop()
   * can synchronize-with the worker before destroying the Worker object:
   * juce::Thread uses detached pthreads and a sleep-polling
   * waitForThreadToExit, which establishes no happens-before edge for
   * anything the thread did (formally racy teardown, flagged by TSan). */
  class Worker;

  /** Worker thread main loop. */
  void worker_thread_loop () noexcept;

  /**
   * @brief Pops and runs queued work until the queue is drained.
   *
   * Each popped entry whose job still has unclaimed tasks is re-queued (so
   * other participants can claim them) and one more worker is woken before
   * this thread starts claiming tasks itself. Stale entries are discarded.
   */
  void drain_work_queue () noexcept;

  /** Runs the given job's remaining tasks until none are left to claim. */
  void run_tasks_from_job (JobSlot &slot) noexcept;

  /**
   * @brief Returns a slot to the free list.
   *
   * try_push() can fail spuriously while a concurrent pop is between its
   * tail CAS and its turn store on the slot that head wraps onto (see
   * rigtorp::MPMCQueue::try_emplace). The queue always has room for a slot
   * being returned (it is, by definition, not in the queue), so this
   * retries until the preempted popper's turn store lands.
   */
  void return_slot_to_free_list (JobSlot &slot) noexcept;

private:
  std::array<JobSlot, kMaxConcurrentJobs> job_slots_{};

  /** Slots not currently assigned to a job (initially holds all slots). */
  rigtorp::MPMCQueue<JobSlot *> free_slots_{ kMaxConcurrentJobs };

  /** Queued entries pointing at jobs with work to do. Entries are re-queued
   * while their job has unclaimed tasks, so the queue can hold several
   * entries per job; capacity is generous and re-queueing is best-effort. */
  rigtorp::MPMCQueue<WorkItem> work_queue_{ kMaxConcurrentJobs * 4 };

  /** Wakes idle workers when jobs arrive. Not a std::counting_semaphore due
   * to issues on MSVC/Windows (same as the graph scheduler). */
  moodycamel::LightweightSemaphore work_available_sem_{ 0 };

  std::vector<std::unique_ptr<Worker>> workers_;

  std::atomic<bool> stop_requested_{ false };
  std::atomic<bool> started_{ false };
};

} // namespace zrythm::dsp::graph
