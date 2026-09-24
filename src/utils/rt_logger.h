// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <source_location>

#include "utils/logger.h"
#include "utils/rt_thread_id.h"

#include <fmt/format.h>

// Realtime-safe logging: messages are formatted into a fixed-size slot on
// the calling thread and pushed to a lock-free multi-producer queue; a
// background thread drains the queue and forwards each message to the
// regular spdlog pipeline with the original call site's metadata. When the
// queue is full the push fails and the message is counted as dropped; the
// consumer reports the drop count, throttled to the first drop and each
// power-of-two total. Errors and criticals have no realtime variant: they
// keep using the synchronous z_error/z_critical, whose backtrace is only
// meaningful on the calling thread and whose blocking is acceptable on a
// path that is about to abort.
//
// The slot-and-drain-thread technique was popularized by rtlog-cpp
// (https://github.com/cjappl/rtlog-cpp, ADCx 2023 talk); this is an
// independent implementation on boost::lockfree::queue and fmt. A full
// push is rejected and counted, so no message is delivered twice and
// drops are counted exactly. Messages from concurrent producers appear
// in the queue's linearization order, which need not match wall-clock
// submission order across threads. Formatting arguments must not
// allocate when formatted (plain strings and arithmetic types are safe).

namespace zrythm::utils
{

/** Number of message slots in the realtime queue. */
constexpr std::size_t rt_log_queue_capacity = 512;

/**
 * @brief A single fixed-size realtime log message slot.
 *
 * Constructed on the caller's stack, formatted in place, then moved into
 * the realtime queue.
 */
struct RtLogMsg
{
  static constexpr std::size_t text_capacity = 256;

  /** Log severity at the call site. */
  LogLevel level = LogLevel::Info;
  /** Call site captured by the z_rt_* macro. */
  std::source_location loc = std::source_location::current ();
  /** Identifier of the producing thread (RTThreadId of the caller). */
  std::uint32_t thread_id = 0;
  /** Length of the formatted text, at most @ref text_capacity - 1. */
  std::uint32_t len = 0;
  /** Fixed-size formatted text, always null-terminated. */
  char text[text_capacity] = {};
};

bool
is_rt_logging_initialized () noexcept [[clang::nonblocking]];
void
submit_rt_log (RtLogMsg &&msg) noexcept [[clang::nonblocking]];

/**
 * @brief Creates the realtime log queue and starts its consumer thread.
 *
 * Calling it again has no effect. Realtime log functions are no-ops
 * before this has run.
 */
void
init_rt_logging ();

namespace detail
{

template <typename... Args>
void
rt_log_impl (
  LogLevel                    level,
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args) noexcept [[clang::nonblocking]]
{
  if (!is_rt_logging_initialized () || !should_log (level))
    return;

  RtLogMsg msg;
  msg.level = level;
  msg.loc = loc;
  msg.thread_id = current_thread_id.get ();
  const auto result = fmt::format_to_n (
    msg.text, RtLogMsg::text_capacity - 1, fmt, std::forward<Args> (args)...);
  msg.len = static_cast<std::uint32_t> (
    std::min (result.size, RtLogMsg::text_capacity - 1));
  msg.text[msg.len] = '\0';
  submit_rt_log (std::move (msg));
}

} // namespace detail

/**
 * @name Realtime-safe log functions
 *
 * Same contract as their synchronous counterparts, except the message is
 * delivered asynchronously and truncated to RtLogMsg::text_capacity - 1
 * characters. No-ops until init_logging() has run, and for levels the
 * active logger filters out. Formatting arguments must not allocate when
 * formatted.
 */
///@{
template <typename... Args>
void
log_rt_trace (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args) noexcept [[clang::nonblocking]]
{
  detail::rt_log_impl (LogLevel::Trace, loc, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_rt_debug (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args) noexcept [[clang::nonblocking]]
{
  detail::rt_log_impl (LogLevel::Debug, loc, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_rt_info (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args) noexcept [[clang::nonblocking]]
{
  detail::rt_log_impl (LogLevel::Info, loc, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_rt_warning (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args) noexcept [[clang::nonblocking]]
{
  detail::rt_log_impl (
    LogLevel::Warning, loc, fmt, std::forward<Args> (args)...);
}
///@}

/**
 * @brief Waits until every message submitted so far has been forwarded.
 *
 * Blocks for at most @p timeout (two consumer passes, so up to roughly
 * twenty milliseconds plus drain time); returns false on timeout.
 */
[[nodiscard]] bool
drain_rt_log (std::chrono::milliseconds timeout);

/**
 * @brief Total number of messages dropped because the queue was full.
 */
[[nodiscard]] std::uint64_t
rt_log_dropped_count () noexcept;

/**
 * @brief Suspends or resumes the consumer thread.
 *
 * With the consumer suspended, a full queue rejects pushes and counts
 * them as dropped, which makes drop behavior deterministic. Pausing
 * blocks until the consumer has observed the pause. The consumer
 * resumes on the next pass after a resume.
 */
void
set_rt_log_consumer_paused (bool paused);

#define z_rt_trace(...) \
  ::zrythm::utils::log_rt_trace (std::source_location::current (), __VA_ARGS__)
#define z_rt_debug(...) \
  ::zrythm::utils::log_rt_debug (std::source_location::current (), __VA_ARGS__)
#define z_rt_info(...) \
  ::zrythm::utils::log_rt_info (std::source_location::current (), __VA_ARGS__)
#define z_rt_warning(...) \
  ::zrythm::utils::log_rt_warning ( \
    std::source_location::current (), __VA_ARGS__)

} // namespace zrythm::utils
