// SPDX-FileCopyrightText: © 2019, 2022-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ctime>

#include "utils/datetime.h"
#include "utils/logger.h"

#include <QDateTime>

#include <fmt/chrono.h>

namespace zrythm::utils::datetime
{

Utf8String
get_current_as_string ()
{
  auto now = std::chrono::system_clock::now ();
  auto in_time_t = std::chrono::system_clock::to_time_t (now);

  std::stringstream ss;
  ss << std::put_time (std::localtime (&in_time_t), "%Y-%m-%d %H:%M:%S");
  return Utf8String::from_utf8_encoded_string (ss.str ());

  // some systems do not support std::chrono::current_zone() yet
#if 0
  auto now = round<std::chrono::seconds> (
    std::chrono::current_zone ()->to_local (std::chrono::system_clock::now ()));

  return fmt::format ("{:%Y-%m-%d %H:%M:%S}", now);
#endif
}

Utf8String
epoch_to_str (qint64 epoch, const Utf8String &format)
{
  QDateTime dt = QDateTime::fromSecsSinceEpoch (epoch);
  z_return_val_if_fail (!dt.isNull (), {});
  QString str = dt.toString (format.to_qstring ());
  return Utf8String::from_qstring (str);
}

Utf8String
get_for_filename ()
{
  QDateTime datetime = QDateTime::currentDateTime ();
  QString   str_datetime =
    datetime.toString (QString::fromUtf8 ("yyyy-MM-dd_HH-mm-ss"));

  return utils::Utf8String::from_qstring (str_datetime);
}

}; // zrythm::utils::datetime
