
#include "utils/datetime.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "zrythm.h"

JUCE_IMPLEMENT_SINGLETON (Logger);

Logger::Logger ()
{
  // Create a rotating file sink with a maximum size of 10 MB and 5 rotated
  // files
  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt> (
    get_log_file_path (), 1024 * 1024 * 10, 5);

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

  // Create a ring buffer sink with a maximum capacity
  auto ringbuffer_sink =
    std::make_shared<spdlog::sinks::ringbuffer_sink_mt> (1000);

  // Create a logger with all the sinks
  logger_ = std::make_shared<spdlog::logger> (
    "zrythm",
    spdlog::sinks_init_list{ file_sink, console_sink, ringbuffer_sink });

  // Set the log level and format
  logger_->set_level (spdlog::level::debug);
  // [time] [thread id] [level] [source file:line] message
  logger_->set_pattern ("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] [%s:%!():%#] %v");

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