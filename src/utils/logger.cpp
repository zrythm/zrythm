// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/datetime.h"
#include "utils/debug.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "zrythm.h"

JUCE_IMPLEMENT_SINGLETON (Logger);

class ErrorHandlingSink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
  void sink_it_ (const spdlog::details::log_msg &msg) override
  {
    if (msg.level >= spdlog::level::err)
      {
        if (ZRYTHM_TESTING || ZRYTHM_BREAK_ON_ERROR)
          {
            if (juce::Process::isRunningUnderDebugger ())
              {
                DEBUG_BREAK ();
              }
            else
              {
                abort ();
              }
          }
        else
          {
            // TODO: show backtrace etc.
          }
      }
  }

  void flush_ () override { }
};

Logger::Logger ()
{

  auto set_pattern = [] (auto &sink, bool with_date) {
    if (with_date)
      {
        // [time] [thread id] [level] [source file:line] message
        sink->set_pattern (
          "[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] "
#ifdef _WIN32
          "[%s:%!()" // only function name is shown on windows, so show filename
                     // too
#else
          "[%!"
#endif
          ":%#] %v");
      }
    else
      {
        // [time] [thread id] [level] [source file:line] message
        sink->set_pattern (
          "[%H:%M:%S.%e] [%t] [%^%l%$] "
#ifdef _WIN32
          "[%s:%!()" // only function name is shown on windows, so show
                     // filename too
#else
          "[%!"
#endif
          ":%#] %v");
      }
  };

  // Create a rotating file sink with a maximum size of 10 MB and 5 rotated
  // files
  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt> (
    get_log_file_path (), 1024 * 1024 * 10, 5);
  set_pattern (file_sink, true);

#if 0
    /* also log to /tmp */
    if (!tmp_log_file)
      {
        char *       datetime = datetime_get_for_filename ();
        char *       filename = g_strdup_printf ("zrythm_%s.log", datetime);
        const char * tmpdir = g_get_tmp_dir ();
        tmp_log_file = g_build_filename (tmpdir, filename, nullptr);
        g_free (filename);
        g_free (datetime);
      }
    FILE * file = fopen (tmp_log_file, "a");
    z_return_val_if_fail (file, G_LOG_WRITER_UNHANDLED);
    fprintf (file, "%s\n", str);
    fclose (file);
#endif

  // Create a stdout color sink
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt> ();
  console_sink->set_color_mode (spdlog::color_mode::always);
  set_pattern (console_sink, false);

  // Create a ring buffer sink with a maximum capacity
  auto ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt> (1000);
  set_pattern (ringbuffer_sink, false);

  auto error_handling_sink = std::make_shared<ErrorHandlingSink> ();
  set_pattern (error_handling_sink, false);

  // Create a logger with all the sinks
  logger_ = std::make_shared<spdlog::logger> (
    "zrythm",
    spdlog::sinks_init_list{
      file_sink, console_sink, ringbuffer_sink, error_handling_sink });

  // Set the log level
  logger_->set_level (spdlog::level::debug);

  // Set the error handler for critical logs
  logger_->set_error_handler ([&] (const std::string &msg) {
    // Default error handler
    std::cerr << "Critical log: " << msg << std::endl;

#if 0
      // Show a backtrace if needed
      if (need_backtrace () && !RUNNING_ON_VALGRIND)
        {
        if (!gZrythm || ZRYTHM_TESTING)
          {
            z_info ("Backtrace: {}", ev->backtrace);
          }

          auto backtrace = backtrace_get_with_lines ("", 100, true);

        if (
          ev->log_level == G_LOG_LEVEL_CRITICAL && ZRYTHM_HAVE_UI
          && !zrythm_app->bug_report_dialog)
          {
            char msg[500];
            sprintf (msg, _ ("%s has encountered an error\n"), PROGRAM_NAME);
            GtkWindow * win =
              gtk_application_get_active_window (GTK_APPLICATION (zrythm_app));
            zrythm_app->bug_report_dialog = bug_report_dialog_new (
              win ? GTK_WIDGET (win) : nullptr, msg, ev->backtrace, false);
            adw_dialog_present (
              ADW_DIALOG (zrythm_app->bug_report_dialog),
              win ? GTK_WIDGET (win) : nullptr);
          }

        /* write the backtrace to the log after
         * showing the popup (if any) */
        if (self->logfile)
          {
            g_fprintf (self->logfile, "%s\n", ev->backtrace);
            fflush (self->logfile);
          }
      }
#endif
    if (ZRYTHM_TESTING)
      abort ();
  });
}

std::string
Logger::get_log_file_path () const
{
  if (ZRYTHM_TESTING)
    {
      auto tmp_log_dir = fs::path (g_get_tmp_dir ()) / "zrythm_test_logs";
      REQUIRE_NOTHROW (io_mkdir (tmp_log_dir));
      auto str_datetime = datetime_get_for_filename ();
      return tmp_log_dir / str_datetime;
    }
  else
    {
#if 0
    char * str_datetime = datetime_get_for_filename ();
    auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
    char * user_log_dir = dir_mgr->get_dir (USER_LOG);
    self->log_filepath = g_strdup_printf (
      "%s%slog_%s.log", user_log_dir, G_DIR_SEPARATOR_S, str_datetime);
    GError * err = NULL;
    bool     success = io_mkdir (user_log_dir, &err);
    if (!success)
      {
        PROPAGATE_PREFIXED_ERROR (
          error, err, "Failed to make log directory %s", user_log_dir);
        return false;
      }
    self->logfile = fopen (self->log_filepath, "a");
    if (!self->logfile)
      {
        g_set_error (
          error, Z_UTILS_LOG_ERROR, Z_UTILS_LOG_ERROR_FAILED,
          "Failed to open logfile at %s", self->log_filepath);
        return false;
      }
    g_free (user_log_dir);
    g_free (str_datetime);
#endif
      return "zrythm.log";
    }
}

bool
Logger::need_backtrace () const
{
  constexpr int backtrace_cooldown_time = 16 * 1000 * 1000;
  auto          now = juce::Time::getMillisecondCounterHiRes ();
  return now - last_bt_time_ > backtrace_cooldown_time;
}

std::pair<std::string, std::string>
Logger::generate_compresed_file (std::string &dir, std::string &path) const
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

std::vector<std::string>
Logger::get_last_log_entries (size_t count, bool formatted) const
{
  // Get the circular buffer sink from the logger
  auto buffer_sink = std::dynamic_pointer_cast<
    spdlog::sinks::ringbuffer_sink_mt> (logger_->sinks ().back ());
  assert (buffer_sink);
  return buffer_sink->last_formatted (count);
}

Logger::~Logger ()
{
  clearSingletonInstance ();
}