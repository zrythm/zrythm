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

#include "utils/format.h"

#include "ext/juce/juce.h"
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

class Logger
{
public:
  std::vector<std::string>
  get_last_log_entries (size_t count, bool formatted) const
  {
    // Get the circular buffer sink from the logger
    auto buffer_sink = std::dynamic_pointer_cast<
      spdlog::sinks::ringbuffer_sink_mt> (logger_->sinks ().back ());
    assert (buffer_sink);
    return buffer_sink->last_formatted (count);
  }

  /**
   * Generates a compressed log file (for sending with bug reports).
   *
   * @return The directory and the path of the compressed file.
   * @throw ZrythmException on failure.
   */
  std::pair<std::string, std::string>
  generate_compresed_file (std::string &dir, std::string &path) const
  {
#if 0
    Error * err = NULL;
    char *  log_file_tmpdir = g_dir_make_tmp ("zrythm-log-file-XXXXXX", &err);
    if (!log_file_tmpdir)
      {
        g_set_error_literal (
          error, Z_UTILS_LOG_ERROR, Z_UTILS_LOG_ERROR_FAILED,
          "Failed to create temporary dir");
        return false;
      }

    /* get zstd-compressed text */
    char * log_txt = log_get_last_n_lines (LOG, 40000);
    z_return_val_if_fail (log_txt, false);
    size_t log_txt_sz = strlen (log_txt);
    size_t compress_bound = ZSTD_compressBound (log_txt_sz);
    char * dest = static_cast<char *> (malloc (compress_bound));
    size_t dest_size =
      ZSTD_compress (dest, compress_bound, log_txt, log_txt_sz, 1);
    if (ZSTD_isError (dest_size))
      {
        free (dest);

        g_set_error (
          error, Z_UTILS_LOG_ERROR, Z_UTILS_LOG_ERROR_FAILED,
          "Failed to compress log text: %s", ZSTD_getErrorName (dest_size));

        g_free (log_file_tmpdir);
        return false;
      }

    /* write to dest file */
    char * dest_filepath =
      g_build_filename (log_file_tmpdir, "log.txt.zst", nullptr);
    bool ret =
      g_file_set_contents (dest_filepath, dest, (gssize) dest_size, error);
    g_free (dest);
    if (!ret)
      {
        g_free (log_file_tmpdir);
        g_free (dest_filepath);
        return false;
      }

    *ret_dir = log_file_tmpdir;
    *ret_path = dest_filepath;
#endif
    return std::make_pair ("dir", "path");
  }

  auto get_logger () const { return logger_; }

private:
  Logger ();

  bool need_backtrace () const
  {
    constexpr int backtrace_cooldown_time = 16 * 1000 * 1000;
    auto          now = juce::Time::getMillisecondCounterHiRes ();
    return now - last_bt_time_ > backtrace_cooldown_time;
  }

  std::string get_log_file_path () const;

private:
  std::shared_ptr<spdlog::logger> logger_;

  /**
   * Last timestamp a backtrace was obtained.
   *
   * This is used to avoid calculating too many backtraces and showing too
   * many error popups at once.
   */
  double last_bt_time_;

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
