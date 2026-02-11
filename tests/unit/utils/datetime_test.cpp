// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <regex>

#include "utils/datetime.h"
#include "utils/types.h"

#include <QDateTime>

#include <gtest/gtest.h>

namespace zrythm::utils
{

using namespace Qt::StringLiterals;

TEST (DateTimeTest, GetCurrentAsStringFormat)
{
  // Define regex pattern for YYYY-MM-DD HH:MM:SS
  const std::regex datetime_pattern (R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");

  // Get current datetime string
  auto datetime_str = zrythm::utils::datetime::get_current_as_string ();

  // Verify format
  EXPECT_TRUE (std::regex_match (datetime_str.str (), datetime_pattern))
    << "Datetime string does not match expected format: " << datetime_str;

  // Verify timestamp is close to current time
  auto now = std::chrono::system_clock::now ();
  auto time_t_now = std::chrono::system_clock::to_time_t (now);
  auto time_t_str =
    QDateTime::fromString (datetime_str.to_qstring (), u"yyyy-MM-dd hh:mm:ss"_s)
      .toSecsSinceEpoch ();

  // Allow 2 second difference to account for processing time
  constexpr int allowed_time_difference = 2;
  EXPECT_NEAR (time_t_str, time_t_now, allowed_time_difference)
    << "Returned datetime is not close enough to current time";
}

TEST (DateTimeTest, GetCurrentAsString)
{
  auto datetime_str = zrythm::utils::datetime::get_current_as_string ();
  EXPECT_FALSE (datetime_str.empty ());
  EXPECT_TRUE (datetime_str.str ().find ('-') != std::string::npos);
  EXPECT_TRUE (datetime_str.str ().find (':') != std::string::npos);
}

TEST (DateTimeTest, EpochToStrDefault)
{
  qint64    test_epoch = 1577836800; // 2020-01-01 00:00:00
  QDateTime dt = QDateTime::fromSecsSinceEpoch (test_epoch);
  qint64    offset = dt.offsetFromUtc ();
  auto      datetime_str =
    zrythm::utils::datetime::epoch_to_str (test_epoch - offset);
  EXPECT_EQ (datetime_str, "2020-01-01 00:00:00");
}

TEST (DateTimeTest, EpochToStrCustomFormat)
{
  qint64 test_epoch = 1577836800; // 2020-01-01 00:00:00
  auto   datetime_str =
    zrythm::utils::datetime::epoch_to_str (test_epoch, u8"yyyy-MM-dd");
  EXPECT_EQ (datetime_str, "2020-01-01");
}

TEST (DateTimeTest, GetForFilename)
{
  auto filename_str = zrythm::utils::datetime::get_for_filename ();
  EXPECT_FALSE (filename_str.empty ());
  EXPECT_TRUE (filename_str.str ().find (' ') == std::string::npos);
  EXPECT_TRUE (filename_str.str ().find (':') == std::string::npos);
}

TEST (DateTimeTest, GetCurrentAsIso8601StringFormat)
{
  // Define regex pattern for ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ
  const std::regex iso8601_pattern (R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)");

  // Get current datetime string
  auto datetime_str = zrythm::utils::datetime::get_current_as_iso8601_string ();

  // Verify format
  EXPECT_TRUE (std::regex_match (datetime_str.str (), iso8601_pattern))
    << "Datetime string does not match ISO 8601 format: " << datetime_str;

  // Verify timestamp is close to current time (within 2 seconds)
  auto now = std::chrono::system_clock::now ();
  auto time_t_now = std::chrono::system_clock::to_time_t (now);

  // Parse the ISO 8601 string back to time_t
  std::tm            tm = {};
  std::istringstream ss (datetime_str.str ());
  ss >> std::get_time (&tm, "%Y-%m-%dT%H:%M:%SZ");
  auto time_t_str = std::mktime (&tm);
  // Convert to UTC by adding timezone offset if needed
  // Note: std::mktime uses local timezone, so we need to adjust
  auto offset =
    std::mktime (std::localtime (&time_t_now))
    - std::mktime (std::gmtime (&time_t_now));
  time_t_str += offset;

  constexpr int allowed_time_difference = 2;
  EXPECT_NEAR (time_t_str, time_t_now, allowed_time_difference)
    << "Returned datetime is not close enough to current time";
}

TEST (DateTimeTest, GetCurrentAsIso8601String)
{
  auto datetime_str = zrythm::utils::datetime::get_current_as_iso8601_string ();
  EXPECT_FALSE (datetime_str.empty ());
  EXPECT_TRUE (datetime_str.str ().find ('T') != std::string::npos);
  EXPECT_TRUE (datetime_str.str ().find ('Z') != std::string::npos);
  EXPECT_FALSE (datetime_str.str ().find (' ') != std::string::npos);
}
}
