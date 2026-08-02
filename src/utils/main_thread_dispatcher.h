// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <bit>
#include <cassert>
#include <chrono>
#include <functional>
#include <type_traits>
#include <utility>

#include "utils/qt.h"

#include <QThread>
#include <QTimer>

#include <farbot/fifo.hpp>

namespace zrythm::utils
{

/**
 * @brief Dispatches requests of a trivially copyable type from any thread
 * (including realtime threads) to a Qt object's thread for handling.
 *
 * Requests are pushed to a pre-allocated lock-free fifo — no allocations or
 * locks on the calling thread — and drained by a QTimer pump on the context
 * object's thread. Requests posted from the context's own thread are handled
 * synchronously, after any already-queued requests.
 *
 * Each pump handles at most one fifo's worth of requests, so a sustained
 * flood of posts cannot starve the event loop; the remainder is handled on
 * subsequent pumps.
 *
 * Preconditions:
 * - @p context's thread affinity must not change for the dispatcher's
 *   lifetime.
 * - Other threads must stop calling post() before the dispatcher is
 *   destroyed.
 *
 * @note Due to a farbot implementation detail, at most 64 distinct threads
 * can ever post to a given dispatcher instance (threads are registered on
 * first post and never unregistered). This is ample for plugin main-thread
 * callbacks (audio thread, host thread pools, plugin-internal threads).
 *
 * @tparam fifo_capacity Maximum number of pending requests before posts
 * start getting dropped. Must be a power of 2.
 */
template <typename T, std::size_t fifo_capacity = 32>
  requires std::is_trivially_copy_constructible_v<T>
           && std::is_default_constructible_v<T>
class MainThreadDispatcher
{
  static_assert (
    std::has_single_bit (fifo_capacity),
    "fifo_capacity must be a power of 2");

public:
  using Handler = std::function<void (const T &)>;

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
      : context_ (context), handler_ (std::move (handler)),
        fifo_ (static_cast<int> (fifo_capacity))
  {
    assert (QThread::currentThread () == context_.thread ());

    pump_timer_ = utils::make_qobject_unique<QTimer> (&context_);
    QObject::connect (pump_timer_.get (), &QTimer::timeout, &context_, [this] {
      process_pending ();
    });
    pump_timer_->start (pump_interval);
  }

  // The pump timer connection captures `this`, and the class is not intended
  // to be copied or moved (already impossible via the fifo's atomic members)
  MainThreadDispatcher (const MainThreadDispatcher &) = delete;
  MainThreadDispatcher (MainThreadDispatcher &&) = delete;
  MainThreadDispatcher &operator= (const MainThreadDispatcher &) = delete;
  MainThreadDispatcher &operator= (MainThreadDispatcher &&) = delete;

  /**
   * @brief Posts a request to be handled on the context's thread.
   *
   * Realtime-safe: no allocations or locks (the fifo push is block-free).
   * When called on the context's own thread, pending requests are drained
   * first and the request is then handled synchronously, so requests are
   * always handled in the order they were posted.
   *
   * @return False if the request was dropped because the fifo was full
   * (only possible under pathological flooding, since the fifo is drained
   * on every pump interval).
   */
  bool post (const T &request) noexcept
  {
    if (QThread::currentThread () == context_.thread ())
      {
        process_pending ();
        handler_ (request);
        return true;
      }
    return fifo_.push (T{ request });
  }

  /**
   * @brief Handles pending requests (at most one fifo's worth). Called on
   * the context's thread.
   */
  void process_pending ()
  {
    assert (QThread::currentThread () == context_.thread ());

    T   request{};
    int handled = 0;
    while (handled++ < static_cast<int> (fifo_capacity) && fifo_.pop (request))
      handler_ (request);
  }

private:
  QObject &context_;
  Handler  handler_;

  farbot::fifo<
    T,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::concurrency::multiple>
                                  fifo_;
  utils::QObjectUniquePtr<QTimer> pump_timer_;
};

} // namespace zrythm::utils
