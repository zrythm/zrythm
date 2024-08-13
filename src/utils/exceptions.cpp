// SPDX-CopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/main_window.h"
#include "utils/exceptions.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <fmt/format.h>

template <typename... Args>
AdwDialog *
ZrythmException::handle (const std::string &format, Args &&... args) const
{
  std::string errorMessage =
    fmt::format ("{}\n- {} -\n{}", _ ("Error"), _ ("Details"), what ());
  // std::string formattedMessage =
  // fmt::format (fmt::runtime (format), std::forward<Args> (args)...);
  std::string formattedMessage =
    format_str (format, std::forward<Args> (args)...);
  errorMessage += "\n" + formattedMessage;

  AdwDialog * win = nullptr;
  if (ZRYTHM_HAVE_UI)
    {
      win = ui_show_message_literal (_ ("Error"), errorMessage.c_str ());
    }
  else
    {
      z_warning ("%s", errorMessage.c_str ());
    }

  return win;
}

AdwDialog *
ZrythmException::handle (const char * str) const
{
  return handle ("{}", str);
}

AdwDialog *
ZrythmException::handle (const std::string &str) const
{
  return handle ("{}", str);
}