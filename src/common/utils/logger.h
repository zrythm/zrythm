// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_LOGGER_H__
#define __UTILS_LOGGER_H__

#include "zrythm-config.h"

/**
 * @addtogroup utils
 *
 * @{
 */

#include "common/utils/format.h"

#include "juce/juce.h"
#include <spdlog/spdlog.h>

class Logger
{
public:
  std::vector<std::string>
  get_last_log_entries (size_t count, bool formatted) const;

  /**
   * Generates a compressed log file (for sending with bug reports).
   *
   * @return The directory and the path of the compressed file.
   * @throw ZrythmException on failure.
   */
  std::pair<std::string, std::string>
  generate_compresed_file (std::string &dir, std::string &path) const;

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

  ~Logger ();

private:
  Logger ();

  bool need_backtrace () const;

  std::string get_log_file_path () const;

private:
  std::shared_ptr<spdlog::logger> logger_;

  /**
   * Last timestamp a backtrace was obtained.
   *
   * This is used to avoid calculating too many backtraces and showing too
   * many error popups at once.
   */
  double last_bt_time_ = 0;

public:
  JUCE_DECLARE_SINGLETON_SINGLETHREADED (Logger, false)
};

#define z_warning(...) \
  SPDLOG_LOGGER_WARN (Logger::getInstance ()->get_logger (), __VA_ARGS__)
#define z_error(...) \
  SPDLOG_LOGGER_ERROR (Logger::getInstance ()->get_logger (), __VA_ARGS__)
#define z_critical(...) \
  SPDLOG_LOGGER_CRITICAL (Logger::getInstance ()->get_logger (), __VA_ARGS__)
#define z_trace(...) \
  SPDLOG_LOGGER_TRACE (Logger::getInstance ()->get_logger (), __VA_ARGS__)
#define z_debug(...) \
  SPDLOG_LOGGER_DEBUG (Logger::getInstance ()->get_logger (), __VA_ARGS__)
#define z_info(...) \
  SPDLOG_LOGGER_INFO (Logger::getInstance ()->get_logger (), __VA_ARGS__)

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

/**
 * @}
 */

#endif
