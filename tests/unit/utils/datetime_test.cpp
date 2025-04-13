// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <regex>

#include <QDateTime>

#include "utils/datetime.h"
#include "utils/gtest_wrapper.h"
#include "utils/types.h"

TEST (DateTimeTest, GetCurrentAsStringFormat)
{
  // Define regex pattern for YYYY-MM-DD HH:MM:SS
  const std::regex datetime_pattern (R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");

  // Get current datetime string
  auto datetime_str = zrythm::utils::datetime::get_current_as_string ();

  // Verify format
  EXPECT_TRUE (std::regex_match (datetime_str, datetime_pattern))
    << "Datetime string does not match expected format: " << datetime_str;

  // Verify timestamp is close to current time
  auto now = std::chrono::system_clock::now ();
  auto time_t_now = std::chrono::system_clock::to_time_t (now);
  auto time_t_str =
    QDateTime::fromString (
      QString::fromStdString (datetime_str),
      QString::fromUtf8 ("yyyy-MM-dd hh:mm:ss"))
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
  EXPECT_TRUE (datetime_str.find ('-') != std::string::npos);
  EXPECT_TRUE (datetime_str.find (':') != std::string::npos);
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
    zrythm::utils::datetime::epoch_to_str (test_epoch, "yyyy-MM-dd");
  EXPECT_EQ (datetime_str, "2020-01-01");
}

TEST (DateTimeTest, GetForFilename)
{
  auto filename_str = zrythm::utils::datetime::get_for_filename ();
  EXPECT_FALSE (filename_str.empty ());
  EXPECT_TRUE (filename_str.find (' ') == std::string::npos);
  EXPECT_TRUE (filename_str.find (':') == std::string::npos);
}
