// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <source_location>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace zrythm::utils
{
class Utf8String;

class ILogger
{
public:
  virtual ~ILogger () = default;

  void init_sinks (bool for_testing);

  std::vector<Utf8String>
  get_last_log_entries (size_t count, bool formatted) const;

  /**
   * Generates a compressed log file (for sending with bug reports).
   *
   * @return The directory and the path of the compressed file.
   * @throw ZrythmException on failure.
   */
  virtual std::pair<std::filesystem::path, std::filesystem::path>
  generate_compresed_file (
    std::filesystem::path &dir,
    std::filesystem::path &path) const = 0;

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
  [[nodiscard]] const std::shared_ptr<spdlog::logger> &get_logger () const
  {
    return logger_;
  }
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

  virtual bool need_backtrace () const = 0;

  virtual std::filesystem::path get_log_file_path () const = 0;

protected:
  std::shared_ptr<spdlog::logger> logger_;

  /**
   * Last timestamp a backtrace was obtained.
   *
   * This is used to avoid calculating too many backtraces and showing too
   * many error popups at once.
   */
  double last_bt_time_ = 0;
};

class Logger : public ILogger
{
public:
  enum class LoggerType
  {
    GUI,
    Engine,
  };

  Logger (LoggerType type);

  std::pair<std::filesystem::path, std::filesystem::path>
  generate_compresed_file (
    std::filesystem::path &dir,
    std::filesystem::path &path) const override;

  bool need_backtrace () const override;

  [[nodiscard]] std::filesystem::path get_log_file_path () const override;

private:
  LoggerType type_{};
};

class TestLogger : public ILogger
{
public:
  TestLogger ();

  std::pair<std::filesystem::path, std::filesystem::path>
  generate_compresed_file (
    std::filesystem::path &dir,
    std::filesystem::path &path) const override;

  bool need_backtrace () const override;

  std::filesystem::path get_log_file_path () const override;
};

class LoggerProvider
{
public:
  static void set_logger (std::shared_ptr<ILogger> logger)
  {
    instance ().logger_ = std::move (logger);
  }

  static ILogger &logger ()
  {
    auto &logger_provider = instance ();
    if (!logger_provider.logger_) [[unlikely]]
      {
        logger_provider.logger_ = std::make_shared<TestLogger> ();
      }
    return *logger_provider.logger_;
  }

  static bool has_logger () { return instance ().logger_ != nullptr; };

private:
  static LoggerProvider &instance ()
  {
    static LoggerProvider provider;
    return provider;
  }

  std::shared_ptr<ILogger> logger_;
};

namespace detail
{
inline spdlog::source_loc
to_source_loc (const std::source_location &loc)
{
  return {
    loc.file_name (), static_cast<int> (loc.line ()), loc.function_name ()
  };
}
} // namespace detail

template <typename... Args>
void
log_trace (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  LoggerProvider::logger ().get_logger ()->log (
    detail::to_source_loc (loc), spdlog::level::trace, fmt,
    std::forward<Args> (args)...);
}

template <typename... Args>
void
log_debug (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  LoggerProvider::logger ().get_logger ()->log (
    detail::to_source_loc (loc), spdlog::level::debug, fmt,
    std::forward<Args> (args)...);
}

template <typename... Args>
void
log_info (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  LoggerProvider::logger ().get_logger ()->log (
    detail::to_source_loc (loc), spdlog::level::info, fmt,
    std::forward<Args> (args)...);
}

template <typename... Args>
void
log_warning (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  LoggerProvider::logger ().get_logger ()->log (
    detail::to_source_loc (loc), spdlog::level::warn, fmt,
    std::forward<Args> (args)...);
}

template <typename... Args>
void
log_error (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  LoggerProvider::logger ().get_logger ()->log (
    detail::to_source_loc (loc), spdlog::level::err, fmt,
    std::forward<Args> (args)...);
}

template <typename... Args>
void
log_critical (
  std::source_location        loc,
  fmt::format_string<Args...> fmt,
  Args &&... args)
{
  LoggerProvider::logger ().get_logger ()->log (
    detail::to_source_loc (loc), spdlog::level::critical, fmt,
    std::forward<Args> (args)...);
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

/**
 * @brief Safe assertion macro that returns a value if the assertion fails.
 */
#define z_return_val_if_fail(cond, val) \
  if (!(cond)) [[unlikely]] \
    { \
      z_error ("Assertion failed: {}", #cond); \
      return val; \
    }

/**
 * @brief Safe assertion macro that returns if the assertion fails.
 */
#define z_return_if_fail(cond) z_return_val_if_fail (cond, )

/**
 * @brief Logs an error and returns @p val if hit.
 */
#define z_return_val_if_reached(val) \
  { \
    z_error ("This code should not be reached"); \
    return val; \
  }

/**
 * @brief Logs an error and returns if hit.
 */
#define z_return_if_reached() z_return_val_if_reached ()

#define z_warn_if_fail(cond) \
  if (!(cond)) [[unlikely]] \
    { \
      z_warning ("Assertion failed: {}", #cond); \
    }

#define z_warn_if_reached() z_warning ("This code should not be reached")

}; // namespace zrythm::utils
