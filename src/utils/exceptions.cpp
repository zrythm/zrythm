// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/exceptions.h"
#include "utils/logger.h"

namespace zrythm::utils
{

ZrythmException::ZrythmException (const char * message)
    : std::runtime_error (message)
{
}

ZrythmException::ZrythmException (const std::string &message)
    : std::runtime_error (message)
{
}

ZrythmException::ZrythmException (std::string_view message)
    : std::runtime_error (std::string (message))
{
}

ZrythmException::ZrythmException (const QString &message)
    : std::runtime_error (Utf8String::from_qstring (message).str ())
{
}

namespace
{

void
append_exception_chain (std::string &out, const std::exception &e, int level)
{
  out.append (static_cast<std::string::size_type> (level) * 2, ' ');
  out += e.what ();
  try
    {
      std::rethrow_if_nested (e);
    }
  catch (const std::exception &nested)
    {
      out += "\n";
      append_exception_chain (out, nested, level + 1);
    }
  catch (...)
    {
    }
}

} // namespace

std::string
format_exception (const std::exception &exception)
{
  std::string out;
  append_exception_chain (out, exception, 0);
  return out;
}

void
log_exception (const std::exception &exception, const std::string &context)
{
  if (context.empty ())
    z_warning ("{}", format_exception (exception));
  else
    z_warning ("{}:\n{}", context, format_exception (exception));
}

} // namespace zrythm::utils
