// SPDX-CopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/exceptions.h"
#include "common/utils/ui.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include <fmt/format.h>

ZrythmException::ZrythmException (const std::string &message)
    : message_ (message)
{
  z_warning ("Exception:\n" + message);
}

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
      z_warning ("{}", errorMessage.c_str ());
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

const char *
ZrythmException::what () const noexcept
{
  if (full_message_.empty ())
    {
      std::ostringstream oss;
      oss << "Exception Stack:\n";
      oss << "- " << message_ << "\n";

      auto current = std::current_exception ();
      while (current)
        {
          try
            {
              std::rethrow_exception (current);
            }
          catch (const std::exception &e)
            {
              oss << "- " << e.what () << "\n";
              current =
                dynamic_cast<const std::nested_exception *> (&e)
                  ? dynamic_cast<const std::nested_exception *> (&e)->nested_ptr ()
                  : nullptr;
            }
          catch (...)
            {
              oss << "- Unknown exception\n";
              current = nullptr;
            }
        }
      full_message_ = oss.str ();
    }
  return full_message_.c_str ();
}