// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/logger.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>

namespace zrythm::utils
{

TEST (LoggerTest, UninitializedLoggerDoesNotCrash)
{
  // Before init_logging(), logger_ is nullptr. All z_* macros should be no-ops.
  z_trace ("trace message {}", 42);
  z_debug ("debug message");
  z_info ("info message");
  z_warning ("warning message");
  z_error ("error message");
  z_critical ("critical message");
}

TEST (LoggerTest, UninitializedGettersReturnDefaults)
{
  EXPECT_TRUE (get_last_log_entries (10).empty ());
  EXPECT_TRUE (get_log_file_path ().empty ());
}

TEST (LoggerTest, LogLevelEnumValues)
{
  EXPECT_LT (
    static_cast<int> (LogLevel::Trace), static_cast<int> (LogLevel::Debug));
  EXPECT_LT (
    static_cast<int> (LogLevel::Debug), static_cast<int> (LogLevel::Info));
  EXPECT_LT (
    static_cast<int> (LogLevel::Info), static_cast<int> (LogLevel::Warning));
  EXPECT_LT (
    static_cast<int> (LogLevel::Warning), static_cast<int> (LogLevel::Error));
  EXPECT_LT (
    static_cast<int> (LogLevel::Error), static_cast<int> (LogLevel::Critical));
}

TEST (LoggerTest, InitLoggingDoesNotCrash)
{
  init_logging (LoggerType::Test);
  z_info ("test message after init");
  z_debug ("debug with args {}", 123);
  z_warning ("warning with multiple args {} {}", 1, 2);
}

TEST (LoggerTest, GetLastLogEntriesReturnsEntries)
{
  init_logging (LoggerType::Test);
  z_info ("entry one");
  z_info ("entry two");
  auto entries = get_last_log_entries (2);
  EXPECT_GE (entries.size (), 2);
}

TEST (LoggerTest, GetLogFilePathReturnsNonEmpty)
{
  init_logging (LoggerType::Test);
  auto path = get_log_file_path ();
  EXPECT_FALSE (path.empty ());
}

} // namespace zrythm::utils
