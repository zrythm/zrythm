// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/backtrace.h"
#include "utils/datetime.h"
#include "utils/debug.h"
#include "utils/io_utils.h"
#include "utils/logger.h"
#include "utils/threads.h"

#include <QStandardPaths>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace zrythm::utils
{

// Opaque definition of SpdlogApi::Logger -- fully hidden from the header.
struct detail::SpdlogApi::Logger
{
  std::shared_ptr<spdlog::logger> spd_logger;
  std::filesystem::path           log_file_path;
  bool                            for_testing = false;
};

detail::SpdlogApi::~SpdlogApi ()
{
  auto * p = logger_.exchange (nullptr, std::memory_order_acq_rel);
  delete p;
}

// Prints backtraces on errors, breaks into debugger or aborts on criticals.
class ErrorHandlingSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
  ErrorHandlingSink (bool break_on_error) : break_on_error_ (break_on_error) { }

protected:
  void sink_it_ (const spdlog::details::log_msg &msg) override
  {
    if (msg.level >= spdlog::level::err)
      {
        auto bt = utils::Backtrace ().get_backtrace ("", 32, false);
        if (!juce::Process::isRunningUnderDebugger ())
          {
            fmt::println (stderr, "Backtrace:\n{}", bt);
          }

        if (break_on_error_)
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

        if (msg.level >= spdlog::level::critical)
          {
            fmt::println (stderr, "Critical error, exiting...");
            exit (EXIT_FAILURE);
          }
      }
  }

  void flush_ () override { }

private:
  bool break_on_error_ = false;
};

static spdlog::level::level_enum
to_spdlog_level (LogLevel level)
{
  switch (level)
    {
    case LogLevel::Trace:
      return spdlog::level::trace;
    case LogLevel::Debug:
      return spdlog::level::debug;
    case LogLevel::Info:
      return spdlog::level::info;
    case LogLevel::Warning:
      return spdlog::level::warn;
    case LogLevel::Error:
      return spdlog::level::err;
    case LogLevel::Critical:
      return spdlog::level::critical;
    }
  std::unreachable ();
}

namespace
{
class ThreadNameFormatter : public spdlog::custom_flag_formatter
{
public:
  void format (
    const spdlog::details::log_msg &,
    const std::tm &,
    spdlog::memory_buf_t &dest) override
  {
    thread_local const std::string name = get_current_thread_name ();
    dest.append (name.data (), name.data () + name.size ());
  }

  std::unique_ptr<spdlog::custom_flag_formatter> clone () const override
  {
    return std::make_unique<ThreadNameFormatter> ();
  }
};
} // namespace

static void
set_sink_pattern (auto &sink, bool with_date)
{
  auto formatter = std::make_unique<spdlog::pattern_formatter> ();
  formatter->add_flag<ThreadNameFormatter> ('@');

#ifdef _WIN32
#  define FUNCTION_AND_LINE_NO_PART "%s:%!():%#"
#else
#  define FUNCTION_AND_LINE_NO_PART "%!:%#"
#endif

#define TIMESTAMP_PART "%H:%M:%S.%f"
#define FINAL_PART "[%@] [%^%l%$] [" FUNCTION_AND_LINE_NO_PART "] %v"
  if (with_date)
    {
      formatter->set_pattern ("[%Y-%m-%d " TIMESTAMP_PART "] " FINAL_PART);
    }
  else
    {
      formatter->set_pattern ("[" TIMESTAMP_PART "] " FINAL_PART);
    }

#undef FUNCTION_AND_LINE_NO_PART
#undef TIMESTAMP_PART
#undef FINAL_PART

  sink->set_formatter (std::move (formatter));
}

static std::filesystem::path
make_log_file_path (LoggerType type)
{
  auto str_datetime = utils::datetime::get_for_filename ();

  if (type == LoggerType::Test)
    {
      auto tmp_log_dir = utils::io::get_temp_path () / "zrythm_test_logs";
      utils::io::mkdir (tmp_log_dir);
      return tmp_log_dir / str_datetime.to_path ();
    }

  auto user_log_dir =
    QStandardPaths::writableLocation (QStandardPaths::CacheLocation);
  auto log_filepath =
    Utf8String::from_qstring (user_log_dir).to_path ()
    / (std::u8string (u8"log_") + str_datetime.to_u8_string ());
  utils::io::mkdir (Utf8String::from_qstring (user_log_dir).to_path ());
  return log_filepath;
}

void
detail::SpdlogApi::init (LoggerType type)
{
  if (logger_.load (std::memory_order_acquire)) [[unlikely]]
    return;

  std::call_once (init_flag_, [this, type] {
    const bool for_testing = (type == LoggerType::Test);

    auto * impl = new Logger ();
    impl->for_testing = for_testing;
    impl->log_file_path = make_log_file_path (type);

    // 10 MB rotating file, 5 files max
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt> (
      utils::Utf8String::from_path (impl->log_file_path).str (),
      1024 * 1024 * 10, 5);
    set_sink_pattern (file_sink, true);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt> ();
    console_sink->set_color_mode (spdlog::color_mode::always);
    set_sink_pattern (console_sink, false);

#ifdef _MSC_VER
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt> ();
    set_sink_pattern (msvc_sink, false);
#endif

    // Ringbuffer for get_last_log_entries()
    auto ringbuffer_sink =
      std::make_shared<spdlog::sinks::ringbuffer_sink_mt> (1000);
    set_sink_pattern (ringbuffer_sink, false);

    auto error_handling_sink =
      std::make_shared<ErrorHandlingSink> (!for_testing);
    set_sink_pattern (error_handling_sink, false);

    impl->spd_logger = std::make_shared<spdlog::logger> (
      "zrythm",
      spdlog::sinks_init_list{
        file_sink, console_sink,
#ifdef _MSC_VER
        msvc_sink,
#endif
        ringbuffer_sink, error_handling_sink });

    impl->spd_logger->set_level (spdlog::level::debug);

    impl->spd_logger->set_error_handler ([for_testing] (const std::string &msg) {
      std::cerr << "Critical log: " << msg << std::endl;
      if (for_testing)
        abort ();
    });

    logger_.store (impl, std::memory_order_release);
  });
}

void
detail::SpdlogApi::
  submit (std::source_location loc, LogLevel level, std::string msg) const
{
  auto * l = logger_.load (std::memory_order_acquire);
  if ((l == nullptr) || !l->spd_logger)
    return;

  spdlog::source_loc src{
    loc.file_name (), static_cast<int> (loc.line ()), loc.function_name ()
  };
  l->spd_logger->log (src, to_spdlog_level (level), msg);
}

void
init_logging (LoggerType type)
{
  detail::log_api<>.init (type);
}

bool
is_logging_initialized ()
{
  return detail::log_api<>.logger_.load (std::memory_order_acquire) != nullptr;
}

std::vector<Utf8String>
get_last_log_entries (size_t count)
{
  auto * l = detail::log_api<>.logger_.load (std::memory_order_acquire);
  if ((l == nullptr) || !l->spd_logger)
    return {};

  auto &sinks = l->spd_logger->sinks ();
  auto  it = std::ranges::find_if (sinks, [] (const auto &sink) {
    return nullptr
           != dynamic_cast<spdlog::sinks::ringbuffer_sink_mt *> (sink.get ());
  });
  if (it == sinks.end ())
    return {};
  auto buffer_sink =
    std::dynamic_pointer_cast<spdlog::sinks::ringbuffer_sink_mt> (*it);
  if (!buffer_sink)
    return {};
  return buffer_sink->last_formatted (count)
         | std::views::transform ([&] (const auto &entry) {
             return Utf8String::from_utf8_encoded_string (entry);
           })
         | std::ranges::to<std::vector> ();
}

std::filesystem::path
get_log_file_path ()
{
  auto * l = detail::log_api<>.logger_.load (std::memory_order_acquire);
  if (l == nullptr)
    return {};
  return l->log_file_path;
}

} // namespace zrythm::utils
