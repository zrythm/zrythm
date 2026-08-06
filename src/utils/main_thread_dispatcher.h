// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cassert>
#include <chrono>
#include <functional>
#include <type_traits>
#include <utility>

#include "utils/inplace_function.h"
#include "utils/qt.h"

#include <QThread>
#include <QTimer>

#include <rigtorp/MPMCQueue.h>

namespace zrythm::utils
{

/**
 * @brief Dispatches requests of a move-constructible type from any thread
 * (including realtime threads) to a Qt object's thread for handling.
 *
 * Requests are pushed to a pre-allocated lock-free MPMC queue — no
 * allocations or locks on the calling thread — and drained by a QTimer pump
 * on the context object's thread. Requests posted from the context's own
 * thread are handled synchronously, after any already-queued requests —
 * except reentrant posts from a running handler, which are queued and
 * handled by the in-progress drain (or the next pump), so FIFO ordering is
 * preserved with bounded stack depth.
 *
 * Each pump handles at most @ref max_requests_per_pump requests, so a
 * sustained flood of posts cannot starve the event loop; the remainder is
 * handled on subsequent pumps.
 *
 * Preconditions:
 * - @p context's thread affinity must not change for the dispatcher's
 *   lifetime.
 * - Other threads must stop calling post() before the dispatcher is
 *   destroyed.
 *
 * @tparam fifo_capacity Maximum number of pending requests before posts
 * start getting dropped.
 */
template <typename T, std::size_t fifo_capacity = 32>
  requires std::is_default_constructible_v<T>
           && std::is_nothrow_move_constructible_v<T>
           && std::is_nothrow_move_assignable_v<T>
class MainThreadDispatcher
{
  static_assert (fifo_capacity >= 1);

public:
  using Handler = std::function<void (const T &)>;

  /**
   * @brief Maximum number of requests handled per pump, so that a
   * sustained flood of posts cannot starve the event loop.
   */
  static constexpr std::size_t max_requests_per_pump = 64;

  /**
   * @brief Constructs the dispatcher and starts the pump timer.
   *
   * Must be constructed on @p context's thread, and @p context must outlive
   * the dispatcher.
   *
   * @param context QObject whose thread requests are dispatched to.
   * @param pump_interval Interval at which pending requests are drained.
   * @param handler Invoked for each request on @p context's thread. Must not
   * throw.
   */
  MainThreadDispatcher (
    QObject                  &context,
    std::chrono::milliseconds pump_interval,
    Handler                   handler)
      : context_ (context), handler_ (std::move (handler)), queue_ (fifo_capacity)
  {
    assert (QThread::currentThread () == context_.thread ());

    pump_timer_ = utils::make_qobject_unique<QTimer> (&context_);
    QObject::connect (pump_timer_.get (), &QTimer::timeout, &context_, [this] {
      process_pending ();
    });
    pump_timer_->start (pump_interval);
  }

  /**
   * @brief Constructs the dispatcher without an explicit handler: each
   * request is invoked as the work item.
   *
   * Only participates in overload resolution when T is invocable.
   */
  MainThreadDispatcher (QObject &context, std::chrono::milliseconds pump_interval)
    requires std::invocable<const T &>
      : MainThreadDispatcher (context, pump_interval, [] (const T &request) {
          std::invoke (request);
        })
  {
  }

  // The pump timer connection captures `this`, and the queue is neither
  // copyable nor movable
  MainThreadDispatcher (const MainThreadDispatcher &) = delete;
  MainThreadDispatcher (MainThreadDispatcher &&) = delete;
  MainThreadDispatcher &operator= (const MainThreadDispatcher &) = delete;
  MainThreadDispatcher &operator= (MainThreadDispatcher &&) = delete;

  /**
   * @brief Posts a request to be handled on the context's thread.
   *
   * Realtime-safe: no allocations or locks (the queue push is lock-free and
   * bounded). When called on the context's own thread, pending requests are
   * drained first and the request is then handled synchronously, so
   * requests are always handled in the order they were posted. A reentrant
   * post from a running handler is queued instead of running inline, so
   * self-reposting handlers cannot recurse unboundedly.
   *
   * @return False if the request was dropped because the queue was full
   * (only possible under pathological flooding, since the queue is drained
   * on every pump interval).
   */
  bool post (T request) noexcept
  {
    if (QThread::currentThread () == context_.thread ())
      {
        if (handling_)
          {
            // Reentrant post from a running handler: queue it — the
            // in-progress drain (or the next pump) picks it up,
            // preserving FIFO order with bounded stack depth
            return queue_.try_push (std::move (request));
          }
        process_pending ();
        handling_ = true;
        handler_ (request);
        handling_ = false;
        return true;
      }
    return queue_.try_push (std::move (request));
  }

  /**
   * @brief Handles pending requests (at most @ref max_requests_per_pump).
   * Called on the context's thread.
   */
  void process_pending ()
  {
    assert (QThread::currentThread () == context_.thread ());

    // Reentrant call from a running handler: the in-progress drain keeps
    // handling queued requests
    if (handling_)
      return;

    handling_ = true;
    T           request{};
    std::size_t handled = 0;
    while (handled++ < max_requests_per_pump && queue_.try_pop (request))
      handler_ (request);
    handling_ = false;
  }

private:
  QObject &context_;
  Handler  handler_;

  rigtorp::MPMCQueue<T>           queue_;
  utils::QObjectUniquePtr<QTimer> pump_timer_;
  bool                            handling_ = false;
};

/**
 * @brief Type-erased closure for use with MainThreadDispatcher.
 *
 * Stored inline: capturing a few pointers never allocates.
 */
using MainThreadCallback = InplaceFunction<void (), 48>;

/**
 * @brief Dispatcher of type-erased closures (the common case for
 * host-bound requests, e.g. from plugins).
 *
 * The capacity is generous so that even plugins posting requests every
 * process block (e.g. CLAP request_callback) do not overflow between
 * pumps; the memory cost is ~200 KB for a single shared instance.
 */
using MainThreadClosureDispatcher =
  MainThreadDispatcher<MainThreadCallback, 1024>;

} // namespace zrythm::utils
