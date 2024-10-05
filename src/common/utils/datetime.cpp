// SPDX-FileCopyrightText: © 2019, 2022-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ctime>

#include "common/utils/datetime.h"
#include "common/utils/logger.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include <fmt/chrono.h>

std::string
datetime_get_current_as_string ()
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
datetime_epoch_to_str (gint64 epoch, const std::string &format)
{
  GDateTime * dt = g_date_time_new_from_unix_local (epoch);
  z_return_val_if_fail (dt, "");
  char * str = g_date_time_format (dt, format.c_str ());
  g_date_time_unref (dt);
  z_return_val_if_fail (str, "");
  std::string ret = str;
  g_free (str);
  return ret;
}

std::string
datetime_get_for_filename ()
{
  Glib::DateTime datetime = Glib::DateTime::create_now_local ();
  std::string    str_datetime = datetime.format ("%F_%H-%M-%S");

  return str_datetime;
}
