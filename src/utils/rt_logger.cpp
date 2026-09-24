// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "utils/logger.h"
#include "utils/rt_logger.h"
#include "utils/threads.h"

#include <boost/lockfree/queue.hpp>

namespace zrythm::utils
{

namespace
{

// Pre-allocated node pool (capacity plus the queue's dummy node), so
// pushes never allocate; a full queue rejects the push
using RtLogQueue = boost::lockfree::
  queue<RtLogMsg, boost::lockfree::capacity<rt_log_queue_capacity>>;

constexpr auto consumer_tick = std::chrono::milliseconds{ 10 };

class RtLoggerState
{
public:
  RtLoggerState () : queue_ ()
  {
    assert (queue_.is_lock_free ());
    thread_ = std::jthread ([this] (std::stop_token st) { run (st); });
  }

  void submit (RtLogMsg &&msg) noexcept
  {
    if (!queue_.push (msg))
      dropped_total_.fetch_add (1, std::memory_order_relaxed);
  }

  std::uint64_t dropped_count () const noexcept
  {
    return dropped_total_.load (std::memory_order_relaxed);
  }

  /**
   * @brief Suspends or resumes draining.
   *
   * Pausing blocks until the consumer has observed the pause, so no
   * message pushed after this returns can be drained concurrently.
   */
  void set_paused (bool paused)
  {
    if (paused)
      {
        // A stale acknowledgement from an earlier pause cycle must not
        // satisfy this pause wait
        paused_ack_.store (false, std::memory_order_release);
        paused_.store (true, std::memory_order_release);
        std::unique_lock lk (mtx_);
        cv_.wait_for (lk, std::chrono::milliseconds{ 2000 }, [this] () noexcept {
          return paused_ack_.load (std::memory_order_acquire);
        });
      }
    else
      {
        paused_.store (false, std::memory_order_release);
        std::lock_guard lk (mtx_);
        cv_.notify_all ();
      }
  }

  /**
   * @brief Waits until two drain passes have completed after this call.
   *
   * If the epoch load sees E, the pass that produced E finished its pops
   * before this call, so only the completion of the second later pass
   * guarantees that every message submitted before this call was popped.
   */
  bool wait_until_drained (std::chrono::milliseconds timeout)
  {
    const auto       my_epoch = pass_epoch_.load (std::memory_order_acquire);
    std::unique_lock lk (mtx_);
    return cv_.wait_for (lk, timeout, [this, my_epoch] () noexcept {
      return pass_epoch_.load (std::memory_order_acquire) >= my_epoch + 2;
    });
  }

private:
  void run (std::stop_token stop_token)
  {
    set_current_thread_name ("RT log pump");

    while (!stop_token.stop_requested ())
      {
        if (paused_.load (std::memory_order_acquire))
          {
            std::lock_guard lk (mtx_);
            paused_ack_.store (true, std::memory_order_release);
            cv_.notify_all ();
          }
        else
          {
            drain ();
          }
        std::unique_lock lk (mtx_);
        cv_.wait_for (lk, consumer_tick, [&stop_token] {
          return stop_token.stop_requested ();
        });
      }
    drain ();
  }

  void drain ()
  {
    RtLogMsg msg;
    while (queue_.pop (msg))
      forward (msg);

    const auto dropped = dropped_total_.load (std::memory_order_relaxed);
    if (dropped > last_reported_drops_)
      {
        report_drops_if_boundary_crossed (last_reported_drops_, dropped);
        last_reported_drops_ = dropped;
      }

    pass_epoch_.fetch_add (1, std::memory_order_release);
    std::lock_guard lk (mtx_);
    cv_.notify_all ();
  }

  static void
  report_drops_if_boundary_crossed (std::uint64_t before, std::uint64_t total)
  {
    // Report the first drop and every power-of-two total
    for (std::uint64_t boundary = 1; boundary <= total; boundary <<= 1)
      {
        if (boundary > before)
          {
            log_warning (
              std::source_location::current (),
              "RT log queue full: {} message(s) dropped so far", total);
            return;
          }
      }
  }

  static void forward (const RtLogMsg &msg)
  {
    const auto text = std::string_view (msg.text, msg.len);
    switch (msg.level)
      {
      case LogLevel::Trace:
        log_trace (msg.loc, "[rt thread {}] {}", msg.thread_id, text);
        break;
      case LogLevel::Debug:
        log_debug (msg.loc, "[rt thread {}] {}", msg.thread_id, text);
        break;
      case LogLevel::Info:
        log_info (msg.loc, "[rt thread {}] {}", msg.thread_id, text);
        break;
      case LogLevel::Warning:
        log_warning (msg.loc, "[rt thread {}] {}", msg.thread_id, text);
        break;
      case LogLevel::Error:
      case LogLevel::Critical:
        // Unreachable: the realtime API only accepts trace..warning
        break;
      }
  }

  RtLogQueue queue_;

  // Producer-side drop count, incremented when the queue rejects a push
  std::atomic<std::uint64_t> dropped_total_{ 0 };
  std::atomic<std::uint64_t> pass_epoch_{ 0 };
  std::atomic<bool>          paused_{ false };
  std::atomic<bool>          paused_ack_{ false };

  // Consumer-thread state
  std::uint64_t last_reported_drops_ = 0;

  std::mutex              mtx_;
  std::condition_variable cv_;
  std::jthread            thread_;
};

// Leaked on purpose: never destroyed, so the queue stays valid for any
// producer still running during static teardown, and the consumer thread
// is never joined while a forward may be in flight
std::atomic<RtLoggerState *> g_rt_logger_state{ nullptr };

RtLoggerState *
rt_state () noexcept
{
  return g_rt_logger_state.load (std::memory_order_acquire);
}

} // namespace

void
init_rt_logging ()
{
  if (rt_state () != nullptr)
    return;
  auto   expected = static_cast<RtLoggerState *> (nullptr);
  auto * state = new RtLoggerState ();
  if (!g_rt_logger_state.compare_exchange_strong (
        expected, state, std::memory_order_acq_rel))
    {
      delete state;
    }
}

bool
is_rt_logging_initialized () noexcept
{
  return rt_state () != nullptr;
}

void
submit_rt_log (RtLogMsg &&msg) noexcept
{
  if (auto * state = rt_state ())
    state->submit (std::move (msg));
}

bool
drain_rt_log (std::chrono::milliseconds timeout)
{
  auto * state = rt_state ();
  if (state == nullptr)
    return true;
  return state->wait_until_drained (timeout);
}

std::uint64_t
rt_log_dropped_count () noexcept
{
  auto * state = rt_state ();
  return state == nullptr ? 0 : state->dropped_count ();
}

void
set_rt_log_consumer_paused (bool paused)
{
  if (auto * state = rt_state ())
    state->set_paused (paused);
}

} // namespace zrythm::utils
