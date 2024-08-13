// SPDX-FileCopyrightText: © 2019, 2022-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ctime>

#include "utils/datetime.h"
#include "utils/logger.h"

#include "gtk_wrapper.h"

std::string
datetime_get_current_as_string ()
{

  time_t t = time (nullptr);
#ifdef HAVE_LOCALTIME_R
  struct tm   tm;
  struct tm * ret = localtime_r (&t, &tm);
  z_return_val_if_fail (ret, "");
#else
  struct tm tm = *localtime (&t);
#endif

  auto str = g_strdup_printf (
    "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  auto ret = std::string (str);
  g_free (str);
  return ret;
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
