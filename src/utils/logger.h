// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_LOGGER_H__
#define __UTILS_LOGGER_H__

#include "zrythm-config.h"

#include "utils/format.h"
#include "utils/types.h"

#include "juce_wrapper.h"
#include <spdlog/spdlog.h>

namespace zrythm::utils
{

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
  virtual std::pair<fs::path, fs::path>
  generate_compresed_file (fs::path &dir, fs::path &path) const = 0;

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

  virtual fs::path get_log_file_path () const = 0;

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

  std::pair<fs::path, fs::path>
  generate_compresed_file (fs::path &dir, fs::path &path) const override;

  bool need_backtrace () const override;

  [[nodiscard]] fs::path get_log_file_path () const override;

private:
  LoggerType type_{};
};

class TestLogger : public ILogger
{
public:
  TestLogger ();

  std::pair<fs::path, fs::path>
  generate_compresed_file (fs::path &dir, fs::path &path) const override;

  bool need_backtrace () const override;

  fs::path get_log_file_path () const override;
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

#define LOGGER_INSTANCE zrythm::utils::LoggerProvider::logger ().get_logger ()
#define z_warning(...) SPDLOG_LOGGER_WARN (LOGGER_INSTANCE, __VA_ARGS__)
#define z_error(...) SPDLOG_LOGGER_ERROR (LOGGER_INSTANCE, __VA_ARGS__)
#define z_critical(...) SPDLOG_LOGGER_CRITICAL (LOGGER_INSTANCE, __VA_ARGS__)
#define z_trace(...) SPDLOG_LOGGER_TRACE (LOGGER_INSTANCE, __VA_ARGS__)
#define z_debug(...) SPDLOG_LOGGER_DEBUG (LOGGER_INSTANCE, __VA_ARGS__)
#define z_info(...) SPDLOG_LOGGER_INFO (LOGGER_INSTANCE, __VA_ARGS__)

/**
 * @brief Safe assertion macro that returns a value if the assertion fails.
 */
#define z_return_val_if_fail(cond, val) \
  if (!(cond)) [[unlikely]] \
    { \
      z_error (format_str ("Assertion failed: {}", #cond)); \
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
      z_warning (format_str ("Assertion failed: {}", #cond)); \
    }

#define z_warn_if_reached() z_warning ("This code should not be reached")

}; // namespace zrythm::utils

#endif
