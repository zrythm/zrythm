// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <chrono>
#include <regex>

#include "utils/datetime.h"
#include "utils/gtest_wrapper.h"

#include "tests/helpers/zrythm_helper.h"

TEST (DatetimeTest, GetCurrentAsString)
{
  // Define a regex pattern for the expected format: YYYY-MM-DD HH:MM:SS
  const std::regex datetime_pattern (R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");

  // Get the current datetime string
  auto datetime_str = datetime_get_current_as_string ();

  // Check if the returned string matches the expected format
  EXPECT_TRUE (std::regex_match (datetime_str, datetime_pattern))
    << "Datetime string does not match expected format: " << datetime_str;

  // Check if the returned datetime is close to the current time
  auto now = std::chrono::system_clock::now ();
  auto time_t_now = std::chrono::system_clock::to_time_t (now);
  auto time_t_str =
    Glib::DateTime::create_from_iso8601 (
      datetime_str, Glib::TimeZone::create_local ())
      .to_unix ();

  // Allow for a small time difference (e.g., 2 seconds) to account for
  // processing time
  constexpr int allowed_time_difference = 2;
  EXPECT_NEAR (time_t_str, time_t_now, allowed_time_difference)
    << "Returned datetime is not close enough to the current time";
}
