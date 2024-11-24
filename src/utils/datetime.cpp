// SPDX-FileCopyrightText: © 2019, 2022-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ctime>

#include <QDateTime>

#include "utils/datetime.h"
#include "utils/logger.h"
#include <fmt/chrono.h>

namespace zrythm::utils::datetime
{

std::string
get_current_as_string ()
{
  auto now = std::chrono::system_clock::now ();
  auto in_time_t = std::chrono::system_clock::to_time_t (now);

  std::stringstream ss;
  ss << std::put_time (std::localtime (&in_time_t), "%Y-%m-%d %H:%M:%S");
  return ss.str ();

  // some systems do not support std::chrono::current_zone() yet
#if 0
  auto now = round<std::chrono::seconds> (
    std::chrono::current_zone ()->to_local (std::chrono::system_clock::now ()));

  return fmt::format ("{:%Y-%m-%d %H:%M:%S}", now);
#endif
}

std::string
epoch_to_str (qint64 epoch, const std::string &format)
{
  QDateTime dt = QDateTime::fromSecsSinceEpoch (epoch);
  z_return_val_if_fail (!dt.isNull (), "");
  QString str = dt.toString (QString::fromStdString (format));
  return str.toStdString ();
}

std::string
get_for_filename ()
{
  QDateTime datetime = QDateTime::currentDateTime ();
  QString   str_datetime =
    datetime.toString (QString::fromUtf8 ("yyyy-MM-dd_HH-mm-ss"));

  return str_datetime.toStdString ();
}

}; // zrythm::utils::datetime
