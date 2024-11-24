// SPDX-CopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/exceptions.h"
#include "utils/logger.h"
#include <fmt/format.h>

ZrythmException::ZrythmException (const char * message)
    : ZrythmException (std::string (message))
{
}

ZrythmException::ZrythmException (const std::string &message)
    : message_ (message)
{
  z_warning ("Exception:\n{}", message);
}

ZrythmException::ZrythmException (const QString &message)
    : ZrythmException (message.toStdString ())
{
}

template <typename... Args>
void
ZrythmException::handle (const std::string &format, Args &&... args) const
{
  std::string errorMessage = fmt::format (
    "{}\n- {} -\n{}", QObject ::tr ("Error"), QObject::tr ("Details"), what ());
  // std::string formattedMessage =
  // fmt::format (fmt::runtime (format), std::forward<Args> (args)...);
  std::string formattedMessage =
    format_str (format, std::forward<Args> (args)...);
  errorMessage += "\n" + formattedMessage;

#if 0
  AdwDialog * win = nullptr;
  if (ZRYTHM_HAVE_UI)
    {
      win = ui_show_message_literal (QObject::tr ("Error"), errorMessage.c_str ());
    }
  else
    {
#endif
  z_warning ("{}", errorMessage.c_str ());
#if 0
    }

  return win;
#endif
}

void
ZrythmException::handle (const char * str) const
{
  handle ("{}", str);
}

void
ZrythmException::handle (const std::string &str) const
{
  handle ("{}", str);
}

void
ZrythmException::handle (const QString &str) const
{
  handle ("{}", str);
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
