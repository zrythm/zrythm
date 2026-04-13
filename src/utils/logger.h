// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>
#include <source_location>
#include <vector>

#include <fmt/format.h>

// Logger architecture: Global API Injection Pattern
//
// Uses a variable template (log_api<>) that defaults to NullLogApi and is
// explicitly specialized to SpdlogApi. The log_* function templates dispatch
// through this variable template, so spdlog is never referenced in this
// header -- all spdlog usage lives in logger.cpp.
//
// This is a compile-time form of dependency injection: no virtual dispatch,
// no singletons, no passthrough antipattern. When the logger is not
// initialized (logger_ == nullptr), SpdlogApi::log() returns immediately
// after a single pointer check.
//
// Based on Ben Deane's pattern:
// https://www.elbeno.com/blog/?p=1831

namespace zrythm::utils
{
class Utf8String;

enum class LoggerType
{
  GUI,
  Test,
};

enum class LogLevel
{
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warning = 3,
  Error = 4,
  Critical = 5,
};

namespace detail
{

struct NullLogApi
{
  template <typename... FmtArgs>
  void log (
    std::source_location,
    LogLevel,
    fmt::format_string<FmtArgs...>,
    FmtArgs &&...) const
  {
  }
};

struct SpdlogApi
{
  ~SpdlogApi ();

  template <typename... FmtArgs>
  void log (
    std::source_location           loc,
    LogLevel                       level,
    fmt::format_string<FmtArgs...> fmt,
    FmtArgs &&... args) const
  {
    if (!logger_.load (std::memory_order_acquire))
      return;
    auto msg = fmt::format (fmt, std::forward<FmtArgs> (args)...);
    submit (loc, level, std::move (msg));
  }

  void init (LoggerType type);

  // Opaque -- defined in logger.cpp, hides spdlog from this header.
  struct Logger;
  // Owned raw pointer: cleaned up by ~SpdlogApi() (defined in .cpp where
  // Logger is complete). unique_ptr doesn't work here because the inline
  // variable specialization triggers unique_ptr's destructor in every TU.
  // Atomic for thread-safe init + safe static teardown.
  std::atomic<Logger *> logger_{ nullptr };
  std::once_flag        init_flag_;

private:
  // Non-template bridge to spdlog -- defined in logger.cpp.
  void submit (std::source_location loc, LogLevel level, std::string msg) const;
};

template <typename... Tags> inline auto log_api = NullLogApi{};

template <> inline auto log_api<> = SpdlogApi{};

} // namespace detail

void
init_logging (LoggerType type);
bool
is_logging_initialized ();

std::vector<Utf8String>
get_last_log_entries (size_t count);

std::filesystem::path
get_log_file_path ();

template <typename... Args>
void
log_trace (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  detail::log_api<>.log (
    loc, LogLevel::Trace, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_debug (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  detail::log_api<>.log (
    loc, LogLevel::Debug, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_info (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  detail::log_api<>.log (loc, LogLevel::Info, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_warning (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  detail::log_api<>.log (
    loc, LogLevel::Warning, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_error (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  detail::log_api<>.log (
    loc, LogLevel::Error, fmt, std::forward<Args> (args)...);
}

template <typename... Args>
void
log_critical (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  detail::log_api<>.log (
    loc, LogLevel::Critical, fmt, std::forward<Args> (args)...);
}

#define z_trace(...) \
  ::zrythm::utils::log_trace (std::source_location::current (), __VA_ARGS__)
#define z_debug(...) \
  ::zrythm::utils::log_debug (std::source_location::current (), __VA_ARGS__)
#define z_info(...) \
  ::zrythm::utils::log_info (std::source_location::current (), __VA_ARGS__)
#define z_warning(...) \
  ::zrythm::utils::log_warning (std::source_location::current (), __VA_ARGS__)
#define z_error(...) \
  ::zrythm::utils::log_error (std::source_location::current (), __VA_ARGS__)
#define z_critical(...) \
  ::zrythm::utils::log_critical (std::source_location::current (), __VA_ARGS__)

#define z_return_val_if_fail(cond, val) \
  if (!(cond)) [[unlikely]] \
    { \
      z_error ("Assertion failed: {}", #cond); \
      return val; \
    }

#define z_return_if_fail(cond) z_return_val_if_fail (cond, )

#define z_return_val_if_reached(val) \
  { \
    z_error ("This code should not be reached"); \
    return val; \
  }

#define z_return_if_reached() z_return_val_if_reached ()

#define z_warn_if_fail(cond) \
  if (!(cond)) [[unlikely]] \
    { \
      z_warning ("Assertion failed: {}", #cond); \
    }

#define z_warn_if_reached() z_warning ("This code should not be reached")

}; // namespace zrythm::utils
